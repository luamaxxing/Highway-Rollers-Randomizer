#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <coreinit/dynload.h>
#include <coreinit/memorymap.h>
#include <coreinit/time.h>
#include <coreinit/title.h>
#include <function_patcher/function_patching.h>
#include <notifications/notifications.h>
#include <stdio.h>
#include <string.h>
#include "randomizer.h"
#include "revision.h"

WUPS_PLUGIN_NAME("Highway Rollers Randomizer");
WUPS_PLUGIN_DESCRIPTION("Randomizes the order of all the spaces in Wii Party U's Highway Rollers.");
WUPS_PLUGIN_VERSION("v0.1.2");
WUPS_PLUGIN_AUTHOR("luamaxxing");
WUPS_PLUGIN_LICENSE("GPL");
WUPS_USE_STORAGE("highway_rollers_randomizer");

static const uint64_t target_titles[] = { UINT64_C(0x0005000010137d00) }; // title id of NTSC-U wii party u
static const char randomizer_enabled_key[] = "randomize_board";
static const char rerandomize_after_minigames_key[] = "rerandomize_after_minigames";
static const char randomization_method_key[] = "randomization_method";
static volatile bool randomizer_enabled = false;
static volatile bool rerandomize_after_minigames = false;
static volatile HrRandomizationMethod randomization_method = HR_METHOD_SHUFFLE_EXISTING;

static HrMatch match;
static PatchedFunctionHandle patch_handle;

static bool patch_registered;
static bool library_ready;
static bool notifications_ready;
static bool config_ready;

static const char notification_prefix[] = "Highway Rollers Randomizer: ";

// notification helper
static void notify_message(const char *message, bool error) {
    if (!notifications_ready) return;

    char text[256];
    snprintf(text, sizeof(text), "%s%s", notification_prefix, message);

    if (error) (void)NotificationModule_AddErrorNotification(text);
    else (void)NotificationModule_AddInfoNotification(text);
}
static void notify_error(const char *message) {
    notify_message(message, true);
}
static void notify_info(const char *message) {
    notify_message(message, false);
}

// helper to verify if a memory range is accessible
static bool mapped_range(uintptr_t address, uint32_t size) {
    if (!address || (address & 3u) || !size || address > UINT32_MAX - size) return false;
    const uintptr_t end = address + size - 1;
    if (!OSIsAddressValid((uint32_t)address) || !OSIsAddressValid((uint32_t)end)) return false;
    for (uintptr_t p = (address & ~(uintptr_t)0xfffu) + 0x1000u; p < end; p += 0x1000u)
        if (!OSIsAddressValid((uint32_t)p)) return false;
    return true;
}
static uint32_t read_field(const void *object, size_t offset) {
    uint32_t result;
    memcpy(&result, (const uint8_t *)object + offset, sizeof(result));
    return result;
}

// find the loaded highway rollers module and and verify we can access board builder
// pass in rebuild and it'll get updated to the board builder
static bool resolve_module(void (**rebuild)(void *)) {
    const int32_t count = OSDynLoad_GetNumberOfRPLs();
    for (int32_t i = 0; i < count; ++i) {
        OSDynLoad_NotifyData info;
        if (!OSDynLoad_GetRPLInfo((uint32_t)i, 1, &info)) return false;
        if (!hr_module_name_matches(info.name)) continue;
        if (info.textSize < HR_MODEL_OFFSET + sizeof(uint32_t) ||
            !mapped_range(info.textAddr, info.textSize)) return false;
        *rebuild = (void (*)(void *))(uintptr_t)(info.textAddr + HR_MODEL_OFFSET);
        return true;
    }
    return false;
}

