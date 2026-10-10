#ifndef MODEL_SERIALIZATION_H
#define MODEL_SERIALIZATION_H

#include "linear_regression.h"
#include "logistic_regression.h"

#define ML_MODEL_SERIALIZATION_MAX_FILE_SIZE (16U * 1024U * 1024U)
#define ML_MODEL_SERIALIZATION_MAX_FEATURES 250000U

/* Version 1 is ASCII whitespace-delimited text using hexadecimal doubles:
 * CMLMODEL 1, model type, feature count, bias, then one weight per line.
 * Files must be interpreted in the C numeric locale. Save replaces/truncates
 * the destination and is not atomic. Loads return an owned model. `out` is
 * cleared on entry and on every failure. */
MLStatus ml_model_save_linear(const char *path, const MLLinearRegression *model);
MLStatus ml_model_load_linear(const char *path, MLLinearRegression **out);
MLStatus ml_model_save_logistic(const char *path, const MLLogisticRegression *model);
MLStatus ml_model_load_logistic(const char *path, MLLogisticRegression **out);

#endif
