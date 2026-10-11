#include "cross_validation.h"

#include <stdlib.h>

static uint64_t next_random(uint64_t *state)
{
    uint64_t value = *state;
    value ^= value >> 12U; value ^= value << 25U; value ^= value >> 27U;
    *state = value;
    return value * UINT64_C(2685821657736338717);
}

MLStatus ml_kfold_assign(size_t sample_count, size_t fold_count, uint64_t seed, size_t *fold_of_row)
{
    size_t *order;
    uint64_t state = seed == 0U ? UINT64_C(0x9e3779b97f4a7c15) : seed;
    if (fold_of_row == NULL || fold_count < 2U || fold_count > sample_count ||
        sample_count > SIZE_MAX / sizeof(*order))
        return ML_STATUS_INVALID_ARGUMENT;
    order = malloc(sample_count * sizeof(*order));
    if (order == NULL) return ML_STATUS_OUT_OF_MEMORY;
    for (size_t i = 0U; i < sample_count; ++i) order[i] = i;
    /* Fisher-Yates shuffle, then deal rows to folds like cards. */
    for (size_t i = sample_count - 1U; i > 0U; --i) {
        const size_t j = (size_t)(next_random(&state) % (uint64_t)(i + 1U));
        const size_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }
    for (size_t position = 0U; position < sample_count; ++position)
        fold_of_row[order[position]] = position % fold_count;
    free(order);
    return ML_STATUS_OK;
}
