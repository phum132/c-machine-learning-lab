#ifndef MATRIX_H
#define MATRIX_H

#include <stddef.h>

#include "ml_error.h"

/*
 * Opaque owning matrix with contiguous row-major double storage.
 * Dimensions are positive and immutable after creation. The caller owns a
 * successfully created matrix and must release it with matrix_destroy.
 */
typedef struct MLMatrix MLMatrix;

/*
 * Create a matrix. Its element values are initially unspecified. On success,
 * *out receives a newly owned matrix; on failure, *out is NULL. Zero
 * dimensions and unrepresentable allocation sizes return
 * ML_STATUS_INVALID_ARGUMENT.
 */
MLStatus matrix_create(size_t rows, size_t cols, MLMatrix **out);
void matrix_destroy(MLMatrix *matrix);

/*
 * Non-owning accessors. A NULL matrix returns zero dimensions or NULL data.
 * Data pointers address rows*cols row-major elements and remain valid until
 * matrix_destroy; matrix_data permits mutation, while matrix_const_data does
 * not. The caller must keep the matrix alive and must not free these pointers.
 */
size_t matrix_rows(const MLMatrix *matrix);
size_t matrix_cols(const MLMatrix *matrix);
double *matrix_data(MLMatrix *matrix);
const double *matrix_const_data(const MLMatrix *matrix);

/*
 * Inputs are borrowed and remain unchanged; results are newly allocated and
 * owned by the caller. Each function sets *out to NULL before work, including
 * on failure. Invalid matrices return ML_STATUS_INVALID_ARGUMENT, shape
 * mismatches return ML_STATUS_DIMENSION_MISMATCH, and non-finite inputs or
 * arithmetic results return ML_STATUS_NUMERIC_ERROR.
 */
MLStatus matrix_add(const MLMatrix *left, const MLMatrix *right, MLMatrix **out);
MLStatus matrix_subtract(const MLMatrix *left, const MLMatrix *right, MLMatrix **out);
MLStatus matrix_multiply(const MLMatrix *left, const MLMatrix *right, MLMatrix **out);
MLStatus matrix_scale(const MLMatrix *matrix, double scalar, MLMatrix **out);
MLStatus matrix_transpose(const MLMatrix *matrix, MLMatrix **out);

/*
 * Dot product of borrowed vectors. left and right must each contain length
 * elements, and length must be positive. Rejects non-finite inputs/results.
 * Writes *out only on success; on failure its prior value is unchanged.
 */
MLStatus matrix_dot(const double *left, const double *right, size_t length, double *out);

#endif
