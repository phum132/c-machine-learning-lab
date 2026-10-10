#include "dataset.h"

#include "linear_regression.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATASET_FIXTURE_PATH "build/test-dataset.tmp"
#define DATASET_TEST_MAX_FILE_BYTES (16U * 1024U * 1024U)

static int write_fixture(const char *contents, size_t length)
{
    FILE *file = fopen(DATASET_FIXTURE_PATH, "wb");
    int success;
    if (file == NULL) return 0;
    success = fwrite(contents, 1U, length, file) == length;
    if (fclose(file) != 0) success = 0;
    return success;
}

static int expect_load_status(const char *contents, size_t length,
                              const MLDatasetCsvOptions *options, MLStatus expected)
{
    MLDataset *dataset = NULL;
    MLStatus status;
    int output_was_null;
    if (!write_fixture(contents, length)) return 0;
    status = ml_dataset_load_csv(DATASET_FIXTURE_PATH, options, &dataset);
    output_was_null = dataset == NULL;
    ml_dataset_destroy(dataset);
    (void)remove(DATASET_FIXTURE_PATH);
    return status == expected && output_was_null;
}

int test_dataset(void)
{
    const MLDatasetCsvOptions with_header = {1, 2U};
    const MLDatasetCsvOptions no_header = {0, 1U};
    const char valid_csv[] = "id,height,weight\r\n0,1,1\r\n1,3,2\r\n2,5,3";
    const char single_row[] = "4,9\n";
    MLDataset *dataset = NULL;
    MLLinearRegression *model = NULL;
    double mse = 0.0;
    int failed = 0;

    if (!write_fixture(valid_csv, sizeof(valid_csv) - 1U) ||
        ml_dataset_load_csv(DATASET_FIXTURE_PATH, &with_header, &dataset) != ML_STATUS_OK ||
        dataset == NULL || ml_dataset_sample_count(dataset) != 3U ||
        ml_dataset_feature_count(dataset) != 2U || ml_dataset_column_count(dataset) != 3U ||
        ml_dataset_target_column(dataset) != 2U ||
        strcmp(ml_dataset_column_name(dataset, 0U), "id") != 0 ||
        strcmp(ml_dataset_column_name(dataset, 1U), "height") != 0 ||
        strcmp(ml_dataset_column_name(dataset, 2U), "weight") != 0 ||
        matrix_rows(ml_dataset_features(dataset)) != 3U ||
        matrix_cols(ml_dataset_features(dataset)) != 2U ||
        matrix_const_data(ml_dataset_features(dataset))[0] != 0.0 ||
        matrix_const_data(ml_dataset_features(dataset))[1] != 1.0 ||
        matrix_const_data(ml_dataset_features(dataset))[4] != 2.0 ||
        ml_dataset_targets(dataset)[0] != 1.0 || ml_dataset_targets(dataset)[2] != 3.0 ||
        ml_dataset_column_name(dataset, 3U) != NULL ||
        ml_linear_regression_create(2U, &model) != ML_STATUS_OK ||
        ml_linear_regression_mse(model, ml_dataset_features(dataset), ml_dataset_targets(dataset),
                                 ml_dataset_sample_count(dataset), &mse) != ML_STATUS_OK ||
        fabs(mse - (14.0 / 3.0)) > 1e-12) {
        failed = 1;
        goto cleanup;
    }
    ml_linear_regression_destroy(model);
    model = NULL;
    ml_dataset_destroy(dataset);
    dataset = NULL;
    (void)remove(DATASET_FIXTURE_PATH);

    if (!write_fixture(single_row, sizeof(single_row) - 1U) ||
        ml_dataset_load_csv(DATASET_FIXTURE_PATH, &no_header, &dataset) != ML_STATUS_OK ||
        dataset == NULL || ml_dataset_sample_count(dataset) != 1U ||
        ml_dataset_feature_count(dataset) != 1U ||
        ml_dataset_targets(dataset)[0] != 9.0 ||
        matrix_const_data(ml_dataset_features(dataset))[0] != 4.0 ||
        ml_dataset_column_name(dataset, 0U) != NULL) {
        failed = 1;
        goto cleanup;
    }
    ml_dataset_destroy(dataset);
    dataset = NULL;
    (void)remove(DATASET_FIXTURE_PATH);

    if (!expect_load_status("", 0U, &no_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("1,2\n\n3,4\n", sizeof("1,2\n\n3,4\n") - 1U,
                            &no_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("1,,2\n", sizeof("1,,2\n") - 1U,
                            &no_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("1,2,\n", sizeof("1,2,\n") - 1U,
                            &no_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("1,2\n3\n", sizeof("1,2\n3\n") - 1U,
                            &no_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("x,y\n1,2\n", sizeof("x,y\n1,2\n") - 1U,
                            &no_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("feature,target\n1,2\n", sizeof("feature,target\n1,2\n") - 1U,
                            &with_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("x,x\n1,2\n", sizeof("x,x\n1,2\n") - 1U,
                            &with_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("\"x\",y\n1,2\n", sizeof("\"x\",y\n1,2\n") - 1U,
                            &with_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("1,2junk\n", sizeof("1,2junk\n") - 1U,
                            &no_header, ML_STATUS_PARSE_ERROR) ||
        !expect_load_status("1,NaN\n", sizeof("1,NaN\n") - 1U,
                            &no_header, ML_STATUS_NUMERIC_ERROR) ||
        !expect_load_status("1,-Infinity\n", sizeof("1,-Infinity\n") - 1U,
                            &no_header, ML_STATUS_NUMERIC_ERROR) ||
        !expect_load_status("1,1e9999\n", sizeof("1,1e9999\n") - 1U,
                            &no_header, ML_STATUS_NUMERIC_ERROR)) {
        failed = 1;
        goto cleanup;
    }
    {
        char oversized_field[4098];
        memset(oversized_field, '1', sizeof(oversized_field) - 1U);
        oversized_field[sizeof(oversized_field) - 1U] = '\n';
        if (!expect_load_status(oversized_field, sizeof(oversized_field), &no_header,
                                ML_STATUS_PARSE_ERROR)) {
            failed = 1;
            goto cleanup;
        }
    }
    {
        const size_t oversized_length = (size_t)DATASET_TEST_MAX_FILE_BYTES + 1U;
        char *oversized_file = malloc(oversized_length);
        if (oversized_file == NULL) {
            failed = 1;
            goto cleanup;
        }
        memset(oversized_file, '0', oversized_length);
        if (!expect_load_status(oversized_file, oversized_length, &no_header,
                                ML_STATUS_PARSE_ERROR)) {
            free(oversized_file);
            failed = 1;
            goto cleanup;
        }
        free(oversized_file);
    }
    if (write_fixture("1,2\n", sizeof("1,2\n") - 1U) == 0 ||
        ml_dataset_load_csv(DATASET_FIXTURE_PATH, NULL, &dataset) != ML_STATUS_INVALID_ARGUMENT ||
        dataset != NULL ||
        ml_dataset_load_csv("build/no-such-dataset.csv", &no_header, &dataset) != ML_STATUS_IO_ERROR ||
        dataset != NULL) {
        failed = 1;
        goto cleanup;
    }

cleanup:
    ml_linear_regression_destroy(model);
    ml_dataset_destroy(dataset);
    (void)remove(DATASET_FIXTURE_PATH);
    return failed;
}
