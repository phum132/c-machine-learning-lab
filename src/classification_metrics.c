#include "classification_metrics.h"

#include <math.h>

static int binary_values_valid(const double *values, size_t count)
{
    if (values == NULL || count == 0U) return 0;
    for (size_t i = 0U; i < count; ++i)
        if (!isfinite(values[i]) || (values[i] != 0.0 && values[i] != 1.0)) return 0;
    return 1;
}

MLStatus ml_binary_classification_metrics(const double *labels,
                                          const double *predicted_classes,
                                          size_t sample_count,
                                          MLBinaryClassificationMetrics *out)
{
    MLBinaryClassificationMetrics result = {0U, 0U, 0U, 0U, 0.0, 0.0, 0.0, 0.0};
    double precision_denominator;
    double recall_denominator;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = result;
    if (!binary_values_valid(labels, sample_count) ||
        !binary_values_valid(predicted_classes, sample_count))
        return ML_STATUS_INVALID_ARGUMENT;

    for (size_t i = 0U; i < sample_count; ++i) {
        if (labels[i] == 1.0 && predicted_classes[i] == 1.0) ++result.true_positive;
        else if (labels[i] == 0.0 && predicted_classes[i] == 0.0) ++result.true_negative;
        else if (labels[i] == 0.0) ++result.false_positive;
        else ++result.false_negative;
    }

    result.accuracy = (double)(result.true_positive + result.true_negative) /
                      (double)sample_count;
    precision_denominator = (double)result.true_positive + (double)result.false_positive;
    recall_denominator = (double)result.true_positive + (double)result.false_negative;
    result.precision = precision_denominator == 0.0 ? 0.0 :
                       (double)result.true_positive / precision_denominator;
    result.recall = recall_denominator == 0.0 ? 0.0 :
                    (double)result.true_positive / recall_denominator;
    result.f1 = (result.precision + result.recall) == 0.0 ? 0.0 :
                2.0 * result.precision * result.recall /
                (result.precision + result.recall);
    *out = result;
    return ML_STATUS_OK;
}
