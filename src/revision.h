#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint64_t title_id;
    const char *load_symbol;
    uint32_t model_function_offset;
    // relative to BuilderMega
    uint32_t builder_manager_offset;
    uint32_t builder_new_match_offset;
    // relative to MassMan
    uint32_t manager_count_offset;
    uint32_t manager_records_offset;
} HrRevision;

static const HrRevision ntsc_u = {
    .title_id = 0x0005000010137D00,
    .load_symbol = "Load__Q2_6npg02711BuilderMegaFv",
    .model_function_offset = 0x18200u,
    .builder_manager_offset = 0xd78u,
    .builder_new_match_offset = 0xe10u,
    .manager_count_offset = 0x14u,
    .manager_records_offset = 0x18u,
};

bool hr_module_name_matches(const char *name);
const HrRevision *hr_find_revision(uint64_t title_id);