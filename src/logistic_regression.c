#include "logistic_regression.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ml_numeric.h"

struct MLLogisticRegression {
    size_t feature_count;
    double *weights;
    double bias;
};

static int features_valid(const MLMatrix *features, size_t feature_count)
{
    const double *data;
    size_t count;
    if (features == NULL || matrix_rows(features) == 0U ||
        matrix_cols(features) != feature_count || matrix_const_data(features) == NULL)
        return 0;
    data = matrix_const_data(features);
    count = matrix_rows(features) * matrix_cols(features);
    for (size_t i = 0U; i < count; ++i)
        if (!ml_is_finite(data[i])) return 0;
    return 1;
}

static int labels_valid(const double *labels, size_t count)
{
    if (labels == NULL || count == 0U) return 0;
    for (size_t i = 0U; i < count; ++i)
        if (!ml_is_finite(labels[i]) || (labels[i] != 0.0 && labels[i] != 1.0)) return 0;
    return 1;
}

static double sigmoid_value(double logit)
{
    if (logit >= 0.0) {
        const double exponential = exp(-logit);
        return 1.0 / (1.0 + exponential);
    }
    const double exponential = exp(logit);
    return exponential / (1.0 + exponential);
}

MLStatus ml_sigmoid(double logit, double *out)
{
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = 0.0;
    if (!ml_is_finite(logit)) return ML_STATUS_INVALID_ARGUMENT;
    *out = sigmoid_value(logit);
    return ml_is_finite(*out) ? ML_STATUS_OK : ML_STATUS_NUMERIC_ERROR;
}

MLStatus ml_logistic_regression_create(size_t feature_count, MLLogisticRegression **out)
{
    MLLogisticRegression *model;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (feature_count == 0U || feature_count > SIZE_MAX / sizeof(double))
        return ML_STATUS_INVALID_ARGUMENT;
    model = malloc(sizeof(*model));
    if (model == NULL) return ML_STATUS_OUT_OF_MEMORY;
    model->weights = calloc(feature_count, sizeof(*model->weights));
    if (model->weights == NULL) { free(model); return ML_STATUS_OUT_OF_MEMORY; }
    model->feature_count = feature_count;
    model->bias = 0.0;
    *out = model;
    return ML_STATUS_OK;
}

void ml_logistic_regression_destroy(MLLogisticRegression *model)
{
    if (model == NULL) return;
    free(model->weights);
    free(model);
}

size_t ml_logistic_regression_feature_count(const MLLogisticRegression *model)
{ return model == NULL ? 0U : model->feature_count; }

const double *ml_logistic_regression_weights(const MLLogisticRegression *model)
{ return model == NULL ? NULL : model->weights; }

double ml_logistic_regression_bias(const MLLogisticRegression *model)
{ return model == NULL ? 0.0 : model->bias; }

