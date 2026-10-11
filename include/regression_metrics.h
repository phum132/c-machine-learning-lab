#ifndef REGRESSION_METRICS_H
#define REGRESSION_METRICS_H

#include <stddef.h>

#include "ml_error.h"

typedef struct {
    double mae;
    double rmse;
    /* Coefficient of determination. Undefined when the actual values have
     * zero variance; then has_r2 is 0 and r2 is 0. */
    double r2;
    int has_r2;
} MLRegressionMetrics;

/* Inputs are borrowed arrays of finite values. Empty input is invalid. `out`
 * is zeroed on entry and populated only on success. */
MLStatus ml_regression_metrics(const double *actual, const double *predicted,
                               size_t sample_count, MLRegressionMetrics *out);

#endif
