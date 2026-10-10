#include "linear_regression.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

int test_linear_regression(void)
{
    MLMatrix *features = NULL, *predictions = NULL;
    MLLinearRegression *model = NULL;
    MLTrainingHistory history = {0U, NULL};
    MLLinearSplit first = {0}, second = {0};
    const double targets[] = {1.0, 3.0, 5.0, 7.0};
    const double input[] = {0.0, 1.0, 2.0, 3.0};
    const MLLinearRegressionConfig config = {2000U, 20U, 0.03, 1e-12};
    double mse = 0.0;

    if (matrix_create(4U, 1U, &features) != ML_STATUS_OK ||
        ml_linear_regression_create(1U, &model) != ML_STATUS_OK) goto fail;
    for (size_t i = 0U; i < 4U; ++i) matrix_data(features)[i] = input[i];
    if (ml_linear_regression_mse(model, features, targets, 4U, &mse) != ML_STATUS_OK || mse != 21.0) goto fail;
    if (ml_linear_regression_train(model, features, targets, 4U, &config, &history) != ML_STATUS_OK ||
        history.length < 2U || history.mse_values[history.length - 1U] >= 1e-8) goto fail;
    if (fabs(ml_linear_regression_weights(model)[0] - 2.0) > 1e-3 ||
        fabs(ml_linear_regression_bias(model) - 1.0) > 1e-3) goto fail;
    if (ml_linear_regression_predict(model, features, &predictions) != ML_STATUS_OK ||
        matrix_rows(predictions) != 4U || matrix_cols(predictions) != 1U ||
        fabs(matrix_const_data(predictions)[3] - 7.0) > 1e-3) goto fail;
    if (ml_linear_regression_split(features, targets, 4U, 0.75, UINT64_C(42), &first) != ML_STATUS_OK ||
        ml_linear_regression_split(features, targets, 4U, 0.75, UINT64_C(42), &second) != ML_STATUS_OK ||
        first.train_count != 3U || first.validation_count != 1U) goto fail;
    for (size_t i = 0U; i < first.train_count; ++i)
        if (first.train_targets[i] != second.train_targets[i] ||
            matrix_const_data(first.train_features)[i] != matrix_const_data(second.train_features)[i]) goto fail;
    if (ml_linear_regression_mse(model, features, targets, 3U, &mse) != ML_STATUS_INVALID_ARGUMENT) goto fail;

    /* Invalid training parameters leave the model unchanged and clear history. */
    {
        const MLLinearRegressionConfig invalid = {0U, 1U, 0.01, 0.0};
        MLTrainingHistory invalid_history = {0U, NULL};
        if (ml_linear_regression_train(model, features, targets, 4U, &invalid,
                                       &invalid_history) != ML_STATUS_INVALID_ARGUMENT ||
            invalid_history.length != 0U || invalid_history.mse_values != NULL ||
            fabs(ml_linear_regression_weights(model)[0] - 2.0) > 1e-3) goto fail;
    }

    /* A non-finite update rolls back all parameters and does not expose history. */
    {
        MLLinearRegression *rollback_model = NULL;
        MLTrainingHistory rollback_history = {0U, NULL};
        const double one_target[] = {1.0};
        const MLLinearRegressionConfig divergent = {2U, 1U, DBL_MAX, 0.0};
        MLMatrix *one_row = NULL;
        if (matrix_create(1U, 1U, &one_row) != ML_STATUS_OK ||
            ml_linear_regression_create(1U, &rollback_model) != ML_STATUS_OK) {
            matrix_destroy(one_row);
            ml_linear_regression_destroy(rollback_model);
            goto fail;
        }
        matrix_data(one_row)[0] = 1.0;
        if (ml_linear_regression_train(rollback_model, one_row, one_target, 1U,
                                       &divergent, &rollback_history) != ML_STATUS_NUMERIC_ERROR ||
            ml_linear_regression_weights(rollback_model)[0] != 0.0 ||
            ml_linear_regression_bias(rollback_model) != 0.0 ||
            rollback_history.length != 0U || rollback_history.mse_values != NULL) {
            matrix_destroy(one_row);
            ml_linear_regression_destroy(rollback_model);
            ml_training_history_destroy(&rollback_history);
            goto fail;
        }
        matrix_destroy(one_row);
        ml_linear_regression_destroy(rollback_model);
    }

    /* Reject non-finite features and keep the output pointer null. */
    matrix_destroy(predictions);
    predictions = NULL;
    matrix_data(features)[0] = NAN;
    if (ml_linear_regression_predict(model, features, &predictions) != ML_STATUS_INVALID_ARGUMENT ||
        predictions != NULL) goto fail;
    matrix_data(features)[0] = input[0];

    ml_linear_split_destroy(&first); ml_linear_split_destroy(&second);
    ml_training_history_destroy(&history); matrix_destroy(predictions); matrix_destroy(features);
    ml_linear_regression_destroy(model);
    return 0;
fail:
    ml_linear_split_destroy(&first); ml_linear_split_destroy(&second);
    ml_training_history_destroy(&history); matrix_destroy(predictions); matrix_destroy(features);
    ml_linear_regression_destroy(model);
    return 1;
}
