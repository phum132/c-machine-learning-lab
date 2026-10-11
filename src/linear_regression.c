#include "linear_regression.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ml_numeric.h"

struct MLLinearRegression {
    size_t feature_count;
    double *weights;
    double bias;
};

static int features_valid(const MLMatrix *features, size_t feature_count)
{
    if (features == NULL || matrix_rows(features) == 0U || matrix_cols(features) != feature_count ||
        matrix_const_data(features) == NULL) return 0;
    size_t count = matrix_rows(features) * matrix_cols(features);
    for (size_t i = 0U; i < count; ++i)
        if (!ml_is_finite(matrix_const_data(features)[i])) return 0;
    return 1;
}

static int targets_valid(const double *targets, size_t count)
{
    if (targets == NULL || count == 0U) return 0;
    for (size_t i = 0U; i < count; ++i)
        if (!ml_is_finite(targets[i])) return 0;
    return 1;
}

MLStatus ml_linear_regression_create(size_t feature_count, MLLinearRegression **out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (feature_count == 0U || feature_count > SIZE_MAX / sizeof(double)) return ML_STATUS_INVALID_ARGUMENT;
    MLLinearRegression *model = malloc(sizeof(*model));
    if (model == NULL) return ML_STATUS_OUT_OF_MEMORY;
    model->weights = calloc(feature_count, sizeof(*model->weights));
    if (model->weights == NULL) { free(model); return ML_STATUS_OUT_OF_MEMORY; }
    model->feature_count = feature_count;
    model->bias = 0.0;
    *out = model;
    return ML_STATUS_OK;
}

void ml_linear_regression_destroy(MLLinearRegression *model)
{
    if (model != NULL) { free(model->weights); free(model); }
}

size_t ml_linear_regression_feature_count(const MLLinearRegression *model)
{ return model == NULL ? 0U : model->feature_count; }

const double *ml_linear_regression_weights(const MLLinearRegression *model)
{ return model == NULL ? NULL : model->weights; }

double ml_linear_regression_bias(const MLLinearRegression *model)
{ return model == NULL ? 0.0 : model->bias; }

MLStatus ml_linear_regression_set_parameters(MLLinearRegression *model,
                                             const double *weights,
                                             size_t weight_count, double bias)
{
    if (model == NULL || weights == NULL || weight_count != model->feature_count)
        return ML_STATUS_INVALID_ARGUMENT;
    if (!ml_is_finite(bias)) return ML_STATUS_NUMERIC_ERROR;
    for (size_t i = 0U; i < weight_count; ++i)
        if (!ml_is_finite(weights[i])) return ML_STATUS_NUMERIC_ERROR;
    memmove(model->weights, weights, weight_count * sizeof(*weights));
    model->bias = bias;
    return ML_STATUS_OK;
}

void ml_training_history_destroy(MLTrainingHistory *history)
{
    if (history != NULL) { free(history->mse_values); history->mse_values = NULL; history->length = 0U; }
}

void ml_linear_split_destroy(MLLinearSplit *split)
{
    if (split != NULL) {
        matrix_destroy(split->train_features); free(split->train_targets);
        matrix_destroy(split->validation_features); free(split->validation_targets);
        memset(split, 0, sizeof(*split));
    }
}

static MLStatus predict_value(const MLLinearRegression *model, const double *row, double *out)
{
    double value = model->bias;
    for (size_t col = 0U; col < model->feature_count; ++col) {
        value += model->weights[col] * row[col];
        if (!ml_is_finite(value)) return ML_STATUS_NUMERIC_ERROR;
    }
    *out = value;
    return ML_STATUS_OK;
}

MLStatus ml_linear_regression_predict(const MLLinearRegression *model,
                                      const MLMatrix *features, MLMatrix **out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (model == NULL || !features_valid(features, model->feature_count)) return ML_STATUS_INVALID_ARGUMENT;
    MLStatus status = matrix_create(matrix_rows(features), 1U, out);
    if (status != ML_STATUS_OK) return status;
    for (size_t row = 0U; row < matrix_rows(features); ++row) {
        status = predict_value(model, matrix_const_data(features) + row * model->feature_count,
                               matrix_data(*out) + row);
        if (status != ML_STATUS_OK) { matrix_destroy(*out); *out = NULL; return status; }
    }
    return ML_STATUS_OK;
}

