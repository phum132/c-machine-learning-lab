#include "linear_regression.h"
#include "logistic_regression.h"

#include <math.h>
#include <stddef.h>

/* Ridge regression with one feature has a closed form: with population
 * variance var(x) and covariance cov(x, y), w = cov / (var + l2) and
 * b = mean(y) - w * mean(x). For x = 0..3, y = 2x + 1, l2 = 1:
 * var = 1.25, cov = 2.5, so w = 2.5 / 2.25 and b = 4 - 1.5 w. */
static int ridge_matches_closed_form(void)
{
    const double x[] = {0.0, 1.0, 2.0, 3.0}, y[] = {1.0, 3.0, 5.0, 7.0};
    const MLLinearRegressionConfig ridge = {20000U, 50U, 0.05, 1e-15, 1.0};
    const MLLinearRegressionConfig negative = {10U, 1U, 0.05, 0.0, -0.1};
    MLMatrix *features = NULL;
    MLLinearRegression *model = NULL;
    int failed = 1;
    const double w = 2.5 / 2.25, b = 4.0 - 1.5 * w;

    if (matrix_create(4U, 1U, &features) != ML_STATUS_OK || ml_linear_regression_create(1U, &model) != ML_STATUS_OK)
        goto done;
    for (size_t i = 0U; i < 4U; ++i) matrix_data(features)[i] = x[i];
    if (ml_linear_regression_train(model, features, y, 4U, &negative, NULL) != ML_STATUS_INVALID_ARGUMENT) goto done;
    if (ml_linear_regression_train(model, features, y, 4U, &ridge, NULL) != ML_STATUS_OK) goto done;
    if (fabs(ml_linear_regression_weights(model)[0] - w) > 1e-6 || fabs(ml_linear_regression_bias(model) - b) > 1e-6)
        goto done;
    failed = 0;
done:
    matrix_destroy(features);
    ml_linear_regression_destroy(model);
    return failed;
}

/* On separable data the unregularized weight keeps growing; L2 keeps it smaller. */
static int l2_shrinks_logistic_weight(void)
{
    const double x[] = {-2.0, -1.0, 1.0, 2.0}, labels[] = {0.0, 0.0, 1.0, 1.0};
    const MLLogisticRegressionConfig plain = {3000U, 3000U, 0.5, 0.0, 0.0};
    const MLLogisticRegressionConfig shrunk = {3000U, 3000U, 0.5, 0.0, 0.1};
    const MLLogisticRegressionConfig negative = {10U, 1U, 0.5, 0.0, -1.0};
    MLMatrix *features = NULL;
    MLLogisticRegression *a = NULL, *b = NULL;
    MLLogisticHistory history = {0U, NULL};
    int failed = 1;

    if (matrix_create(4U, 1U, &features) != ML_STATUS_OK || ml_logistic_regression_create(1U, &a) != ML_STATUS_OK ||
        ml_logistic_regression_create(1U, &b) != ML_STATUS_OK) goto done;
    for (size_t i = 0U; i < 4U; ++i) matrix_data(features)[i] = x[i];
    if (ml_logistic_regression_train(a, features, labels, 4U, &negative, NULL) != ML_STATUS_INVALID_ARGUMENT) goto done;
    if (ml_logistic_regression_train(a, features, labels, 4U, &plain, NULL) != ML_STATUS_OK ||
        ml_logistic_regression_train(b, features, labels, 4U, &shrunk, &history) != ML_STATUS_OK) goto done;
    {
        const double wa = ml_logistic_regression_weights(a)[0], wb = ml_logistic_regression_weights(b)[0];
        if (!(wb > 0.0 && wb < 0.5 * wa)) goto done;
        /* The recorded loss includes the penalty: last value >= penalty term. */
        if (history.length < 2U || history.cross_entropy_values[history.length - 1U] < 0.1 * wb * wb) goto done;
    }
    failed = 0;
done:
    ml_logistic_history_destroy(&history);
    matrix_destroy(features);
    ml_logistic_regression_destroy(a);
    ml_logistic_regression_destroy(b);
    return failed;
}

int test_regularization(void)
{
    return ridge_matches_closed_form() || l2_shrinks_logistic_weight();
}
