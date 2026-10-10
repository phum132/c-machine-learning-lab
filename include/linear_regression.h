#ifndef LINEAR_REGRESSION_H
#define LINEAR_REGRESSION_H

#include <stddef.h>
#include <stdint.h>

#include "matrix.h"

typedef struct MLLinearRegression MLLinearRegression;

typedef struct {
    /* Full-batch gradient descent stops after `patience` consecutive updates
     * whose absolute change in training MSE is <= tolerance, or at the
     * iteration limit. This is a stability criterion, not a guarantee that
     * the loss decreased or reached a global minimum. */
    size_t max_iterations;
    size_t patience;
    double learning_rate;
    double tolerance;
} MLLinearRegressionConfig;

typedef struct {
    /* Owned by the caller after successful training; release with destroy. */
    size_t length;
    double *mse_values;
} MLTrainingHistory;

typedef struct {
    MLMatrix *train_features;
    double *train_targets;
    size_t train_count;
    MLMatrix *validation_features;
    double *validation_targets;
    size_t validation_count;
} MLLinearSplit;

MLStatus ml_linear_regression_create(size_t feature_count, MLLinearRegression **out);
void ml_linear_regression_destroy(MLLinearRegression *model);
size_t ml_linear_regression_feature_count(const MLLinearRegression *model);
const double *ml_linear_regression_weights(const MLLinearRegression *model);
double ml_linear_regression_bias(const MLLinearRegression *model);
/* Copies finite parameters into an existing model. The weight count must match
 * the model feature count; on error the model is unchanged. */
MLStatus ml_linear_regression_set_parameters(MLLinearRegression *model,
                                             const double *weights,
                                             size_t weight_count, double bias);
void ml_training_history_destroy(MLTrainingHistory *history);
void ml_linear_split_destroy(MLLinearSplit *split);

/* X is samples-by-features; targets has one finite value per row.
 * Predict/MSE borrow model and inputs. Predict returns an owned N-by-1 matrix. */
MLStatus ml_linear_regression_predict(const MLLinearRegression *model,
                                      const MLMatrix *features, MLMatrix **out);
MLStatus ml_linear_regression_mse(const MLLinearRegression *model,
                                  const MLMatrix *features, const double *targets,
                                  size_t target_count, double *out);
/* `history` must be zero-initialized or destroyed before reuse. It is cleared
 * on entry and owns its values on success; release with history_destroy. */
MLStatus ml_linear_regression_train(MLLinearRegression *model,
                                   const MLMatrix *features, const double *targets,
                                   size_t target_count,
                                   const MLLinearRegressionConfig *config,
                                   MLTrainingHistory *history);
/* Randomly partitions samples with a deterministic seed. `out` must be
 * zero-initialized or previously destroyed; successful arrays/matrices are
 * owned by it and released with ml_linear_split_destroy. */
MLStatus ml_linear_regression_split(const MLMatrix *features, const double *targets,
                                   size_t sample_count, double train_fraction,
                                   uint64_t seed, MLLinearSplit *out);

#endif
