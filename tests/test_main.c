#include <stdio.h>

int test_ml_error(void);
int test_app_cli(void);
int test_matrix(void);
int test_linear_regression(void);
int test_dataset(void);
int test_logistic_regression(void);
int test_classification_metrics(void);
int test_model_serialization(void);
int test_feature_scaling(void);
int test_regression_metrics(void);

int main(void)
{
    int failed = 0;

    if (test_ml_error() != 0) {
        (void)fprintf(stderr, "FAIL: ml_error tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: ml_error tests\n");
    }

    if (test_app_cli() != 0) {
        (void)fprintf(stderr, "FAIL: app_cli tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: app_cli tests\n");
    }

    if (test_matrix() != 0) {
        (void)fprintf(stderr, "FAIL: matrix tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: matrix tests\n");
    }

    if (test_linear_regression() != 0) {
        (void)fprintf(stderr, "FAIL: linear_regression tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: linear_regression tests\n");
    }

    if (test_dataset() != 0) {
        (void)fprintf(stderr, "FAIL: dataset tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: dataset tests\n");
    }

    if (test_logistic_regression() != 0) {
        (void)fprintf(stderr, "FAIL: logistic_regression tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: logistic_regression tests\n");
    }

    if (test_classification_metrics() != 0) {
        (void)fprintf(stderr, "FAIL: classification_metrics tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: classification_metrics tests\n");
    }

    if (test_model_serialization() != 0) {
        (void)fprintf(stderr, "FAIL: model_serialization tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: model_serialization tests\n");
    }

    if (test_feature_scaling() != 0) {
        (void)fprintf(stderr, "FAIL: feature_scaling tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: feature_scaling tests\n");
    }

    if (test_regression_metrics() != 0) {
        (void)fprintf(stderr, "FAIL: regression_metrics tests\n");
        failed = 1;
    } else {
        (void)fprintf(stdout, "PASS: regression_metrics tests\n");
    }

    return failed;
}