MLStatus ml_linear_regression_mse(const MLLinearRegression *model,
                                  const MLMatrix *features, const double *targets,
                                  size_t target_count, double *out)
{
    if (out == NULL || model == NULL || !features_valid(features, model->feature_count) ||
        matrix_rows(features) != target_count || !targets_valid(targets, target_count))
        return ML_STATUS_INVALID_ARGUMENT;
    double sum = 0.0;
    for (size_t row = 0U; row < target_count; ++row) {
        double prediction = 0.0;
        MLStatus status = predict_value(model, matrix_const_data(features) + row * model->feature_count,
                                        &prediction);
        if (status != ML_STATUS_OK) return status;
        double error = prediction - targets[row];
        sum += error * error;
        if (!ml_is_finite(sum)) return ML_STATUS_NUMERIC_ERROR;
    }
    *out = sum / (double)target_count;
    return ml_is_finite(*out) ? ML_STATUS_OK : ML_STATUS_NUMERIC_ERROR;
}

MLStatus ml_linear_regression_train(MLLinearRegression *model,
                                   const MLMatrix *features, const double *targets,
                                   size_t target_count,
                                   const MLLinearRegressionConfig *config,
                                   MLTrainingHistory *history)
{
    if (history != NULL) { history->length = 0U; history->mse_values = NULL; }
    if (model == NULL || config == NULL || !features_valid(features, model->feature_count) ||
        matrix_rows(features) != target_count || !targets_valid(targets, target_count) ||
        config->max_iterations == 0U || config->patience == 0U ||
        !ml_is_finite(config->learning_rate) || config->learning_rate <= 0.0 ||
        !ml_is_finite(config->tolerance) || config->tolerance < 0.0 ||
        !ml_is_finite(config->l2) || config->l2 < 0.0 ||
        config->max_iterations == SIZE_MAX || config->max_iterations + 1U > SIZE_MAX / sizeof(double))
        return ML_STATUS_INVALID_ARGUMENT;

    double *weights_before = malloc(model->feature_count * sizeof(*weights_before));
    double *gradient = malloc(model->feature_count * sizeof(*gradient));
    double *losses = malloc((config->max_iterations + 1U) * sizeof(*losses));
    if (weights_before == NULL || gradient == NULL || losses == NULL) {
        free(weights_before); free(gradient); free(losses); return ML_STATUS_OUT_OF_MEMORY;
    }
    memcpy(weights_before, model->weights, model->feature_count * sizeof(*weights_before));
    double bias_before = model->bias;
    size_t history_length = 0U, stable_count = 0U;
    MLStatus status = ML_STATUS_OK;
    const double *x = matrix_const_data(features);
    const size_t d = model->feature_count;

    /* Inputs were validated once above. Each pass computes the loss of the
     * current parameters and, unless it is the final pass, the gradient, so
     * every iteration reads the data exactly once. losses[k] is the loss after
     * k updates, matching the documented history. */
    for (size_t iteration = 0U;; ++iteration) {
        const int final_pass = iteration == config->max_iterations;
        double squared = 0.0, bias_gradient = 0.0, penalty = 0.0;
        if (!final_pass) memset(gradient, 0, d * sizeof(*gradient));
        for (size_t row = 0U; row < target_count; ++row) {
            const double *xr = x + row * d;
            double prediction = model->bias;
            for (size_t col = 0U; col < d; ++col) prediction += model->weights[col] * xr[col];
            const double error = prediction - targets[row];
            squared += error * error;
            if (!final_pass) {
                bias_gradient += error;
                for (size_t col = 0U; col < d; ++col) gradient[col] += error * xr[col];
            }
        }
        for (size_t col = 0U; col < d; ++col) penalty += model->weights[col] * model->weights[col];
        const double loss = squared / (double)target_count + config->l2 * penalty;
        /* Overflow anywhere in the pass leaves Inf/NaN in the sums, so one check suffices. */
        if (!ml_is_finite(loss)) { status = ML_STATUS_NUMERIC_ERROR; goto rollback; }
        losses[history_length++] = loss;
        if (history_length >= 2U) {
            if (fabs(losses[history_length - 2U] - loss) <= config->tolerance) {
                if (++stable_count >= config->patience) break;
            } else stable_count = 0U;
        }
        if (final_pass) break;

        const double factor = 2.0 / (double)target_count;
        bias_gradient *= factor;
        if (!ml_is_finite(bias_gradient)) { status = ML_STATUS_NUMERIC_ERROR; goto rollback; }
        for (size_t col = 0U; col < d; ++col) {
            const double g = gradient[col] * factor + 2.0 * config->l2 * model->weights[col];
            const double updated = model->weights[col] - config->learning_rate * g;
            if (!ml_is_finite(g) || !ml_is_finite(updated)) { status = ML_STATUS_NUMERIC_ERROR; goto rollback; }
            model->weights[col] = updated;
        }
        model->bias -= config->learning_rate * bias_gradient;
        if (!ml_is_finite(model->bias)) { status = ML_STATUS_NUMERIC_ERROR; goto rollback; }
    }
    free(weights_before); free(gradient);
    if (history != NULL) { history->mse_values = losses; history->length = history_length; }
    else free(losses);
    return ML_STATUS_OK;

rollback:
    memcpy(model->weights, weights_before, model->feature_count * sizeof(*weights_before));
    model->bias = bias_before;
    free(weights_before); free(gradient); free(losses);
    return status;
}

