#include "model_serialization.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MLStatus save_model(const char *path, const char *type, size_t count,
                           const double *weights, double bias)
{
    FILE *file;
    int failed = 0;
    if (path == NULL || path[0] == '\0' || weights == NULL || count == 0U)
        return ML_STATUS_INVALID_ARGUMENT;
    if (count > ML_MODEL_SERIALIZATION_MAX_FEATURES) return ML_STATUS_INVALID_ARGUMENT;
    if (!isfinite(bias)) return ML_STATUS_NUMERIC_ERROR;
    for (size_t i = 0U; i < count; ++i)
        if (!isfinite(weights[i])) return ML_STATUS_NUMERIC_ERROR;
    file = fopen(path, "wb");
    if (file == NULL) return ML_STATUS_IO_ERROR;
    if (fprintf(file, "CMLMODEL 1\n%s\n%zu\n%a\n", type, count, bias) < 0)
        failed = 1;
    for (size_t i = 0U; !failed && i < count; ++i)
        if (fprintf(file, "%a\n", weights[i]) < 0) failed = 1;
    if (fclose(file) != 0) failed = 1;
    return failed ? ML_STATUS_IO_ERROR : ML_STATUS_OK;
}

static int ascii_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static char *next_token(char **cursor)
{
    char *start;
    while (**cursor != '\0' && ascii_space(**cursor)) ++*cursor;
    if (**cursor == '\0') return NULL;
    start = *cursor;
    while (**cursor != '\0' && !ascii_space(**cursor)) ++*cursor;
    if (**cursor != '\0') *(*cursor)++ = '\0';
    return start;
}

static int parse_count(const char *token, size_t *out)
{
    size_t value = 0U;
    if (token == NULL || *token == '\0') return 0;
    for (const unsigned char *p = (const unsigned char *)token; *p != 0U; ++p) {
        size_t digit;
        if (*p < '0' || *p > '9') return 0;
        digit = (size_t)(*p - '0');
        if (value > (SIZE_MAX - digit) / 10U) return 0;
        value = value * 10U + digit;
    }
    if (value == 0U || value > ML_MODEL_SERIALIZATION_MAX_FEATURES) return 0;
    *out = value;
    return 1;
}

static int parse_double(const char *token, double *out)
{
    char *end;
    double value;
    if (token == NULL || strlen(token) < 3U) return 0;
    if (!(token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) &&
        !(token[0] == '-' && token[1] == '0' && (token[2] == 'x' || token[2] == 'X')) &&
        !(token[0] == '+' && token[1] == '0' && (token[2] == 'x' || token[2] == 'X')))
        return 0;
    errno = 0;
    value = strtod(token, &end);
    if (end == token || *end != '\0') return 0;
    if (errno == ERANGE || !isfinite(value)) return -1;
    *out = value;
    return 1;
}

