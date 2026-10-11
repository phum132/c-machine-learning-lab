#include "regression_metrics.h"

#include <math.h>
#include <string.h>

#include "ml_numeric.h"

MLStatus ml_regression_metrics(const double *actual, const double *predicted,
                               size_t sample_count, MLRegressionMetrics *out)
{
    double absolute = 0.0, squared = 0.0, total = 0.0, sum = 0.0, mean;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (actual == NULL || predicted == NULL || sample_count == 0U) return ML_STATUS_INVALID_ARGUMENT;
    for (size_t i = 0U; i < sample_count; ++i) {
        if (!ml_is_finite(actual[i]) || !ml_is_finite(predicted[i])) return ML_STATUS_NUMERIC_ERROR;
        sum += actual[i];
    }
    mean = sum / (double)sample_count;
    for (size_t i = 0U; i < sample_count; ++i) {
        const double error = actual[i] - predicted[i];
        const double centered = actual[i] - mean;
        absolute += fabs(error);
        squared += error * error;
        total += centered * centered;
    }
    if (!ml_is_finite(mean) || !ml_is_finite(absolute) || !ml_is_finite(squared) ||
        !ml_is_finite(total)) return ML_STATUS_NUMERIC_ERROR;
    out->mae = absolute / (double)sample_count;
    out->rmse = sqrt(squared / (double)sample_count);
    if (total > 0.0) {
        out->r2 = 1.0 - squared / total;
        out->has_r2 = 1;
    }
    return ML_STATUS_OK;
}
