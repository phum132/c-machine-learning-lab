#ifndef CROSS_VALIDATION_H
#define CROSS_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

#include "ml_error.h"

/*
 * Assign each of `sample_count` rows to one of `fold_count` folds after a
 * seeded shuffle. Fold sizes differ by at most one row, and the same seed
 * always gives the same assignment. `fold_of_row` is caller-owned and must
 * hold sample_count entries; it is written only on success.
 * Requires 2 <= fold_count <= sample_count.
 */
MLStatus ml_kfold_assign(size_t sample_count, size_t fold_count, uint64_t seed, size_t *fold_of_row);

#endif
