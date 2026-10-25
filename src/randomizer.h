#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HR_RECORD_STRIDE 0x3cu
#define HR_TYPE_OFFSET 0x2cu
#define HR_MAX_RECORDS 1024u
#define HR_FALLBACK_SEED UINT32_C(0x6d2b79f5)

typedef struct { uint8_t *records; uint32_t count; } HrBoard;
typedef struct {
    bool valid;
    bool enabled;
    uint32_t seed;
    uint32_t stock_fingerprint;
} HrMatch;
typedef enum { HR_STOCK, HR_RANDOMIZED, HR_INVALID_BOARD, HR_BOARD_CHANGED } HrResult;
typedef uint32_t (*HrNextRandom)(void *context);

uint32_t hr_type(HrBoard board, uint32_t index);
uint32_t hr_fingerprint(HrBoard board);
uint32_t hr_next_random(void *state);
uint32_t hr_bounded_random(uint32_t bound, HrNextRandom next, void *context);
void hr_shuffle(HrBoard board, uint32_t seed);
HrResult hr_apply(HrMatch *match, HrBoard board, bool fresh, bool requested, uint32_t seed);
