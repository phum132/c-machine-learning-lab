#include "logistic_regression.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

static int finite_double(double value)
{
    return value <= DBL_MAX && value >= -DBL_MAX;
}

int test_logistic_regression(void)
{
    const double feature_values[] = {-2.0, -1.0, 1.0, 2.0};
    const double labels[] = {0.0, 0.0, 1.0, 1.0};
    const double invalid_labels[] = {0.0, 0.5, 1.0, 1.0};
    const MLLogisticRegressionConfig config = {3000U, 20U, 0.1, 1e-12};
    MLMatrix *features = NULL;
    MLMatrix *probabilities = NULL;
    MLMatrix *classes = NULL;
    MLLogisticRegression *model = NULL;
    MLLogisticHistory history = {0U, NULL};
    double loss = 0.0;
    double sigmoid_result = 0.0;
    int failed = 0;
    int completed = 0;

    if (matrix_create(4U, 1U, &features) != ML_STATUS_OK ||
        ml_logistic_regression_create(1U, &model) != ML_STATUS_OK) goto cleanup;
    for (size_t row = 0U; row < 4U; ++row) matrix_data(features)[row] = feature_values[row];

    if (ml_sigmoid(0.0, &sigmoid_result) != ML_STATUS_OK || sigmoid_result != 0.5 ||
        ml_sigmoid(1000.0, &sigmoid_result) != ML_STATUS_OK || sigmoid_result != 1.0 ||
        ml_sigmoid(-1000.0, &sigmoid_result) != ML_STATUS_OK || sigmoid_result != 0.0 ||
        ml_sigmoid(INFINITY, &sigmoid_result) != ML_STATUS_INVALID_ARGUMENT ||
        sigmoid_result != 0.0) goto cleanup;
    if (ml_logistic_regression_predict_probabilities(model, features, &probabilities) != ML_STATUS_OK ||
        matrix_rows(probabilities) != 4U || matrix_cols(probabilities) != 1U ||
        matrix_const_data(probabilities)[0] != 0.5 ||
        ml_logistic_regression_binary_cross_entropy(model, features, labels, 4U, &loss) != ML_STATUS_OK ||
        fabs(loss - log(2.0)) > 1e-15) goto cleanup;
    matrix_destroy(probabilities);
    probabilities = NULL;
    if (ml_logistic_regression_predict_classes(model, features, 0.5, &classes) != ML_STATUS_OK ||
        matrix_const_data(classes)[0] != 1.0) goto cleanup;
    matrix_destroy(classes);
    classes = NULL;
    if (ml_logistic_regression_predict_classes(model, features, 1.0, &classes) != ML_STATUS_OK ||
        matrix_const_data(classes)[0] != 0.0) goto cleanup;
    matrix_destroy(classes);
    classes = NULL;

    if (ml_logistic_regression_train(model, features, labels, 4U, &config, &history) != ML_STATUS_OK ||
        history.length < 2U || history.cross_entropy_values[history.length - 1U] >=
        history.cross_entropy_values[0U] ||
        ml_logistic_regression_predict_probabilities(model, features, &probabilities) != ML_STATUS_OK ||
        matrix_const_data(probabilities)[0] >= 0.5 || matrix_const_data(probabilities)[3] <= 0.5 ||
        ml_logistic_regression_predict_classes(model, features, 0.5, &classes) != ML_STATUS_OK ||
        matrix_const_data(classes)[0] != 0.0 || matrix_const_data(classes)[1] != 0.0 ||
        matrix_const_data(classes)[2] != 1.0 || matrix_const_data(classes)[3] != 1.0) goto cleanup;
    matrix_destroy(probabilities);
    probabilities = NULL;
    matrix_destroy(classes);
    classes = NULL;
    ml_logistic_history_destroy(&history);

    if (ml_logistic_regression_predict_classes(model, features, 1.1, &classes) !=
        ML_STATUS_INVALID_ARGUMENT || classes != NULL ||
        ml_logistic_regression_predict_classes(model, features, -0.1, &classes) !=
        ML_STATUS_INVALID_ARGUMENT || classes != NULL ||
        ml_logistic_regression_binary_cross_entropy(model, features, invalid_labels, 4U, &loss) !=
        ML_STATUS_INVALID_ARGUMENT ||
        ml_logistic_regression_binary_cross_entropy(model, features, labels, 3U, &loss) !=
        ML_STATUS_INVALID_ARGUMENT) goto cleanup;

    /* A failed non-finite update rolls all parameters back. */
    {
        const double one_label[] = {1.0};
        const MLLogisticRegressionConfig divergent = {2U, 1U, DBL_MAX, 0.0};
        MLMatrix *one_row = NULL;
        MLLogisticRegression *rollback_model = NULL;
        MLLogisticHistory rollback_history = {0U, NULL};
        if (matrix_create(1U, 1U, &one_row) != ML_STATUS_OK ||
            ml_logistic_regression_create(1U, &rollback_model) != ML_STATUS_OK) {
            matrix_destroy(one_row);
            ml_logistic_regression_destroy(rollback_model);
            goto cleanup;
        }
        matrix_data(one_row)[0] = 4.0;
        if (ml_logistic_regression_train(rollback_model, one_row, one_label, 1U,
                                         &divergent, &rollback_history) != ML_STATUS_NUMERIC_ERROR ||
            ml_logistic_regression_weights(rollback_model)[0] != 0.0 ||
            ml_logistic_regression_bias(rollback_model) != 0.0 ||
            rollback_history.length != 0U || rollback_history.cross_entropy_values != NULL) {
            failed = 1;
        }
        ml_logistic_history_destroy(&rollback_history);
        matrix_destroy(one_row);
        ml_logistic_regression_destroy(rollback_model);
        if (failed != 0) goto cleanup;
    }
    matrix_data(features)[0] = NAN;
    if (ml_logistic_regression_predict_probabilities(model, features, &probabilities) !=
        ML_STATUS_INVALID_ARGUMENT || probabilities != NULL) goto cleanup;

    /* Cross-entropy remains finite when training creates very large logits. */
    {
        const double conflicting_labels[] = {1.0, 0.0};
        const MLLogisticRegressionConfig extreme_config = {1U, 1U, 1000000.0, 0.0};
        MLMatrix *extreme_features = NULL;
        MLLogisticRegression *extreme_model = NULL;
        MLLogisticHistory extreme_history = {0U, NULL};
        if (matrix_create(2U, 1U, &extreme_features) != ML_STATUS_OK ||
            ml_logistic_regression_create(1U, &extreme_model) != ML_STATUS_OK) {
            matrix_destroy(extreme_features);
            ml_logistic_regression_destroy(extreme_model);
            failed = 1;
            goto cleanup;
        }
        matrix_data(extreme_features)[0] = 1.0;
        matrix_data(extreme_features)[1] = 2.0;
        if (ml_logistic_regression_train(extreme_model, extreme_features, conflicting_labels, 2U,
                                         &extreme_config, &extreme_history) != ML_STATUS_OK ||
            ml_logistic_regression_binary_cross_entropy(extreme_model, extreme_features,
                                                        conflicting_labels, 2U, &loss) != ML_STATUS_OK ||
            !finite_double(loss) || fabs(loss - 125000.0) > 1e-9) failed = 1;
        ml_logistic_history_destroy(&extreme_history);
        ml_logistic_regression_destroy(extreme_model);
        matrix_destroy(extreme_features);
        if (failed != 0) goto cleanup;
    }
    completed = 1;

cleanup:
    if (completed == 0) failed = 1;
    ml_logistic_history_destroy(&history);
    matrix_destroy(probabilities);
    matrix_destroy(classes);
    matrix_destroy(features);
    ml_logistic_regression_destroy(model);
    return failed;
}
