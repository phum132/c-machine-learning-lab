#ifndef MODEL_SERIALIZATION_H
#define MODEL_SERIALIZATION_H

#include "feature_scaling.h"
#include "linear_regression.h"
#include "logistic_regression.h"

#define ML_MODEL_SERIALIZATION_MAX_FILE_SIZE (32U * 1024U * 1024U)
#define ML_MODEL_SERIALIZATION_MAX_FEATURES 250000U

/* ASCII whitespace-delimited text using hexadecimal doubles.
 * Version 1: CMLMODEL 1, model type, feature count, bias, one weight per line.
 * Version 2: the version 1 fields (with version 2), then the token `standard`
 * and one `mean scale` pair per feature for input standardization.
 * Files must be interpreted in the C numeric locale.
 *
 * Saves create a uniquely named `<path>.<random>.tmp` exclusively (C11 "wbx") and then replace `path`, so a failed save leaves
 * an existing destination untouched. Replacement uses rename (POSIX) or
 * MoveFileEx (Windows); file contents are not fsynced. Loads return an owned
 * model; `out` is cleared on entry and on every failure. */
MLStatus ml_model_save_linear(const char *path, const MLLinearRegression *model);
MLStatus ml_model_save_logistic(const char *path, const MLLogisticRegression *model);

/* `scaling` may be NULL or empty to write version 1. Otherwise its feature
 * count must match the model and the file is written as version 2. */
MLStatus ml_model_save_linear_scaled(const char *path, const MLLinearRegression *model,
                                     const MLFeatureScaling *scaling);
MLStatus ml_model_save_logistic_scaled(const char *path, const MLLogisticRegression *model,
                                       const MLFeatureScaling *scaling);

/* Load version 1 files only. A version 2 file returns
 * ML_STATUS_INVALID_ARGUMENT because the caller could not apply its scaling. */
MLStatus ml_model_load_linear(const char *path, MLLinearRegression **out);
MLStatus ml_model_load_logistic(const char *path, MLLogisticRegression **out);

/* Load version 1 or 2. `scaling` must be zero-initialized or destroyed; it is
 * cleared on entry, stays empty for version 1, and owns the stored statistics
 * for version 2. On failure both outputs are empty. */
MLStatus ml_model_load_linear_scaled(const char *path, MLLinearRegression **out,
                                     MLFeatureScaling *scaling);
MLStatus ml_model_load_logistic_scaled(const char *path, MLLogisticRegression **out,
                                       MLFeatureScaling *scaling);

#endif
