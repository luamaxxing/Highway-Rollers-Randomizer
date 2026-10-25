#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
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
WUPS_PLUGIN_VERSION("v0.1.0");
WUPS_PLUGIN_AUTHOR("luamaxxing");
WUPS_PLUGIN_LICENSE("GPL");
WUPS_USE_STORAGE("highway_rollers_randomizer");

static const uint64_t target_titles[] = { UINT64_C(0x0005000010137d00) }; // title id of NTSC-U wii party u
static const char setting_key[] = "randomize_board";
static volatile bool requested = false;

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

    const bool next_match_enabled = requested;

    // make generation persist throughout minigames
    uint32_t seed = 0;
    if (new_match && next_match_enabled) {
        const uint64_t ticks = (uint64_t)OSGetTime();
        seed = (uint32_t)ticks ^ (uint32_t)(ticks >> 32);
    }

    // call the randomizer
    const HrResult result = hr_apply(&match, board, new_match != 0, next_match_enabled, seed);
    if (result == HR_RANDOMIZED) {
        rebuild(manager);
        
        #ifdef HR_DEBUG
            char seed_message[32];
            snprintf(seed_message, sizeof(seed_message), "Seed: 0x%08X", (unsigned)match.seed);
            notify_info("Board randomized!");
            notify_info(seed_message);
        #endif
    } else if (result != HR_STOCK) {
        notify_error("Failed to randomize the board.");
    }
}

static void setting_changed(ConfigItemBoolean *item, bool value) {
    (void)item;
    requested = value;
}

static WUPSConfigAPICallbackStatus menu_opened(WUPSConfigCategoryHandle root) {
    if (OSGetTitleID() != target_titles[0]) return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;

    if (WUPSConfigItemBoolean_AddToCategory(root, setting_key,
            "Randomizer Enabled", false, requested, setting_changed)
            != WUPSCONFIG_API_RESULT_SUCCESS) {
        notify_error("Could not add the configuration toggle.");
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

static void menu_closed(void) {
    if (OSGetTitleID() != target_titles[0]) return;
    if (WUPSStorageAPI_StoreBool(NULL, setting_key, requested) != WUPS_STORAGE_ERROR_SUCCESS) {
        notify_error("Could not save randomization config.");
        return;
    }
    if (WUPSStorageAPI_SaveStorage(false) != WUPS_STORAGE_ERROR_SUCCESS)
        notify_error("Could not save randomization config to SD.");
}

INITIALIZE_PLUGIN() {} 

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

    bool saved = false;
    const WUPSStorageError stored = WUPSStorageAPI_GetBool(NULL, setting_key, &saved);
    requested = stored == WUPS_STORAGE_ERROR_SUCCESS ? saved : false;

    if (stored != WUPS_STORAGE_ERROR_SUCCESS && stored != WUPS_STORAGE_ERROR_NOT_FOUND) {
        notify_error("Could not read your saved config. Defaulting to off.");
    }

    // now that we're actually ingame we init the aroma config editor
    // may not be needed actually maybe we can init this outside the game :p
    const WUPSConfigAPIOptionsV1 options = { .name = "Highway Rollers Randomizer" };
    if (!config_ready)
        config_ready = WUPSConfigAPI_Init(options, menu_opened, menu_closed) == WUPSCONFIG_API_RESULT_SUCCESS;
    if (!config_ready) {
        notify_error("Aroma config API unavailable, unable to load.");
        return;
    }
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

    notify_info("Loaded successfully!");
    notify_info(requested
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