static uint64_t next_random(uint64_t *state)
{
    uint64_t value = *state;
    value ^= value >> 12U; value ^= value << 25U; value ^= value >> 27U;
    *state = value;
    return value * UINT64_C(2685821657736338717);
}

MLStatus ml_linear_regression_split(const MLMatrix *features, const double *targets,
                                   size_t sample_count, double train_fraction,
                                   uint64_t seed, MLLinearSplit *out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    if (features == NULL || sample_count < 2U || matrix_rows(features) != sample_count ||
        matrix_cols(features) == 0U || !targets_valid(targets, sample_count) ||
        !ml_is_finite(train_fraction) || train_fraction <= 0.0 || train_fraction >= 1.0)
        return ML_STATUS_INVALID_ARGUMENT;
    const size_t train_count = (size_t)((double)sample_count * train_fraction);
    if (train_count == 0U || train_count >= sample_count ||
        sample_count > SIZE_MAX / sizeof(size_t) ||
        train_count > SIZE_MAX / sizeof(double) ||
        sample_count - train_count > SIZE_MAX / sizeof(double))
        return ML_STATUS_INVALID_ARGUMENT;
    for (size_t i = 0U; i < sample_count * matrix_cols(features); ++i)
        if (!ml_is_finite(matrix_const_data(features)[i])) return ML_STATUS_NUMERIC_ERROR;
    size_t *indices = malloc(sample_count * sizeof(*indices));
    if (indices == NULL) return ML_STATUS_OUT_OF_MEMORY;
    for (size_t i = 0U; i < sample_count; ++i) indices[i] = i;
    uint64_t state = seed == 0U ? UINT64_C(0x9e3779b97f4a7c15) : seed;
    for (size_t i = sample_count - 1U; i > 0U; --i) {
        size_t j = (size_t)(next_random(&state) % (uint64_t)(i + 1U));
        size_t temporary = indices[i]; indices[i] = indices[j]; indices[j] = temporary;
    }
    MLStatus status = matrix_create(train_count, matrix_cols(features), &out->train_features);
    if (status == ML_STATUS_OK)
        status = matrix_create(sample_count - train_count, matrix_cols(features), &out->validation_features);
    if (status != ML_STATUS_OK) { free(indices); ml_linear_split_destroy(out); return status; }
    out->train_targets = malloc(train_count * sizeof(*out->train_targets));
    out->validation_targets = malloc((sample_count - train_count) * sizeof(*out->validation_targets));
    if (out->train_targets == NULL || out->validation_targets == NULL) {
        free(indices); ml_linear_split_destroy(out); return ML_STATUS_OUT_OF_MEMORY;
    }
    out->train_count = train_count; out->validation_count = sample_count - train_count;
    for (size_t destination = 0U; destination < sample_count; ++destination) {
        size_t source = indices[destination];
        MLMatrix *target_matrix = destination < train_count ? out->train_features : out->validation_features;
        size_t target_row = destination < train_count ? destination : destination - train_count;
        double *target_values = destination < train_count ? out->train_targets : out->validation_targets;
        memcpy(matrix_data(target_matrix) + target_row * matrix_cols(features),
               matrix_const_data(features) + source * matrix_cols(features),
               matrix_cols(features) * sizeof(double));
        target_values[target_row] = targets[source];
    }
    free(indices);
    return ML_STATUS_OK;
}