// our board loader hook
// gets swapped with functionpatchermodule once we know we're in highway rollers
// this function gets called when the game first loads, and after every minigame
DECL_FUNCTION(void, BuilderLoad, void *builder) {
    real_BuilderLoad(builder);

    // the function that actually "builds" the board
    // call to update board apperance after we swap spaces
    void (*rebuild)(void *) = NULL;
    if (!resolve_module(&rebuild)) {
        notify_error("Couldn't fetch the board builder, unable to randomize.");
        return;
    }

    // BuilderMega +D78 = board manager* (MassMan)
    // BuilderMega +E14 = original new-match flag (+4-byte length)
    if (!mapped_range((uintptr_t)builder, 0xe14u)) {
        notify_error("Board builder was potentially already modified, unable to randomize.");
        return;
    }
    void *manager = (void *)(uintptr_t)read_field(builder, 0xd78u);
    if (!mapped_range((uintptr_t)manager, 0x50u)) {
        notify_error("Couldn't fetch the board manager, unable to randomize.");
        return;
    }

    // manager +14 = record count
    // manager +18 = array of 0x3c-byte MassInfo records
    HrBoard board = { 
        (uint8_t *)(uintptr_t)read_field(manager, 0x18u), 
        read_field(manager, 0x14u) 
    };
    if (!board.count || board.count > HR_MAX_RECORDS ||
        !mapped_range((uintptr_t)board.records, board.count * HR_RECORD_STRIDE)) {
        notify_error("Couldn't fetch board space data, unable to randomize.");
        return;
    }

    const uint32_t new_match = read_field(builder, 0xe10u);
    if (new_match > 1u) {
        notify_error("Failed to read new_match, unable to randomize.");
        return;
    }

    // seed for board generation, changes per minigame if option is enabled
    uint32_t seed = 0;
    if (randomizer_enabled && (new_match || match.rerandomize)) {
        seed = (uint32_t)OSGetTime();
        if (!new_match) match.seed = seed;
    }

    // call the randomizer
    const HrResult result = hr_apply(
        &match, 
        board, 
        new_match != 0, 
        randomizer_enabled,
        rerandomize_after_minigames,
        randomization_method, 
        seed
    );
    if (result == HR_RANDOMIZED) {
        rebuild(manager);
        
        #ifdef HR_DEBUG
            char seed_message[32];
            snprintf(seed_message, sizeof(seed_message), "Seed: 0x%08X", (unsigned)match.seed);
            notify_info("Board randomized!");
            notify_info(seed_message);
            notify_info(match.method == HR_METHOD_RANDOMIZE_INDEPENDENTLY
                ? "Randomized Independently"
                : "Shuffled Existing");
        #endif
    } else if (result != HR_STOCK) {
        notify_error("Failed to randomize the board.");
    }
}

static void enabled_setting_changed(ConfigItemBoolean *item, bool value) {
    (void)item;
    randomizer_enabled = value;
}

static void rerandomize_setting_changed(ConfigItemBoolean *item, bool value) {
    (void)item;
    rerandomize_after_minigames = value;
}

static void method_setting_changed(ConfigItemMultipleValues *item, uint32_t value) {
    (void)item;
    randomization_method = (HrRandomizationMethod)value;
}

