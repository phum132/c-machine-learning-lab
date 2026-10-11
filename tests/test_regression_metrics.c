#include "regression_metrics.h"

#include <math.h>
#include <stddef.h>

int test_regression_metrics(void)
{
    /* Errors are 0, -1, 2, -1: MAE 1, MSE 6/4, SS_tot of 1..4 is 5. */
    const double actual[] = {1.0, 2.0, 3.0, 4.0};
    const double predicted[] = {1.0, 3.0, 1.0, 5.0};
    const double constant[] = {2.0, 2.0};
    const double non_finite[] = {1.0, NAN, 3.0, 4.0};
    MLRegressionMetrics metrics;

    if (ml_regression_metrics(actual, predicted, 4U, &metrics) != ML_STATUS_OK ||
        metrics.mae != 1.0 || fabs(metrics.rmse - sqrt(1.5)) > 1e-15 ||
        !metrics.has_r2 || fabs(metrics.r2 - (1.0 - 6.0 / 5.0)) > 1e-15) return 1;

    if (ml_regression_metrics(actual, actual, 4U, &metrics) != ML_STATUS_OK ||
        metrics.mae != 0.0 || metrics.rmse != 0.0 || metrics.r2 != 1.0) return 1;

    /* Zero-variance targets leave R2 undefined instead of dividing by zero. */
    if (ml_regression_metrics(constant, actual, 2U, &metrics) != ML_STATUS_OK ||
        metrics.has_r2 || metrics.r2 != 0.0 || fabs(metrics.mae - 0.5) > 1e-15) return 1;

    metrics.mae = -1.0;
    if (ml_regression_metrics(actual, predicted, 0U, &metrics) != ML_STATUS_INVALID_ARGUMENT ||
        metrics.mae != 0.0 ||
        ml_regression_metrics(NULL, predicted, 4U, &metrics) != ML_STATUS_INVALID_ARGUMENT ||
        ml_regression_metrics(actual, predicted, 4U, NULL) != ML_STATUS_INVALID_ARGUMENT ||
        ml_regression_metrics(non_finite, predicted, 4U, &metrics) != ML_STATUS_NUMERIC_ERROR ||
        metrics.has_r2 || metrics.rmse != 0.0) return 1;
    return 0;
}
