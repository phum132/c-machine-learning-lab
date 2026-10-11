#include "matrix.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "ml_numeric.h"

struct MLMatrix {
    size_t rows;
    size_t cols;
    double *data;
};

static int matrix_is_valid(const MLMatrix *matrix)
{
    if (matrix == NULL || matrix->rows == 0U || matrix->cols == 0U || matrix->data == NULL ||
        matrix->rows > SIZE_MAX / matrix->cols) {
        return 0;
    }
    return matrix->rows * matrix->cols <= SIZE_MAX / sizeof(*matrix->data);
}

static int matrix_has_finite_data(const MLMatrix *matrix)
{
    size_t count = matrix->rows * matrix->cols;
    for (size_t index = 0U; index < count; ++index) {
        if (!ml_is_finite(matrix->data[index])) {
            return 0;
        }
    }
    return 1;
}

MLStatus matrix_create(size_t rows, size_t cols, MLMatrix **out)
{
    if (out == NULL) {
        return ML_STATUS_INVALID_ARGUMENT;
    }
    *out = NULL;
    if (rows == 0U || cols == 0U || rows > SIZE_MAX / cols ||
        rows * cols > SIZE_MAX / sizeof(double)) {
        return ML_STATUS_INVALID_ARGUMENT;
    }
    MLMatrix *matrix = malloc(sizeof(*matrix));
    if (matrix == NULL) {
        return ML_STATUS_OUT_OF_MEMORY;
    }
    matrix->data = malloc(rows * cols * sizeof(*matrix->data));
    if (matrix->data == NULL) {
        free(matrix);
        return ML_STATUS_OUT_OF_MEMORY;
    }
    matrix->rows = rows;
    matrix->cols = cols;
    *out = matrix;
    return ML_STATUS_OK;
}

void matrix_destroy(MLMatrix *matrix)
{
    if (matrix != NULL) {
        free(matrix->data);
        free(matrix);
    }
}

size_t matrix_rows(const MLMatrix *matrix) { return matrix == NULL ? 0U : matrix->rows; }
size_t matrix_cols(const MLMatrix *matrix) { return matrix == NULL ? 0U : matrix->cols; }
double *matrix_data(MLMatrix *matrix) { return matrix == NULL ? NULL : matrix->data; }
const double *matrix_const_data(const MLMatrix *matrix) { return matrix == NULL ? NULL : matrix->data; }

static MLStatus matrix_elementwise(const MLMatrix *left, const MLMatrix *right,
                                   int subtract, MLMatrix **out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (!matrix_is_valid(left) || !matrix_is_valid(right)) return ML_STATUS_INVALID_ARGUMENT;
    if (left->rows != right->rows || left->cols != right->cols) return ML_STATUS_DIMENSION_MISMATCH;
    if (!matrix_has_finite_data(left) || !matrix_has_finite_data(right)) return ML_STATUS_NUMERIC_ERROR;
    MLStatus status = matrix_create(left->rows, left->cols, out);
    if (status != ML_STATUS_OK) return status;
    size_t count = left->rows * left->cols;
    for (size_t i = 0U; i < count; ++i) {
        double value = subtract ? left->data[i] - right->data[i] : left->data[i] + right->data[i];
        if (!ml_is_finite(value)) { matrix_destroy(*out); *out = NULL; return ML_STATUS_NUMERIC_ERROR; }
        (*out)->data[i] = value;
    }
    return ML_STATUS_OK;
}

MLStatus matrix_add(const MLMatrix *left, const MLMatrix *right, MLMatrix **out)
{ return matrix_elementwise(left, right, 0, out); }

MLStatus matrix_subtract(const MLMatrix *left, const MLMatrix *right, MLMatrix **out)
{ return matrix_elementwise(left, right, 1, out); }

MLStatus matrix_multiply(const MLMatrix *left, const MLMatrix *right, MLMatrix **out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (!matrix_is_valid(left) || !matrix_is_valid(right)) return ML_STATUS_INVALID_ARGUMENT;
    if (left->cols != right->rows) return ML_STATUS_DIMENSION_MISMATCH;
    if (!matrix_has_finite_data(left) || !matrix_has_finite_data(right)) return ML_STATUS_NUMERIC_ERROR;
    MLStatus status = matrix_create(left->rows, right->cols, out);
    if (status != ML_STATUS_OK) return status;
    for (size_t row = 0U; row < left->rows; ++row) {
        for (size_t col = 0U; col < right->cols; ++col) {
            double sum = 0.0;
            for (size_t inner = 0U; inner < left->cols; ++inner) {
                sum += left->data[row * left->cols + inner] * right->data[inner * right->cols + col];
                if (!ml_is_finite(sum)) { matrix_destroy(*out); *out = NULL; return ML_STATUS_NUMERIC_ERROR; }
            }
            (*out)->data[row * right->cols + col] = sum;
        }
    }
    return ML_STATUS_OK;
}

MLStatus matrix_scale(const MLMatrix *matrix, double scalar, MLMatrix **out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (!matrix_is_valid(matrix)) return ML_STATUS_INVALID_ARGUMENT;
    if (!ml_is_finite(scalar)) return ML_STATUS_NUMERIC_ERROR;
    if (!matrix_has_finite_data(matrix)) return ML_STATUS_NUMERIC_ERROR;
    MLStatus status = matrix_create(matrix->rows, matrix->cols, out);
    if (status != ML_STATUS_OK) return status;
    size_t count = matrix->rows * matrix->cols;
    for (size_t i = 0U; i < count; ++i) {
        (*out)->data[i] = matrix->data[i] * scalar;
        if (!ml_is_finite((*out)->data[i])) { matrix_destroy(*out); *out = NULL; return ML_STATUS_NUMERIC_ERROR; }
    }
    return ML_STATUS_OK;
}

MLStatus matrix_transpose(const MLMatrix *matrix, MLMatrix **out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (!matrix_is_valid(matrix)) return ML_STATUS_INVALID_ARGUMENT;
    if (!matrix_has_finite_data(matrix)) return ML_STATUS_NUMERIC_ERROR;
    MLStatus status = matrix_create(matrix->cols, matrix->rows, out);
    if (status != ML_STATUS_OK) return status;
    for (size_t row = 0U; row < matrix->rows; ++row)
        for (size_t col = 0U; col < matrix->cols; ++col)
            (*out)->data[col * matrix->rows + row] = matrix->data[row * matrix->cols + col];
    return ML_STATUS_OK;
}

MLStatus matrix_dot(const double *left, const double *right, size_t length, double *out)
{
    if (left == NULL || right == NULL || out == NULL || length == 0U) return ML_STATUS_INVALID_ARGUMENT;
    double sum = 0.0;
    for (size_t i = 0U; i < length; ++i) {
        if (!ml_is_finite(left[i]) || !ml_is_finite(right[i])) return ML_STATUS_NUMERIC_ERROR;
        sum += left[i] * right[i];
        if (!ml_is_finite(sum)) return ML_STATUS_NUMERIC_ERROR;
    }
    *out = sum;
    return ML_STATUS_OK;
}
