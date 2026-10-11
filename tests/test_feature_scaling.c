#include "feature_scaling.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); goto cleanup; } } while (0)

static int near(double left, double right) { return fabs(left - right) <= 1e-12; }

int test_feature_scaling(void)
{
    /* Column 0: 1,2,3,4 -> mean 2.5, population std sqrt(1.25).
     * Column 1: constant 7 -> mean 7, scale falls back to 1. */
    const double values[] = {1.0, 7.0, 2.0, 7.0, 3.0, 7.0, 4.0, 7.0};
    const double means[] = {2.5, 7.0};
    const double scales[] = {2.0, 1.0};
    const double zero_scale[] = {2.0, 0.0};
    const double non_finite[] = {NAN, 1.0};
    MLMatrix *features = NULL, *wrong_width = NULL;
    MLFeatureScaling fitted = {0U, NULL, NULL}, created = {0U, NULL, NULL};
    int failed = 1;

    CHECK(matrix_create(4U, 2U, &features) == ML_STATUS_OK);
    memcpy(matrix_data(features), values, sizeof(values));
    CHECK(ml_feature_scaling_fit(features, &fitted) == ML_STATUS_OK);
    CHECK(fitted.feature_count == 2U);
    CHECK(near(fitted.means[0], 2.5) && near(fitted.scales[0], sqrt(1.25)));
    CHECK(near(fitted.means[1], 7.0) && fitted.scales[1] == 1.0);

    CHECK(ml_feature_scaling_apply(&fitted, features) == ML_STATUS_OK);
    {
        const double *scaled = matrix_const_data(features);
        double sum = 0.0, squared = 0.0;
        for (size_t row = 0U; row < 4U; ++row) {
            sum += scaled[row * 2U];
            squared += scaled[row * 2U] * scaled[row * 2U];
            CHECK(scaled[row * 2U + 1U] == 0.0);
        }
        CHECK(near(sum, 0.0) && near(squared / 4.0, 1.0));
        CHECK(near(scaled[0], -1.5 / sqrt(1.25)));
    }

    CHECK(ml_feature_scaling_create(means, scales, 2U, &created) == ML_STATUS_OK);
    memcpy(matrix_data(features), values, sizeof(values));
    CHECK(ml_feature_scaling_apply(&created, features) == ML_STATUS_OK);
    CHECK(matrix_const_data(features)[0] == -0.75 && matrix_const_data(features)[6] == 0.75);

    /* Invalid statistics and dimension mismatches leave inputs untouched. */
    CHECK(ml_feature_scaling_create(means, zero_scale, 2U, &created) == ML_STATUS_NUMERIC_ERROR);
    CHECK(created.feature_count == 0U && created.means == NULL);
    CHECK(ml_feature_scaling_create(non_finite, scales, 2U, &created) == ML_STATUS_NUMERIC_ERROR);
    CHECK(ml_feature_scaling_create(NULL, scales, 2U, &created) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(ml_feature_scaling_create(means, scales, 0U, &created) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(matrix_create(1U, 3U, &wrong_width) == ML_STATUS_OK);
    memset(matrix_data(wrong_width), 0, 3U * sizeof(double));
    CHECK(ml_feature_scaling_apply(&fitted, wrong_width) == ML_STATUS_DIMENSION_MISMATCH);
    CHECK(ml_feature_scaling_apply(&created, features) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(ml_feature_scaling_apply(NULL, features) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(ml_feature_scaling_fit(NULL, &created) == ML_STATUS_INVALID_ARGUMENT);
    CHECK(ml_feature_scaling_fit(features, NULL) == ML_STATUS_INVALID_ARGUMENT);

    /* A non-finite input fails fitting and leaves the output empty. */
    matrix_data(features)[3] = INFINITY;
    CHECK(ml_feature_scaling_fit(features, &created) == ML_STATUS_NUMERIC_ERROR);
    CHECK(created.feature_count == 0U && created.scales == NULL);
    /* Applying with an overflowing result leaves the matrix unchanged. */
    {
        const double tiny_scale[] = {1e-300, 1.0};
        const double zero_mean[] = {0.0, 0.0};
        CHECK(ml_feature_scaling_create(zero_mean, tiny_scale, 2U, &created) == ML_STATUS_OK);
        memcpy(matrix_data(features), values, sizeof(values));
        matrix_data(features)[2] = 1e300;
        CHECK(ml_feature_scaling_apply(&created, features) == ML_STATUS_NUMERIC_ERROR);
        CHECK(matrix_const_data(features)[0] == 1.0 && matrix_const_data(features)[2] == 1e300);
    }
    failed = 0;
cleanup:
    ml_feature_scaling_destroy(&fitted);
    ml_feature_scaling_destroy(&created);
    ml_feature_scaling_destroy(NULL);
    matrix_destroy(features);
    matrix_destroy(wrong_width);
    return failed;
}
