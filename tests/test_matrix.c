#include "matrix.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

static int expect_values(const MLMatrix *matrix, size_t rows, size_t cols,
                         const double *expected, size_t count)
{
    if (matrix_rows(matrix) != rows || matrix_cols(matrix) != cols) return 0;
    const double *data = matrix_const_data(matrix);
    if (data == NULL) return 0;
    for (size_t i = 0U; i < count; ++i) {
        if (fabs(data[i] - expected[i]) > 1e-12) return 0;
    }
    return 1;
}

static int test_invalid_arguments(void)
{
    MLMatrix *result = NULL;
    double dot = 123.0;

#define CHECK_INVALID(condition) do { if (!(condition)) return 0; } while (0)
    CHECK_INVALID(matrix_create(1U, 1U, NULL) == ML_STATUS_INVALID_ARGUMENT);
    CHECK_INVALID(matrix_create(0U, 1U, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_create(1U, 0U, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_create(SIZE_MAX, 2U, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_create(SIZE_MAX / sizeof(double) + 1U, 1U, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_rows(NULL) == 0U && matrix_cols(NULL) == 0U &&
        matrix_data(NULL) == NULL && matrix_const_data(NULL) == NULL);
    matrix_destroy(NULL);

    CHECK_INVALID(matrix_add(NULL, NULL, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_subtract(NULL, NULL, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_multiply(NULL, NULL, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_scale(NULL, 1.0, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_transpose(NULL, &result) == ML_STATUS_INVALID_ARGUMENT && result == NULL);
    CHECK_INVALID(matrix_add(NULL, NULL, NULL) == ML_STATUS_INVALID_ARGUMENT);
    CHECK_INVALID(matrix_subtract(NULL, NULL, NULL) == ML_STATUS_INVALID_ARGUMENT);
    CHECK_INVALID(matrix_multiply(NULL, NULL, NULL) == ML_STATUS_INVALID_ARGUMENT);
    CHECK_INVALID(matrix_scale(NULL, 1.0, NULL) == ML_STATUS_INVALID_ARGUMENT);
    CHECK_INVALID(matrix_transpose(NULL, NULL) == ML_STATUS_INVALID_ARGUMENT);

    CHECK_INVALID(matrix_dot(NULL, NULL, 1U, &dot) == ML_STATUS_INVALID_ARGUMENT && dot == 123.0);
    CHECK_INVALID(matrix_dot(NULL, NULL, 0U, &dot) == ML_STATUS_INVALID_ARGUMENT && dot == 123.0);
    CHECK_INVALID(matrix_dot(NULL, NULL, 1U, NULL) == ML_STATUS_INVALID_ARGUMENT);
#undef CHECK_INVALID
    return 1;
}

int test_matrix(void)
{
    MLMatrix *a = NULL;
    MLMatrix *b = NULL;
    MLMatrix *result = NULL;
    MLMatrix *transposed = NULL;
    const double a_values[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const double b_values[] = {7.0, 8.0, 9.0, 10.0, 11.0, 12.0};
    const double sum[] = {8.0, 10.0, 12.0, 14.0, 16.0, 18.0};
    const double difference[] = {-6.0, -6.0, -6.0, -6.0, -6.0, -6.0};
    const double scaled[] = {2.0, 4.0, 6.0, 8.0, 10.0, 12.0};
    const double transpose[] = {1.0, 4.0, 2.0, 5.0, 3.0, 6.0};
    int passed = 0;

    if (!test_invalid_arguments()) goto cleanup;
    if (matrix_create(2U, 3U, &a) != ML_STATUS_OK ||
        matrix_create(2U, 3U, &b) != ML_STATUS_OK) goto cleanup;
    if (matrix_rows(a) != 2U || matrix_cols(a) != 3U || matrix_data(a) == NULL) goto cleanup;
    for (size_t i = 0U; i < 6U; ++i) {
        matrix_data(a)[i] = a_values[i];
        matrix_data(b)[i] = b_values[i];
    }
    if (matrix_add(a, b, &result) != ML_STATUS_OK || !expect_values(result, 2U, 3U, sum, 6U)) goto cleanup;
    matrix_destroy(result); result = NULL;
    if (matrix_subtract(a, b, &result) != ML_STATUS_OK || !expect_values(result, 2U, 3U, difference, 6U)) goto cleanup;
    matrix_destroy(result); result = NULL;
    if (matrix_scale(a, 2.0, &result) != ML_STATUS_OK || !expect_values(result, 2U, 3U, scaled, 6U)) goto cleanup;
    matrix_destroy(result); result = NULL;
    if (matrix_scale(a, 0.0, &result) != ML_STATUS_OK) goto cleanup;
    for (size_t i = 0U; i < 6U; ++i) if (matrix_const_data(result)[i] != 0.0) goto cleanup;
    matrix_destroy(result); result = NULL;
    if (matrix_transpose(a, &transposed) != ML_STATUS_OK ||
        !expect_values(transposed, 3U, 2U, transpose, 6U)) goto cleanup;

    {
        MLMatrix *right = NULL;
        const double right_values[] = {7.0, 8.0, 9.0, 10.0, 11.0, 12.0};
        const double product[] = {58.0, 64.0, 139.0, 154.0};
        if (matrix_create(3U, 2U, &right) != ML_STATUS_OK) goto cleanup;
        for (size_t i = 0U; i < 6U; ++i) matrix_data(right)[i] = right_values[i];
        if (matrix_multiply(a, right, &result) != ML_STATUS_OK ||
            !expect_values(result, 2U, 2U, product, 4U)) {
            matrix_destroy(right);
            goto cleanup;
        }
        matrix_destroy(right);
        matrix_destroy(result); result = NULL;
    }
    {
        const double x[] = {1.0, 2.0, 3.0};
        const double y[] = {4.0, 5.0, 6.0};
        double dot = 0.0;
        if (matrix_dot(x, y, 3U, &dot) != ML_STATUS_OK || dot != 32.0) goto cleanup;
    }

    if (matrix_add(a, transposed, &result) != ML_STATUS_DIMENSION_MISMATCH || result != NULL) goto cleanup;
    if (matrix_subtract(a, transposed, &result) != ML_STATUS_DIMENSION_MISMATCH || result != NULL) goto cleanup;
    if (matrix_multiply(a, b, &result) != ML_STATUS_DIMENSION_MISMATCH || result != NULL) goto cleanup;
    if (matrix_scale(a, INFINITY, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto cleanup;
    if (matrix_scale(a, NAN, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto cleanup;

    matrix_data(a)[0] = DBL_MAX;
    matrix_data(b)[0] = DBL_MAX;
    if (matrix_add(a, b, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto cleanup;
    if (matrix_subtract(a, b, &result) != ML_STATUS_OK || result == NULL) goto cleanup;
    matrix_destroy(result); result = NULL;
    if (matrix_scale(a, 2.0, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto cleanup;
    {
        const double huge[] = {DBL_MAX};
        const double two[] = {2.0};
        double dot = 321.0;
        if (matrix_dot(huge, two, 1U, &dot) != ML_STATUS_NUMERIC_ERROR || dot != 321.0) goto cleanup;
    }
    {
        const double invalid[] = {NAN};
        const double one[] = {1.0};
        double dot = 456.0;
        if (matrix_dot(invalid, one, 1U, &dot) != ML_STATUS_NUMERIC_ERROR || dot != 456.0) goto cleanup;
    }
    {
        MLMatrix *huge_left = NULL;
        MLMatrix *huge_right = NULL;
        if (matrix_create(1U, 1U, &huge_left) != ML_STATUS_OK ||
            matrix_create(1U, 1U, &huge_right) != ML_STATUS_OK) {
            matrix_destroy(huge_left);
            matrix_destroy(huge_right);
            goto cleanup;
        }
        matrix_data(huge_left)[0] = DBL_MAX;
        matrix_data(huge_right)[0] = 2.0;
        if (matrix_multiply(huge_left, huge_right, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) {
            matrix_destroy(huge_left);
            matrix_destroy(huge_right);
            goto cleanup;
        }
        matrix_destroy(huge_left);
        matrix_destroy(huge_right);
    }
    matrix_data(a)[0] = NAN;
    if (matrix_add(a, b, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto cleanup;
    {
        MLMatrix *invalid_left = NULL;
        MLMatrix *valid_right = NULL;
        if (matrix_create(1U, 1U, &invalid_left) != ML_STATUS_OK ||
            matrix_create(1U, 1U, &valid_right) != ML_STATUS_OK) {
            matrix_destroy(invalid_left);
            matrix_destroy(valid_right);
            goto cleanup;
        }
        matrix_data(invalid_left)[0] = NAN;
        matrix_data(valid_right)[0] = 1.0;
        if (matrix_multiply(invalid_left, valid_right, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) {
            matrix_destroy(invalid_left);
            matrix_destroy(valid_right);
            goto cleanup;
        }
        matrix_destroy(invalid_left);
        matrix_destroy(valid_right);
    }
    if (matrix_transpose(a, &result) != ML_STATUS_NUMERIC_ERROR || result != NULL) goto cleanup;

    passed = 1;
cleanup:
    matrix_destroy(a);
    matrix_destroy(b);
    matrix_destroy(result);
    matrix_destroy(transposed);
    return passed ? 0 : 1;
}
