#ifndef CLASSIFICATION_METRICS_H
#define CLASSIFICATION_METRICS_H

#include <stddef.h>

#include "ml_error.h"

typedef struct {
    size_t true_positive;
    size_t true_negative;
    size_t false_positive;
    size_t false_negative;
    double accuracy;
    double precision;
    double recall;
    double f1;
} MLBinaryClassificationMetrics;

/* Inputs are borrowed arrays of exact binary values. Empty input is invalid.
 * Precision/recall/F1 are 0 when their denominator is 0. `out` is zeroed on
 * entry and populated only on success. */
MLStatus ml_binary_classification_metrics(const double *labels,
                                          const double *predicted_classes,
                                          size_t sample_count,
                                          MLBinaryClassificationMetrics *out);

#endif
