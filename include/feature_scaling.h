#ifndef FEATURE_SCALING_H
#define FEATURE_SCALING_H

#include <stddef.h>

#include "matrix.h"

/*
 * Per-feature standardization: x' = (x - mean) / scale. A zero-initialized
 * value (feature_count 0, NULL arrays) means "no scaling". When populated, the
 * struct owns both arrays; release them with ml_feature_scaling_destroy.
 */
typedef struct {
    size_t feature_count;
    double *means;
    double *scales;
} MLFeatureScaling;

/*
 * Fit means and population standard deviations from samples-by-features data.
 * Columns with zero variance get scale 1.0, so they map to 0. `out` must be
 * zero-initialized or destroyed; it is cleared on entry and stays empty on
 * failure.
 */
MLStatus ml_feature_scaling_fit(const MLMatrix *features, MLFeatureScaling *out);

/*
 * Copy validated statistics into `out`. Means must be finite and scales
 * finite and positive. `out` follows the same lifecycle rule as fit.
 */
MLStatus ml_feature_scaling_create(const double *means, const double *scales,
                                   size_t feature_count, MLFeatureScaling *out);

/*
 * Standardize `features` in place. Column count must equal feature_count.
 * The matrix is unchanged when an error is returned.
 */
MLStatus ml_feature_scaling_apply(const MLFeatureScaling *scaling, MLMatrix *features);

void ml_feature_scaling_destroy(MLFeatureScaling *scaling);

#endif