static MLStatus read_model(const char *path, const char *expected_type,
                           size_t *count_out, double **weights_out, double *bias_out)
{
    FILE *file;
    long file_size;
    char *buffer = NULL;
    double *weights = NULL;
    MLStatus status = ML_STATUS_PARSE_ERROR;
    if (path == NULL || path[0] == '\0' || count_out == NULL ||
        weights_out == NULL || bias_out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *count_out = 0U;
    *weights_out = NULL;
    file = fopen(path, "rb");
    if (file == NULL) return ML_STATUS_IO_ERROR;
    if (fseek(file, 0L, SEEK_END) != 0 || (file_size = ftell(file)) < 0L ||
        (unsigned long)file_size > (unsigned long)ML_MODEL_SERIALIZATION_MAX_FILE_SIZE ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return ML_STATUS_PARSE_ERROR;
    }
    buffer = malloc((size_t)file_size + 1U);
    if (buffer == NULL) { (void)fclose(file); return ML_STATUS_OUT_OF_MEMORY; }
    if (fread(buffer, 1U, (size_t)file_size, file) != (size_t)file_size || ferror(file)) {
        status = ML_STATUS_IO_ERROR;
        goto cleanup;
    }
    if (fclose(file) != 0) { file = NULL; status = ML_STATUS_IO_ERROR; goto cleanup; }
    file = NULL;
    if (memchr(buffer, '\0', (size_t)file_size) != NULL) goto cleanup;
    buffer[file_size] = '\0';
    {
        char *cursor = buffer;
        char *magic = next_token(&cursor);
        char *version = next_token(&cursor);
        char *type = next_token(&cursor);
        char *count_token = next_token(&cursor);
        size_t count;
        double bias;
        if (magic == NULL || strcmp(magic, "CMLMODEL") != 0 || version == NULL ||
            strcmp(version, "1") != 0 || type == NULL || strcmp(type, expected_type) != 0 ||
            !parse_count(count_token, &count)) goto cleanup;
        {
            char *bias_token = next_token(&cursor);
            int parsed;
            if (bias_token == NULL) goto cleanup;
            parsed = parse_double(bias_token, &bias);
            if (parsed <= 0) { if (parsed < 0) status = ML_STATUS_NUMERIC_ERROR; goto cleanup; }
        }
        if (count > SIZE_MAX / sizeof(*weights)) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
        weights = malloc(count * sizeof(*weights));
        if (weights == NULL) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
        for (size_t i = 0U; i < count; ++i) {
            char *token = next_token(&cursor);
            int parsed;
            if (token == NULL) goto cleanup;
            parsed = parse_double(token, &weights[i]);
            if (parsed <= 0) { if (parsed < 0) status = ML_STATUS_NUMERIC_ERROR; goto cleanup; }
        }
        if (next_token(&cursor) != NULL) goto cleanup;
        *count_out = count;
        *bias_out = bias;
        *weights_out = weights;
        weights = NULL;
        status = ML_STATUS_OK;
    }
cleanup:
    if (file != NULL) (void)fclose(file);
    free(weights);
    free(buffer);
    return status;
}

MLStatus ml_model_save_linear(const char *path, const MLLinearRegression *model)
{
    if (model == NULL) return ML_STATUS_INVALID_ARGUMENT;
    return save_model(path, "linear", ml_linear_regression_feature_count(model),
                      ml_linear_regression_weights(model), ml_linear_regression_bias(model));
}

MLStatus ml_model_save_logistic(const char *path, const MLLogisticRegression *model)
{
    if (model == NULL) return ML_STATUS_INVALID_ARGUMENT;
    return save_model(path, "logistic", ml_logistic_regression_feature_count(model),
                      ml_logistic_regression_weights(model), ml_logistic_regression_bias(model));
}

MLStatus ml_model_load_linear(const char *path, MLLinearRegression **out)
{
    size_t count;
    double *weights = NULL, bias;
    MLLinearRegression *model = NULL;
    MLStatus status;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    status = read_model(path, "linear", &count, &weights, &bias);
    if (status != ML_STATUS_OK) return status;
    status = ml_linear_regression_create(count, &model);
    if (status == ML_STATUS_OK) status = ml_linear_regression_set_parameters(model, weights, count, bias);
    free(weights);
    if (status != ML_STATUS_OK) { ml_linear_regression_destroy(model); return status; }
    *out = model;
    return ML_STATUS_OK;
}

MLStatus ml_model_load_logistic(const char *path, MLLogisticRegression **out)
{
    size_t count;
    double *weights = NULL, bias;
    MLLogisticRegression *model = NULL;
    MLStatus status;
    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    status = read_model(path, "logistic", &count, &weights, &bias);
    if (status != ML_STATUS_OK) return status;
    status = ml_logistic_regression_create(count, &model);
    if (status == ML_STATUS_OK) status = ml_logistic_regression_set_parameters(model, weights, count, bias);
    free(weights);
    if (status != ML_STATUS_OK) { ml_logistic_regression_destroy(model); return status; }
    *out = model;
    return ML_STATUS_OK;
}
