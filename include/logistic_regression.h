#ifndef LOGISTIC_REGRESSION_H
#define LOGISTIC_REGRESSION_H

#include <stddef.h>

#include "matrix.h"

typedef struct MLLogisticRegression MLLogisticRegression;

typedef struct {
    /* Stops after `patience` consecutive updates whose absolute training
     * cross-entropy change is <= tolerance, or at max_iterations. */
    size_t max_iterations;
    size_t patience;
    double learning_rate;
    double tolerance;
} MLLogisticRegressionConfig;

typedef struct {
    size_t length;
    double *cross_entropy_values;
} MLLogisticHistory;

MLStatus ml_logistic_regression_create(size_t feature_count, MLLogisticRegression **out);
void ml_logistic_regression_destroy(MLLogisticRegression *model);
size_t ml_logistic_regression_feature_count(const MLLogisticRegression *model);
const double *ml_logistic_regression_weights(const MLLogisticRegression *model);
double ml_logistic_regression_bias(const MLLogisticRegression *model);
/* Copies finite parameters into an existing model. The weight count must match
 * the model feature count; on error the model is unchanged. */
MLStatus ml_logistic_regression_set_parameters(MLLogisticRegression *model,
                                               const double *weights,
                                               size_t weight_count, double bias);
void ml_logistic_history_destroy(MLLogisticHistory *history);

/* Stable sigmoid utility; accepts finite logits and writes a probability.
 * If logit is invalid, *out is set to 0.0 before returning an error. */
MLStatus ml_sigmoid(double logit, double *out);

/* Features are samples-by-features. Labels must be exactly 0.0 or 1.0. */
MLStatus ml_logistic_regression_predict_probabilities(const MLLogisticRegression *model,
                                                       const MLMatrix *features,
                                                       MLMatrix **out);
/* Classes are 1.0 when probability >= threshold, otherwise 0.0. Threshold is [0,1]. */
MLStatus ml_logistic_regression_predict_classes(const MLLogisticRegression *model,
                                                const MLMatrix *features,
                                                double threshold, MLMatrix **out);
MLStatus ml_logistic_regression_binary_cross_entropy(const MLLogisticRegression *model,
                                                     const MLMatrix *features,
                                                     const double *labels,
                                                     size_t label_count, double *out);
/* History must be zero-initialized or destroyed before reuse; caller destroys
 * returned history with ml_logistic_history_destroy. Training rolls parameters
 * back if a numeric error occurs. */
MLStatus ml_logistic_regression_train(MLLogisticRegression *model,
                                     const MLMatrix *features, const double *labels,
                                     size_t label_count,
                                     const MLLogisticRegressionConfig *config,
                                     MLLogisticHistory *history);

#endif
