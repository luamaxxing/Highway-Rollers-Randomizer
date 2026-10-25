#pragma once
#include <stdbool.h>

// name of the board loader function
#define HR_LOAD_SYMBOL "Load__Q2_6npg02711BuilderMegaFv"
// MassMan::setupAllMassModel's offset
#define HR_MODEL_OFFSET 0x18200u

bool hr_module_name_matches(const char *name);
