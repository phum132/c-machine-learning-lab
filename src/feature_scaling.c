#include "feature_scaling.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ml_numeric.h"

static MLStatus allocate_scaling(size_t feature_count, MLFeatureScaling *out)
{
    if (feature_count == 0U || feature_count > SIZE_MAX / sizeof(double))
        return ML_STATUS_INVALID_ARGUMENT;
    out->means = malloc(feature_count * sizeof(*out->means));
    out->scales = malloc(feature_count * sizeof(*out->scales));
    if (out->means == NULL || out->scales == NULL) {
        ml_feature_scaling_destroy(out);
        return ML_STATUS_OUT_OF_MEMORY;
    }
    out->feature_count = feature_count;
    return ML_STATUS_OK;
}

void ml_feature_scaling_destroy(MLFeatureScaling *scaling)
{
    if (scaling != NULL) {
        free(scaling->means);
        free(scaling->scales);
        memset(scaling, 0, sizeof(*scaling));
    }
}

MLStatus ml_feature_scaling_fit(const MLMatrix *features, MLFeatureScaling *out)
{
    size_t rows, cols;
    const double *data;
    MLStatus status;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    rows = matrix_rows(features);
    cols = matrix_cols(features);
    data = matrix_const_data(features);
    if (rows == 0U || cols == 0U || data == NULL) return ML_STATUS_INVALID_ARGUMENT;
    status = allocate_scaling(cols, out);
    if (status != ML_STATUS_OK) return status;
    for (size_t col = 0U; col < cols; ++col) {
        double sum = 0.0, squared = 0.0, mean, deviation;
        for (size_t row = 0U; row < rows; ++row) sum += data[row * cols + col];
        mean = sum / (double)rows;
        for (size_t row = 0U; row < rows; ++row) {
            const double difference = data[row * cols + col] - mean;
            squared += difference * difference;
        }
        deviation = sqrt(squared / (double)rows);
        if (!ml_is_finite(mean) || !ml_is_finite(deviation)) {
            ml_feature_scaling_destroy(out);
            return ML_STATUS_NUMERIC_ERROR;
        }
        out->means[col] = mean;
        out->scales[col] = deviation > 0.0 ? deviation : 1.0;
    }
    return ML_STATUS_OK;
}

MLStatus ml_feature_scaling_create(const double *means, const double *scales,
                                   size_t feature_count, MLFeatureScaling *out)
{
    MLStatus status;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (means == NULL || scales == NULL || feature_count == 0U) return ML_STATUS_INVALID_ARGUMENT;
    for (size_t i = 0U; i < feature_count; ++i)
        if (!ml_is_finite(means[i]) || !ml_is_finite(scales[i]) || scales[i] <= 0.0)
            return ML_STATUS_NUMERIC_ERROR;
    status = allocate_scaling(feature_count, out);
    if (status != ML_STATUS_OK) return status;
    memcpy(out->means, means, feature_count * sizeof(*means));
    memcpy(out->scales, scales, feature_count * sizeof(*scales));
    return ML_STATUS_OK;
}

MLStatus ml_feature_scaling_apply(const MLFeatureScaling *scaling, MLMatrix *features)
{
    size_t rows, cols;
    double *data;
    if (scaling == NULL || scaling->feature_count == 0U || scaling->means == NULL ||
        scaling->scales == NULL || features == NULL) return ML_STATUS_INVALID_ARGUMENT;
    rows = matrix_rows(features);
    cols = matrix_cols(features);
    data = matrix_data(features);
    if (cols != scaling->feature_count) return ML_STATUS_DIMENSION_MISMATCH;
    /* Validate every result first so a failure leaves the matrix unchanged. */
    for (size_t i = 0U; i < rows * cols; ++i)
        if (!ml_is_finite((data[i] - scaling->means[i % cols]) / scaling->scales[i % cols]))
            return ML_STATUS_NUMERIC_ERROR;
    for (size_t i = 0U; i < rows * cols; ++i)
        data[i] = (data[i] - scaling->means[i % cols]) / scaling->scales[i % cols];
    return ML_STATUS_OK;
}