MLStatus ml_logistic_regression_set_parameters(MLLogisticRegression *model,
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

void ml_logistic_history_destroy(MLLogisticHistory *history)
{
    if (history == NULL) return;
    free(history->cross_entropy_values);
    history->cross_entropy_values = NULL;
    history->length = 0U;
}

static MLStatus predict_logit(const MLLogisticRegression *model, const double *row,
                              double *out)
{
    double value = model->bias;
    for (size_t col = 0U; col < model->feature_count; ++col) {
        value += model->weights[col] * row[col];
        if (!ml_is_finite(value)) return ML_STATUS_NUMERIC_ERROR;
    }
    *out = value;
    return ML_STATUS_OK;
}

static MLStatus predict_probabilities(const MLLogisticRegression *model,
                                      const MLMatrix *features, MLMatrix **out)
{
    MLStatus status;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (model == NULL || !features_valid(features, model->feature_count))
        return ML_STATUS_INVALID_ARGUMENT;
    status = matrix_create(matrix_rows(features), 1U, out);
    if (status != ML_STATUS_OK) return status;
    for (size_t row = 0U; row < matrix_rows(features); ++row) {
        double logit;
        status = predict_logit(model,
                               matrix_const_data(features) + row * model->feature_count,
                               &logit);
        if (status != ML_STATUS_OK) goto error;
        matrix_data(*out)[row] = sigmoid_value(logit);
        if (!ml_is_finite(matrix_data(*out)[row])) { status = ML_STATUS_NUMERIC_ERROR; goto error; }
    }
    return ML_STATUS_OK;

error:
    matrix_destroy(*out);
    *out = NULL;
    return status;
}

MLStatus ml_logistic_regression_predict_probabilities(const MLLogisticRegression *model,
                                                      const MLMatrix *features,
                                                      MLMatrix **out)
{
    return predict_probabilities(model, features, out);
}

MLStatus ml_logistic_regression_predict_classes(const MLLogisticRegression *model,
                                                 const MLMatrix *features,
                                                 double threshold, MLMatrix **out)
{
    MLMatrix *probabilities = NULL;
    MLStatus status;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (!ml_is_finite(threshold) || threshold < 0.0 || threshold > 1.0)
        return ML_STATUS_INVALID_ARGUMENT;
    status = predict_probabilities(model, features, &probabilities);
    if (status != ML_STATUS_OK) return status;
    status = matrix_create(matrix_rows(probabilities), 1U, out);
    if (status != ML_STATUS_OK) { matrix_destroy(probabilities); return status; }
    for (size_t row = 0U; row < matrix_rows(probabilities); ++row)
        matrix_data(*out)[row] = matrix_const_data(probabilities)[row] >= threshold ? 1.0 : 0.0;
    matrix_destroy(probabilities);
    return ML_STATUS_OK;
}

MLStatus ml_logistic_regression_binary_cross_entropy(const MLLogisticRegression *model,
                                                      const MLMatrix *features,
                                                      const double *labels,
                                                      size_t label_count, double *out)
{
    double total = 0.0;
    if (out == NULL || model == NULL || !features_valid(features, model->feature_count) ||
        matrix_rows(features) != label_count || !labels_valid(labels, label_count))
        return ML_STATUS_INVALID_ARGUMENT;
    for (size_t row = 0U; row < label_count; ++row) {
        double logit;
        double loss;
        MLStatus status = predict_logit(model,
                                        matrix_const_data(features) + row * model->feature_count,
                                        &logit);
        if (status != ML_STATUS_OK) return status;
        /* log(1 + exp(logit)) - label*logit, expressed without overflow. */
        loss = fmax(logit, 0.0) - logit * labels[row] + log1p(exp(-fabs(logit)));
        total += loss;
        if (!ml_is_finite(loss) || !ml_is_finite(total)) return ML_STATUS_NUMERIC_ERROR;
    }
    *out = total / (double)label_count;
    return ml_is_finite(*out) ? ML_STATUS_OK : ML_STATUS_NUMERIC_ERROR;
}

MLStatus ml_logistic_regression_train(MLLogisticRegression *model,
                                      const MLMatrix *features, const double *labels,
                                      size_t label_count,
                                      const MLLogisticRegressionConfig *config,
                                      MLLogisticHistory *history)
{
    double *weights_before;
    double *gradient;
    double *losses;
    double bias_before;
    size_t history_length = 0U;
    size_t stable_count = 0U;
    MLStatus status = ML_STATUS_OK;

    if (history != NULL) { history->length = 0U; history->cross_entropy_values = NULL; }
    if (model == NULL || config == NULL || !features_valid(features, model->feature_count) ||
        matrix_rows(features) != label_count || !labels_valid(labels, label_count) ||
        config->max_iterations == 0U || config->patience == 0U ||
        !ml_is_finite(config->learning_rate) || config->learning_rate <= 0.0 ||
        !ml_is_finite(config->tolerance) || config->tolerance < 0.0 ||
        !ml_is_finite(config->l2) || config->l2 < 0.0 ||
        config->max_iterations == SIZE_MAX ||
        config->max_iterations + 1U > SIZE_MAX / sizeof(double))
        return ML_STATUS_INVALID_ARGUMENT;

    weights_before = malloc(model->feature_count * sizeof(*weights_before));
    gradient = malloc(model->feature_count * sizeof(*gradient));
    losses = malloc((config->max_iterations + 1U) * sizeof(*losses));
    if (weights_before == NULL || gradient == NULL || losses == NULL) {
        free(weights_before); free(gradient); free(losses);
        return ML_STATUS_OUT_OF_MEMORY;
    }
    memcpy(weights_before, model->weights, model->feature_count * sizeof(*weights_before));
    bias_before = model->bias;

    /* Inputs were validated once above. Each pass reads the data once to get
     * the loss of the current parameters and (unless final) the gradient.
     * losses[k] is the loss after k updates. */
    for (size_t iteration = 0U;; ++iteration) {
        const int final_pass = iteration == config->max_iterations;
        const size_t d = model->feature_count;
        const double *x = matrix_const_data(features);
        double total = 0.0, bias_gradient = 0.0, penalty = 0.0;
        if (!final_pass) memset(gradient, 0, d * sizeof(*gradient));
        for (size_t row = 0U; row < label_count; ++row) {
            const double *xr = x + row * d;
            double logit = model->bias;
            for (size_t col = 0U; col < d; ++col) logit += model->weights[col] * xr[col];
            /* log(1 + exp(logit)) - label*logit, expressed without overflow. */
            total += fmax(logit, 0.0) - logit * labels[row] + log1p(exp(-fabs(logit)));
            if (!final_pass) {
                const double residual = sigmoid_value(logit) - labels[row];
                bias_gradient += residual;
                for (size_t col = 0U; col < d; ++col) gradient[col] += residual * xr[col];
            }
        }
        for (size_t col = 0U; col < d; ++col) penalty += model->weights[col] * model->weights[col];
        const double loss = total / (double)label_count + config->l2 * penalty;
        if (!ml_is_finite(loss)) { status = ML_STATUS_NUMERIC_ERROR; goto rollback; }
        losses[history_length++] = loss;
        if (history_length >= 2U) {
            if (fabs(losses[history_length - 2U] - loss) <= config->tolerance) {
                if (++stable_count >= config->patience) break;
            } else {
                stable_count = 0U;
            }
        }
        if (final_pass) break;

        bias_gradient /= (double)label_count;
        if (!ml_is_finite(bias_gradient)) { status = ML_STATUS_NUMERIC_ERROR; goto rollback; }
        for (size_t col = 0U; col < d; ++col) {
            const double g = gradient[col] / (double)label_count + 2.0 * config->l2 * model->weights[col];
            const double updated = model->weights[col] - config->learning_rate * g;
            if (!ml_is_finite(g) || !ml_is_finite(updated)) {
                status = ML_STATUS_NUMERIC_ERROR;
                goto rollback;
            }
            model->weights[col] = updated;
        }
        model->bias -= config->learning_rate * bias_gradient;
        if (!ml_is_finite(model->bias)) { status = ML_STATUS_NUMERIC_ERROR; goto rollback; }
    }

    free(weights_before);
    free(gradient);
    if (history != NULL) {
        history->cross_entropy_values = losses;
        history->length = history_length;
    } else {
        free(losses);
    }
    return ML_STATUS_OK;

rollback:
    memcpy(model->weights, weights_before, model->feature_count * sizeof(*weights_before));
    model->bias = bias_before;
    free(weights_before);
    free(gradient);
    free(losses);
    return status;
}
