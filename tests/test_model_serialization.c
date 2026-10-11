#include "model_serialization.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); goto cleanup; } } while (0)

static int close_fixture(FILE **file)
{
    FILE *owned_file;
    int result;
    if (file == NULL || *file == NULL) return 0;
    owned_file = *file;
    *file = NULL;
    result = fclose(owned_file);
    return result == 0;
}

int test_model_serialization(void)
{
    const char *linear_path = "build/model-linear-test.txt";
    const char *logistic_path = "build/model-logistic-test.txt";
    const char *bad_path = "build/model-invalid-test.txt";
    const char *large_path = "build/model-oversized-test.txt";
    const double weights[] = {0.125, -3.5, 1.0 / 7.0};
    const double invalid[] = {8.0, NAN, 9.0};
    MLLinearRegression *linear = NULL, *loaded_linear = NULL;
    MLLogisticRegression *logistic = NULL, *loaded_logistic = NULL;
    FILE *file = NULL;
    int failed = 1;

    CHECK(ml_linear_regression_create(3U, &linear) == ML_STATUS_OK);
    CHECK(ml_linear_regression_set_parameters(linear, weights, 3U, -0.75) == ML_STATUS_OK);
    CHECK(ml_model_save_linear(NULL, linear) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(ml_model_save_linear("", linear) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(ml_model_save_linear(linear_path, NULL) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(ml_linear_regression_set_parameters(linear, invalid, 3U, 12.0) == ML_STATUS_NUMERIC_ERROR);
    CHECK(ml_linear_regression_bias(linear) == -0.75);
    CHECK(ml_linear_regression_weights(linear)[0] == weights[0]);
    CHECK(ml_model_save_linear(linear_path, linear) == ML_STATUS_OK);
    CHECK(ml_model_load_linear(linear_path, &loaded_linear) == ML_STATUS_OK);
    CHECK(ml_linear_regression_feature_count(loaded_linear) == 3U);
    CHECK(ml_linear_regression_bias(loaded_linear) == -0.75);
    for (size_t i = 0U; i < 3U; ++i)
        CHECK(ml_linear_regression_weights(loaded_linear)[i] == weights[i]);
    ml_linear_regression_destroy(loaded_linear);
    loaded_linear = NULL;

    CHECK(ml_logistic_regression_create(3U, &logistic) == ML_STATUS_OK);
    CHECK(ml_logistic_regression_set_parameters(logistic, weights, 3U, 2.25) == ML_STATUS_OK);
    CHECK(ml_model_save_logistic(logistic_path, logistic) == ML_STATUS_OK);
    CHECK(ml_model_load_logistic(logistic_path, &loaded_logistic) == ML_STATUS_OK);
    CHECK(ml_logistic_regression_feature_count(loaded_logistic) == 3U);
    CHECK(ml_logistic_regression_bias(loaded_logistic) == 2.25);
    for (size_t i = 0U; i < 3U; ++i)
        CHECK(ml_logistic_regression_weights(loaded_logistic)[i] == weights[i]);

    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 2\nlinear\n1\n0x0p+0\n0x1p+0\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(loaded_linear == NULL);
    loaded_linear = linear; /* failed loads must clear an existing output value */
    CHECK(ml_model_load_linear(NULL, &loaded_linear) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(loaded_linear == NULL);
    CHECK(ml_model_load_linear(bad_path, NULL) == ML_STATUS_INVALID_ARGUMENT);
    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 1\nlinear\n1\n0x0p+0\nnot-a-hex-float\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(loaded_linear == NULL);
    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 1\nlinear\n1\n0x0p+0\n0x1p+0\nextra\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(loaded_linear == NULL);
    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 1\nlinear\n1\n0x1p+1024\n0x1p+0\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_NUMERIC_ERROR);
    CHECK(loaded_linear == NULL);
    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 1\nlogistic\n1\n0x0p+0\n0x1p+0\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(file == NULL && loaded_linear == NULL);
    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 1\nlinear\n1\n0x0p+0\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(loaded_linear == NULL);
    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 1\nlinear\n1\n0x0p+0\n0x1p+0 trailing\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(loaded_linear == NULL);
    file = fopen(bad_path, "wb");
    CHECK(file != NULL);
    CHECK(fputs("CMLMODEL 1\nlinear\n250001\n0x0p+0\n", file) >= 0);
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(bad_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(loaded_linear == NULL);
    file = fopen(large_path, "wb");
    CHECK(file != NULL);
    {
        unsigned char block[8192] = {0};
        for (size_t i = 0U; i < (size_t)ML_MODEL_SERIALIZATION_MAX_FILE_SIZE / sizeof(block); ++i)
            CHECK(fwrite(block, 1U, sizeof(block), file) == sizeof(block));
        CHECK(fputc(0, file) != EOF);
    }
    CHECK(close_fixture(&file));
    CHECK(ml_model_load_linear(large_path, &loaded_linear) == ML_STATUS_PARSE_ERROR);
    CHECK(loaded_linear == NULL);
    CHECK(ml_model_load_linear("build/no-such-model-file", &loaded_linear) == ML_STATUS_IO_ERROR);
    CHECK(loaded_linear == NULL);
    failed = 0;
cleanup:
    if (file != NULL) (void)close_fixture(&file);
    ml_linear_regression_destroy(linear);
    ml_linear_regression_destroy(loaded_linear);
    ml_logistic_regression_destroy(logistic);
    ml_logistic_regression_destroy(loaded_logistic);
    (void)remove(linear_path);
    (void)remove(logistic_path);
    (void)remove(bad_path);
    (void)remove(large_path);
    return failed;
}
