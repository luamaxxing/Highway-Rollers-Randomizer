#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HR_RECORD_STRIDE 0x3cu
#define HR_TYPE_OFFSET 0x2cu
#define HR_MAX_RECORDS 1024u

typedef struct
{
    uint8_t *records;
    uint32_t count;
} HrBoard;
typedef enum
{
    HR_METHOD_SHUFFLE_EXISTING = 0,
    HR_METHOD_RANDOMIZE_INDEPENDENTLY = 1
} HrRandomizationMethod;
typedef struct
{
    bool valid;
    bool enabled;
    bool rerandomize;
    HrRandomizationMethod method;
    uint32_t seed;
    uint32_t stock_fingerprint;
} HrMatch;
typedef enum
{
    HR_STOCK,
    HR_RANDOMIZED,
    HR_INVALID_BOARD,
    HR_BOARD_CHANGED
} HrResult;
typedef uint32_t (*HrNextRandom)(void *context);

HrResult hr_apply(HrMatch *match, HrBoard board, bool fresh, bool randomizer_enabled, bool rerandomize,
    HrRandomizationMethod method, uint32_t seed);
