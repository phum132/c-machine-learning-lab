#ifndef MATRIX_H
#define MATRIX_H

#include <stddef.h>

#include "ml_error.h"

/* Opaque owning matrix. Create with matrix_create; release with destroy. */
typedef struct MLMatrix MLMatrix;

/* On success, *out receives a newly owned matrix. On failure it is NULL. */
MLStatus matrix_create(size_t rows, size_t cols, MLMatrix **out);
void matrix_destroy(MLMatrix *matrix);

/* Non-owning accessors. Invalid arguments return 0 dimensions/NULL data.
 * The mutable data pointer is valid until matrix_destroy and has rows*cols items. */
size_t matrix_rows(const MLMatrix *matrix);
size_t matrix_cols(const MLMatrix *matrix);
double *matrix_data(MLMatrix *matrix);
const double *matrix_const_data(const MLMatrix *matrix);

/* Results are newly allocated. Inputs are borrowed and remain unchanged. */
MLStatus matrix_add(const MLMatrix *left, const MLMatrix *right, MLMatrix **out);
MLStatus matrix_subtract(const MLMatrix *left, const MLMatrix *right, MLMatrix **out);
MLStatus matrix_multiply(const MLMatrix *left, const MLMatrix *right, MLMatrix **out);
MLStatus matrix_scale(const MLMatrix *matrix, double scalar, MLMatrix **out);
MLStatus matrix_transpose(const MLMatrix *matrix, MLMatrix **out);

/* Dot product of borrowed vectors. Rejects non-finite inputs/results. */
MLStatus matrix_dot(const double *left, const double *right, size_t length, double *out);

#endif
