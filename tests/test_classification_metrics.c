#include "classification_metrics.h"

#include <math.h>
#include <stddef.h>

int test_classification_metrics(void)
{
    const double labels[] = {1.0, 0.0, 1.0, 0.0};
    const double predicted[] = {1.0, 0.0, 0.0, 0.0};
    const double all_negative[] = {0.0, 0.0};
    const double all_negative_predictions[] = {0.0, 0.0};
    const double invalid[] = {0.0, 0.5};
    const double non_finite[] = {1.0, NAN};
    MLBinaryClassificationMetrics metrics;

    if (ml_binary_classification_metrics(labels, predicted, 4U, &metrics) != ML_STATUS_OK ||
        metrics.true_positive != 1U || metrics.true_negative != 2U ||
        metrics.false_positive != 0U || metrics.false_negative != 1U ||
        fabs(metrics.accuracy - 0.75) > 1e-15 ||
        metrics.precision != 1.0 || metrics.recall != 0.5 ||
        fabs(metrics.f1 - (2.0 / 3.0)) > 1e-15) return 1;

    if (ml_binary_classification_metrics(all_negative, all_negative_predictions, 2U,
                                         &metrics) != ML_STATUS_OK ||
        metrics.accuracy != 1.0 || metrics.precision != 0.0 ||
        metrics.recall != 0.0 || metrics.f1 != 0.0) return 1;

    metrics.accuracy = -1.0;
    if (ml_binary_classification_metrics(all_negative, all_negative_predictions, 0U,
                                         &metrics) != ML_STATUS_INVALID_ARGUMENT ||
        metrics.accuracy != 0.0 || metrics.precision != 0.0 ||
        ml_binary_classification_metrics(invalid, all_negative_predictions, 2U,
                                         &metrics) != ML_STATUS_INVALID_ARGUMENT ||
        metrics.true_positive != 0U || metrics.accuracy != 0.0 ||
        ml_binary_classification_metrics(non_finite, all_negative_predictions, 2U,
                                         &metrics) != ML_STATUS_INVALID_ARGUMENT ||
        metrics.accuracy != 0.0 ||
        ml_binary_classification_metrics(labels, predicted, 4U, NULL) !=
        ML_STATUS_INVALID_ARGUMENT) return 1;
    return 0;
}
