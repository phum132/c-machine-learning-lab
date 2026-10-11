#include "cross_validation.h"

#include <stddef.h>
#include <string.h>

int test_cross_validation(void)
{
    size_t folds_a[23], folds_b[23], folds_c[23];
    size_t sizes[5] = {0U, 0U, 0U, 0U, 0U};

    /* 23 rows into 5 folds: sizes 5,5,5,4,4 in some order, every row assigned once. */
    if (ml_kfold_assign(23U, 5U, 7U, folds_a) != ML_STATUS_OK) return 1;
    for (size_t i = 0U; i < 23U; ++i) {
        if (folds_a[i] >= 5U) return 1;
        ++sizes[folds_a[i]];
    }
    for (size_t f = 0U; f < 5U; ++f)
        if (sizes[f] < 4U || sizes[f] > 5U) return 1;

    /* Same seed, same folds; a different seed shuffles differently. */
    if (ml_kfold_assign(23U, 5U, 7U, folds_b) != ML_STATUS_OK ||
        memcmp(folds_a, folds_b, sizeof(folds_a)) != 0) return 1;
    if (ml_kfold_assign(23U, 5U, 8U, folds_c) != ML_STATUS_OK ||
        memcmp(folds_a, folds_c, sizeof(folds_a)) == 0) return 1;

    /* Leave-one-out is allowed; fewer than 2 folds or more folds than rows are not. */
    if (ml_kfold_assign(3U, 3U, 1U, folds_b) != ML_STATUS_OK) return 1;
    if (folds_b[0] == folds_b[1] || folds_b[1] == folds_b[2] || folds_b[0] == folds_b[2]) return 1;
    if (ml_kfold_assign(23U, 1U, 1U, folds_b) != ML_STATUS_INVALID_ARGUMENT ||
        ml_kfold_assign(3U, 4U, 1U, folds_b) != ML_STATUS_INVALID_ARGUMENT ||
        ml_kfold_assign(23U, 5U, 1U, NULL) != ML_STATUS_INVALID_ARGUMENT) return 1;
    return 0;
}