static WUPSConfigAPICallbackStatus menu_opened(WUPSConfigCategoryHandle root) {
    if (OSGetTitleID() != target_titles[0]) {
        if (WUPSConfigItemStub_AddToCategory(root, "Can only be used while Wii Party U is open.")
                != WUPSCONFIG_API_RESULT_SUCCESS)
            return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
        return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
    }

    if (WUPSConfigItemBoolean_AddToCategory(root, randomizer_enabled_key,
            "Randomizer Enabled", false, randomizer_enabled, enabled_setting_changed)
            != WUPSCONFIG_API_RESULT_SUCCESS) {
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    WUPSConfigCategoryHandle options;
    const WUPSConfigAPICreateCategoryOptionsV1 category_options = { .name = "Randomization Options" };
    if (WUPSConfigAPI_Category_Create(category_options, &options) != WUPSCONFIG_API_RESULT_SUCCESS)
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;

    ConfigItemMultipleValuesPair methods[] = {
        { HR_METHOD_SHUFFLE_EXISTING, "Shuffle Existing" },
        { HR_METHOD_RANDOMIZE_INDEPENDENTLY, "Randomize Independently" }
    };

    if (WUPSConfigItemBoolean_AddToCategory(options, rerandomize_after_minigames_key,
            "Re-Randomize After Minigames", false, rerandomize_after_minigames, rerandomize_setting_changed)
            != WUPSCONFIG_API_RESULT_SUCCESS ||
        WUPSConfigItemMultipleValues_AddToCategory(options, randomization_method_key,
            "Randomization Method", HR_METHOD_SHUFFLE_EXISTING, randomization_method,
            methods, 2, method_setting_changed) != WUPSCONFIG_API_RESULT_SUCCESS ||
        WUPSConfigAPI_Category_AddCategory(root, options) != WUPSCONFIG_API_RESULT_SUCCESS) 
    {
        WUPSConfigAPI_Category_Destroy(options);
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

static void menu_closed(void) {
    if (OSGetTitleID() != target_titles[0]) return;
    if (WUPSStorageAPI_StoreBool(NULL, randomizer_enabled_key, randomizer_enabled) != WUPS_STORAGE_ERROR_SUCCESS ||
        WUPSStorageAPI_StoreBool(NULL, rerandomize_after_minigames_key, rerandomize_after_minigames) != WUPS_STORAGE_ERROR_SUCCESS ||
        WUPSStorageAPI_StoreU32(NULL, randomization_method_key, randomization_method) != WUPS_STORAGE_ERROR_SUCCESS) 
    {
        notify_error("Could not save randomization config.");
        return;
    }
    if (WUPSStorageAPI_SaveStorage(false) != WUPS_STORAGE_ERROR_SUCCESS)
        notify_error("Could not save randomization config to SD.");
}

INITIALIZE_PLUGIN() {
    const WUPSConfigAPIOptionsV1 options = { .name = "Highway Rollers Randomizer" };
    config_ready = WUPSConfigAPI_Init(options, menu_opened, menu_closed) == WUPSCONFIG_API_RESULT_SUCCESS;
}

static void start_game(void) {
    notifications_ready = NotificationModule_InitLibrary() == NOTIFICATION_MODULE_RESULT_SUCCESS;

    // not sure why by default notifications disappear hella quick, increase time shown
    if (notifications_ready) {
        NotificationModule_SetDefaultValue(
            NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO,
            NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT,
            5.0);   
        NotificationModule_SetDefaultValue(
            NOTIFICATION_MODULE_NOTIFICATION_TYPE_ERROR,
            NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT,
            10.0);
    }

    // load options from storage
    bool saved = false;
    randomizer_enabled = WUPSStorageAPI_GetBool(NULL, randomizer_enabled_key, &saved)
        == WUPS_STORAGE_ERROR_SUCCESS && saved;
    rerandomize_after_minigames = WUPSStorageAPI_GetBool(NULL, rerandomize_after_minigames_key, &saved)
        == WUPS_STORAGE_ERROR_SUCCESS && saved;
    uint32_t saved_method = HR_METHOD_SHUFFLE_EXISTING;
    randomization_method = WUPSStorageAPI_GetU32(NULL, randomization_method_key, &saved_method)
        == WUPS_STORAGE_ERROR_SUCCESS && saved_method <= HR_METHOD_RANDOMIZE_INDEPENDENTLY
        ? (HrRandomizationMethod)saved_method : HR_METHOD_SHUFFLE_EXISTING;

    if (patch_registered) return;
    
    if (!library_ready) library_ready = FunctionPatcher_InitLibrary() == FUNCTION_PATCHER_RESULT_SUCCESS;
    if (!library_ready) {
        notify_error("FunctionPatcherModule unavailable, unable to load.");
        return;
    }

    // replace the board loader function with our implementation
    function_replacement_data_t replacement = REPLACE_FUNCTION_OF_EXECUTABLE_BY_FUNCTION_NAME(
        BuilderLoad, target_titles, 1, "npg027.rpl", HR_LOAD_SYMBOL);
    replacement.targetProcess = FP_TARGET_PROCESS_GAME;
    patch_registered = FunctionPatcher_AddFunctionPatch(&replacement, &patch_handle, NULL) == FUNCTION_PATCHER_RESULT_SUCCESS;
    
    if (!patch_registered) {
        notify_error("Failed during hook initialization, unable to load.");
        return;
    }

    notify_info(randomizer_enabled
        ? "The randomizer is currently on."
        : "The randomizer is currently off.");
}

ON_APPLICATION_START() {
    if (OSGetTitleID() != target_titles[0]) return;
    memset(&match, 0, sizeof(match));
    start_game();
}

static void stop_game(void) {
    if (patch_registered) {
        const FunctionPatcherStatus result = FunctionPatcher_RemoveFunctionPatch(patch_handle);
        if (result != FUNCTION_PATCHER_RESULT_SUCCESS)
            notify_error("Could not remove the board randomization hook.");
        else patch_registered = false;
    }
    if (library_ready && !patch_registered) {
        if (FunctionPatcher_DeInitLibrary() != FUNCTION_PATCHER_RESULT_SUCCESS)
            notify_error("Could not release FunctionPatcherModule.");
        else library_ready = false;
    }
    if (notifications_ready) {
        (void)NotificationModule_DeInitLibrary();
        notifications_ready = false;
    }
    memset(&match, 0, sizeof(match));
}

ON_APPLICATION_ENDS() { stop_game(); }
DEINITIALIZE_PLUGIN() { stop_game(); }
