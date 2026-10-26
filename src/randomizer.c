#include "randomizer.h"
#include <string.h>

static uint32_t hr_type(HrBoard board, uint32_t index) {
    uint32_t result;
    memcpy(&result, board.records + index * HR_RECORD_STRIDE + HR_TYPE_OFFSET, sizeof(result));
    return result;
}

static void put_type(HrBoard board, uint32_t index, uint32_t type) {
    memcpy(board.records + index * HR_RECORD_STRIDE + HR_TYPE_OFFSET, &type, sizeof(type));
}

static uint32_t hr_fingerprint(HrBoard board) {
    uint32_t hash = (UINT32_C(2166136261) ^ board.count) * UINT32_C(16777619);
    for (uint32_t i = 0; i < board.count; ++i)
        hash = (hash ^ hr_type(board, i)) * UINT32_C(16777619);
    return hash;
}

static uint32_t hr_next_random(void *context) {
    uint32_t *state = (uint32_t *)context;
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static uint32_t hr_bounded_random(uint32_t bound, HrNextRandom next, void *context) {
    if (bound < 2) return 0;
    const uint32_t threshold = (0u - bound) % bound;
    uint32_t word;
    do { word = next(context); } while (word < threshold);
    return word % bound;
}

static uint32_t eligible_index(HrBoard board, uint32_t ordinal) {
    for (uint32_t i = 0; i < board.count; ++i) {
        if (hr_type(board, i) <= 12u && ordinal-- == 0) return i;
    }
    return board.count;
}

// # of each kind of space stays the same, board just gets shuffled
static void randomize_shuffle(HrBoard board, uint32_t seed) {
    if (seed == 0 || board.count > HR_MAX_RECORDS || (!board.records && board.count)) return;
    uint32_t eligible = 0;
    for (uint32_t i = 0; i < board.count; ++i)
        if (hr_type(board, i) <= 12u) ++eligible;
    for (uint32_t remaining = eligible; remaining > 1; --remaining) {
        const uint32_t a = eligible_index(board, remaining - 1);
        const uint32_t b = eligible_index(board, hr_bounded_random(remaining, hr_next_random, &seed));
        const uint32_t before = hr_type(board, a);
        put_type(board, a, hr_type(board, b));
        put_type(board, b, before);
    }
}

// randomizes each space independently from each other
// (could add weights eventually? right now everything is just a 1 in 13 lol)
static void randomize_independently(HrBoard board, uint32_t seed) {
    if (seed == 0) return;
    for (uint32_t i = 0; i < board.count; ++i) {
        if (hr_type(board, i) > 12u) continue;
        put_type(board, i, hr_bounded_random(13u, hr_next_random, &seed));
    }
}

HrResult hr_apply(
    HrMatch *match, 
    HrBoard board, 
    bool fresh, 
    bool randomizer_enabled,
    bool rerandomize_after_minigames,
    HrRandomizationMethod randomization_method, 
    uint32_t seed) 
{
    if (board.count > HR_MAX_RECORDS || (!board.records && board.count)) return HR_INVALID_BOARD;
    const uint32_t fingerprint = hr_fingerprint(board);
    if (fresh) {
        match->valid = true;
        match->enabled = randomizer_enabled;
        match->rerandomize = rerandomize_after_minigames;
        match->method = randomization_method;
        match->seed = randomizer_enabled ? seed : 0;
        match->stock_fingerprint = fingerprint;
    }
    if (!match->valid || !match->enabled) return HR_STOCK;
    if (fingerprint != match->stock_fingerprint) return HR_BOARD_CHANGED;
    if (match->method == HR_METHOD_RANDOMIZE_INDEPENDENTLY)
        randomize_independently(board, match->seed); else randomize_shuffle(board, match->seed);
    return HR_RANDOMIZED;
}
