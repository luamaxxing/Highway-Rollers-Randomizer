#pragma once
#include "randomizer.h"
#include <wups.h>

extern uint16_t space_weights[HR_SPACE_TYPE_COUNT];

bool save_weights(bool *restored_previous);
void load_weights(void);

WUPSConfigAPIStatus hr_create_weight_editor(WUPSConfigCategoryHandle parent);
