#include "weight_editor.h"
#include <stdio.h>
#include <string.h>
#include <wups/config/WUPSConfigItemStub.h>

static const char space_weights_key[] = "space_weights";
static uint16_t draft_weights[HR_SPACE_TYPE_COUNT];
static bool editor_created;
static bool reset_item_selected;

uint16_t space_weights[HR_SPACE_TYPE_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

#define WEIGHT_MAX 100
typedef struct
{
    uint32_t type;
    bool selected;
} WeightEditorItem;
static WeightEditorItem weight_items[HR_SPACE_TYPE_COUNT];
static const char *const space_names[HR_SPACE_TYPE_COUNT] = {
    "Blank",
    "Forward 10",
    "Forward 20",
    "Backwards 10",
    "Backwards 20",
    "5 Dice Forwards",
    "10 Dice Forwards",
    "5 Dice Backwards",
    "10 Dice Backwards",
    "Alien",
    "Last Place Boost",
    "Versus",
    "Rocket",
};

// idea for another day:
// maybe actual presets instead of resetting everything to equal
// (preset thats equal to games usual space counts, hardmode preset (way more negative then positive), etc.)
static void default_weights(uint16_t weights[HR_SPACE_TYPE_COUNT])
{
    for (int type = 0; type < HR_SPACE_TYPE_COUNT; ++type)
        weights[type] = 1;
}

// if we dont verify at least 1 space type has a weight the game will Break
static bool validate_weights(const uint16_t weights[HR_SPACE_TYPE_COUNT])
{
    uint32_t total = 0;
    for (int type = 0; type < HR_SPACE_TYPE_COUNT; ++type)
    {
        if (weights[type] > WEIGHT_MAX)
            return false;
        total += weights[type];
    }
    return total != 0;
}

// weight selectors

static int32_t weight_display(void *context, char *out, int32_t size)
{
    const WeightEditorItem *item = context;
    snprintf(out, size, item->selected ? "< %u >" : "%u", (unsigned)draft_weights[item->type]);
    return 0;
}

static void weight_selected(void *context, bool selected)
{
    WeightEditorItem *item = context;
    item->selected = selected;
}

static void weight_input(void *context, WUPSConfigSimplePadData input)
{
    const WeightEditorItem *item = context;
    if (!item->selected)
        return;

    int value = draft_weights[item->type];
    if (input.buttons_d & WUPS_CONFIG_BUTTON_LEFT)
        --value;
    else if (input.buttons_d & WUPS_CONFIG_BUTTON_RIGHT)
        ++value;
    else if (input.buttons_d & WUPS_CONFIG_BUTTON_L)
        value -= 10;
    else if (input.buttons_d & WUPS_CONFIG_BUTTON_R)
        value += 10;

    if (value < 0)
        value = 0;
    if (value > WEIGHT_MAX)
        value = WEIGHT_MAX;

    draft_weights[item->type] = (uint16_t)value;
}

static WUPSConfigAPIStatus create_weight_selector(WUPSConfigCategoryHandle category, uint32_t type, const char *name)
{
    weight_items[type] = (WeightEditorItem){.type = type};
    const WUPSConfigAPIItemOptionsV2 options = {
        .displayName = name,
        .context = &weight_items[type],
        .callbacks =
            {
                .getCurrentValueDisplay = weight_display,
                .getCurrentValueSelectedDisplay = weight_display,
                .onSelected = weight_selected,
                .onInput = weight_input,
            },
    };
    WUPSConfigItemHandle handle;
    WUPSConfigAPIStatus status = WUPSConfigAPI_Item_Create(options, &handle);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        return status;
    status = WUPSConfigAPI_Category_AddItem(category, handle);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        WUPSConfigAPI_Item_Destroy(handle);
    return status;
}

// reset button

static int32_t reset_display(void *context, char *out, int32_t size)
{
    (void)context;
    snprintf(out, size, "Press A");
    return 0;
}

static void reset_selected(void *context, bool selected) { *(bool *)context = selected; }

static void reset_input(void *context, WUPSConfigSimplePadData input)
{
    if (*(bool *)context && (input.buttons_d & WUPS_CONFIG_BUTTON_A))
        default_weights(draft_weights);
}

static WUPSConfigAPIStatus create_reset_button(WUPSConfigCategoryHandle category)
{
    reset_item_selected = false;
    const WUPSConfigAPIItemOptionsV2 options = {
        .displayName = "Reset to Default",
        .context = &reset_item_selected,
        .callbacks =
            {
                .getCurrentValueDisplay = reset_display,
                .getCurrentValueSelectedDisplay = reset_display,
                .onSelected = reset_selected,
                .onInput = reset_input,
            },
    };

    WUPSConfigItemHandle handle;
    WUPSConfigAPIStatus status = WUPSConfigAPI_Item_Create(options, &handle);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        return status;

    status = WUPSConfigAPI_Category_AddItem(category, handle);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        WUPSConfigAPI_Item_Destroy(handle);

    return status;
}

// public

bool save_weights(bool *restored_previous)
{
    bool restored = false;

    if (editor_created)
    {
        if (validate_weights(draft_weights))
            memcpy(space_weights, draft_weights, sizeof(space_weights));
        else
        {
            memcpy(draft_weights, space_weights, sizeof(draft_weights));
            restored = true;
        }
        editor_created = false;
    }

    if (restored_previous)
        *restored_previous = restored;

    return WUPSStorageAPI_StoreBinary(NULL, space_weights_key, space_weights, sizeof(space_weights)) ==
           WUPS_STORAGE_ERROR_SUCCESS;
}

void load_weights(void)
{
    editor_created = false;
    default_weights(space_weights);
    uint16_t saved[HR_SPACE_TYPE_COUNT];
    uint32_t bytes = 0;

    if (WUPSStorageAPI_GetBinary(NULL, space_weights_key, saved, sizeof(saved), &bytes) == WUPS_STORAGE_ERROR_SUCCESS &&
        bytes == sizeof(saved) && validate_weights(saved))
    {
        memcpy(space_weights, saved, sizeof(space_weights));
    }
}

WUPSConfigAPIStatus hr_create_weight_editor(WUPSConfigCategoryHandle parent)
{
    editor_created = false;
    memcpy(draft_weights, space_weights, sizeof(draft_weights));

    WUPSConfigCategoryHandle editor;
    const WUPSConfigAPICreateCategoryOptionsV1 category_options = {.name = "Weight Editor"};
    WUPSConfigAPIStatus status = WUPSConfigAPI_Category_Create(category_options, &editor);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        return status;

    status = WUPSConfigItemStub_AddToCategory(editor, "Weights only apply to the 'Weighted' randomization mode");
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        goto fail;

    status =
        WUPSConfigItemStub_AddToCategory(editor, "A higher weight makes a space more likely to appear on the board");
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        goto fail;

    status = WUPSConfigItemStub_AddToCategory(editor, "===");
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        goto fail;

    status = create_reset_button(editor);
    if (status != WUPSCONFIG_API_RESULT_SUCCESS)
        goto fail;

    for (uint32_t type = 0; type < HR_SPACE_TYPE_COUNT; ++type)
    {
        status = create_weight_selector(editor, type, space_names[type]);
        if (status != WUPSCONFIG_API_RESULT_SUCCESS)
            goto fail;
    }

    status = WUPSConfigAPI_Category_AddCategory(parent, editor);
    if (status == WUPSCONFIG_API_RESULT_SUCCESS)
    {
        editor_created = true;
        return status;
    }
fail:
    WUPSConfigAPI_Category_Destroy(editor);
    return status;
}