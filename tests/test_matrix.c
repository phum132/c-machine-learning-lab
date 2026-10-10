#include "matrix.h"

#include <math.h>
#include <stdint.h>
#include <stddef.h>

static int expect_values(const MLMatrix *matrix, size_t rows, size_t cols,
                         const double *expected, size_t count)
{
    if (matrix_rows(matrix) != rows || matrix_cols(matrix) != cols) return 0;
    const double *data = matrix_const_data(matrix);
    for (size_t i = 0U; i < count; ++i) {
        if (fabs(data[i] - expected[i]) > 1e-12) return 0;
    }
    return 1;
}

int test_matrix(void)
{
    MLMatrix *a = NULL, *b = NULL, *result = NULL, *transposed = NULL;
    const double a_values[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const double b_values[] = {7.0, 8.0, 9.0, 10.0, 11.0, 12.0};
    const double sum[] = {8.0, 10.0, 12.0, 14.0, 16.0, 18.0};
    const double difference[] = {-6.0, -6.0, -6.0, -6.0, -6.0, -6.0};
    const double scaled[] = {2.0, 4.0, 6.0, 8.0, 10.0, 12.0};
    const double transpose[] = {1.0, 4.0, 2.0, 5.0, 3.0, 6.0};
    if (matrix_create(2U, 3U, &a) != ML_STATUS_OK || matrix_create(2U, 3U, &b) != ML_STATUS_OK) goto fail;
    for (size_t i = 0U; i < 6U; ++i) { matrix_data(a)[i] = a_values[i]; matrix_data(b)[i] = b_values[i]; }
    if (matrix_add(a, b, &result) != ML_STATUS_OK || !expect_values(result, 2U, 3U, sum, 6U)) goto fail;
    matrix_destroy(result); result = NULL;
    if (matrix_subtract(a, b, &result) != ML_STATUS_OK || !expect_values(result, 2U, 3U, difference, 6U)) goto fail;
    matrix_destroy(result); result = NULL;
    if (matrix_scale(a, 2.0, &result) != ML_STATUS_OK || !expect_values(result, 2U, 3U, scaled, 6U)) goto fail;
    matrix_destroy(result); result = NULL;
    if (matrix_transpose(a, &transposed) != ML_STATUS_OK || !expect_values(transposed, 3U, 2U, transpose, 6U)) goto fail;
    matrix_destroy(transposed); transposed = NULL;
    {
        MLMatrix *right = NULL;
        const double right_values[] = {7.0, 8.0, 9.0, 10.0, 11.0, 12.0};
        const double product[] = {58.0, 64.0, 139.0, 154.0};
        if (matrix_create(3U, 2U, &right) != ML_STATUS_OK) goto fail;
        for (size_t i = 0U; i < 6U; ++i) matrix_data(right)[i] = right_values[i];
        if (matrix_multiply(a, right, &result) != ML_STATUS_OK || !expect_values(result, 2U, 2U, product, 4U)) {
            matrix_destroy(right); goto fail;
        }
        matrix_destroy(right); matrix_destroy(result); result = NULL;
    }
    {
        const double x[] = {1.0, 2.0, 3.0}, y[] = {4.0, 5.0, 6.0};
        double dot = 0.0;
        if (matrix_dot(x, y, 3U, &dot) != ML_STATUS_OK || dot != 32.0) goto fail;
    }
    if (matrix_transpose(a, &transposed) != ML_STATUS_OK) goto fail;
    if (matrix_add(a, transposed, &result) != ML_STATUS_DIMENSION_MISMATCH || result != NULL) goto fail;
    matrix_destroy(transposed); transposed = NULL;
    if (matrix_create(0U, 3U, &result) != ML_STATUS_INVALID_ARGUMENT || result != NULL) goto fail;
    if (matrix_create(SIZE_MAX, 2U, &result) != ML_STATUS_INVALID_ARGUMENT || result != NULL) goto fail;
    if (matrix_scale(a, INFINITY, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto fail;
    matrix_data(a)[0] = NAN;
    if (matrix_scale(a, 1.0, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto fail;
    matrix_destroy(a); matrix_destroy(b);
    return 0;
fail:
    matrix_destroy(a); matrix_destroy(b); matrix_destroy(result); matrix_destroy(transposed);
    return 1;
}
