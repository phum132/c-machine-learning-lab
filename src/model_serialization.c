#include "model_serialization.h"

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "ml_numeric.h"

static int scaling_present(const MLFeatureScaling *scaling)
{
    return scaling != NULL && scaling->feature_count != 0U;
}

static int replace_file(const char *source, const char *destination)
{
#ifdef _WIN32
    return MoveFileExA(source, destination, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(source, destination) == 0;
#endif
}

/* ".<8 hex>-<2 digits>.tmp" is 16 bytes; leave room for the terminator. */
#define TEMP_SUFFIX_MAX 32U
#define TEMP_ATTEMPTS 64U

/* Create a new temporary file next to `path`. Mode "x" (C11) refuses any
 * existing entry, including a planted symlink, and the varying suffix keeps
 * concurrent or crashed saves from colliding. Returns NULL on failure. */
static FILE *open_temporary(const char *path, char *buffer, size_t buffer_size)
{
    static unsigned long counter = 0UL;
    const unsigned long seed = (unsigned long)time(NULL) ^ ((unsigned long)clock() << 16U) ^
                               (unsigned long)(uintptr_t)buffer;
    for (unsigned attempt = 0U; attempt < TEMP_ATTEMPTS; ++attempt) {
        FILE *file;
        const unsigned long token = (seed + ++counter * 2654435761UL) & 0xffffffffUL;
        if (snprintf(buffer, buffer_size, "%s.%08lx-%02u.tmp", path, token, attempt) < 0)
            return NULL;
        file = fopen(buffer, "wbx");
        if (file != NULL) return file;
    }
    return NULL;
}

static MLStatus save_model(const char *path, const char *type, size_t count,
                           const double *weights, double bias,
                           const MLFeatureScaling *scaling)
{
    FILE *file;
    char *temporary_path;
    size_t path_length;
    int failed = 0;
    const int scaled = scaling_present(scaling);
    if (path == NULL || path[0] == '\0' || weights == NULL || count == 0U)
        return ML_STATUS_INVALID_ARGUMENT;
    if (count > ML_MODEL_SERIALIZATION_MAX_FEATURES) return ML_STATUS_INVALID_ARGUMENT;
    if (scaled && (scaling->feature_count != count || scaling->means == NULL || scaling->scales == NULL))
        return ML_STATUS_INVALID_ARGUMENT;
    if (!ml_is_finite(bias)) return ML_STATUS_NUMERIC_ERROR;
    for (size_t i = 0U; i < count; ++i) {
        if (!ml_is_finite(weights[i])) return ML_STATUS_NUMERIC_ERROR;
        if (scaled && (!ml_is_finite(scaling->means[i]) || !ml_is_finite(scaling->scales[i]) ||
                       scaling->scales[i] <= 0.0)) return ML_STATUS_NUMERIC_ERROR;
    }
    path_length = strlen(path);
    if (path_length > SIZE_MAX - TEMP_SUFFIX_MAX) return ML_STATUS_INVALID_ARGUMENT;
    temporary_path = malloc(path_length + TEMP_SUFFIX_MAX);
    if (temporary_path == NULL) return ML_STATUS_OUT_OF_MEMORY;
    file = open_temporary(path, temporary_path, path_length + TEMP_SUFFIX_MAX);
    if (file == NULL) { free(temporary_path); return ML_STATUS_IO_ERROR; }
    if (fprintf(file, "CMLMODEL %d\n%s\n%zu\n%a\n", scaled ? 2 : 1, type, count, bias) < 0)
        failed = 1;
    for (size_t i = 0U; !failed && i < count; ++i)
        if (fprintf(file, "%a\n", weights[i]) < 0) failed = 1;
    if (!failed && scaled && fprintf(file, "standard\n") < 0) failed = 1;
    for (size_t i = 0U; !failed && scaled && i < count; ++i)
        if (fprintf(file, "%a %a\n", scaling->means[i], scaling->scales[i]) < 0) failed = 1;
    if (fclose(file) != 0) failed = 1;
    if (!failed && !replace_file(temporary_path, path)) failed = 1;
    if (failed) (void)remove(temporary_path);
    free(temporary_path);
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
    if (errno == ERANGE || !ml_is_finite(value)) return -1;
    *out = value;
    return 1;
}

/* Parses `count` doubles into `values`. Returns OK, PARSE_ERROR, or NUMERIC_ERROR. */
static MLStatus parse_doubles(char **cursor, double *values, size_t count)
{
    for (size_t i = 0U; i < count; ++i) {
        const int parsed = parse_double(next_token(cursor), &values[i]);
        if (parsed < 0) return ML_STATUS_NUMERIC_ERROR;
        if (parsed == 0) return ML_STATUS_PARSE_ERROR;
    }
    return ML_STATUS_OK;
}

typedef struct {
    size_t count;
    double bias;
    double *weights;
    /* Interleaved mean/scale pairs; NULL for version 1. */
    double *scaling_pairs;
} ParsedModel;

static void parsed_model_destroy(ParsedModel *model)
{
    free(model->weights);
    free(model->scaling_pairs);
    memset(model, 0, sizeof(*model));
}

static MLStatus read_model(const char *path, const char *expected_type, ParsedModel *out)
{
    FILE *file;
    long file_size;
    char *buffer = NULL;
    ParsedModel parsed = {0U, 0.0, NULL, NULL};
    MLStatus status = ML_STATUS_PARSE_ERROR;
    memset(out, 0, sizeof(*out));
    if (path == NULL || path[0] == '\0') return ML_STATUS_INVALID_ARGUMENT;
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
        int scaled;
        if (magic == NULL || strcmp(magic, "CMLMODEL") != 0 || version == NULL ||
            (strcmp(version, "1") != 0 && strcmp(version, "2") != 0) || type == NULL ||
            strcmp(type, expected_type) != 0 || !parse_count(count_token, &parsed.count)) goto cleanup;
        scaled = version[0] == '2';
        status = parse_doubles(&cursor, &parsed.bias, 1U);
        if (status != ML_STATUS_OK) goto cleanup;
        if (parsed.count > SIZE_MAX / (2U * sizeof(double))) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
        parsed.weights = malloc(parsed.count * sizeof(*parsed.weights));
        if (parsed.weights == NULL) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
        status = parse_doubles(&cursor, parsed.weights, parsed.count);
        if (status != ML_STATUS_OK) goto cleanup;
        if (scaled) {
            char *method = next_token(&cursor);
            status = ML_STATUS_PARSE_ERROR;
            if (method == NULL || strcmp(method, "standard") != 0) goto cleanup;
            parsed.scaling_pairs = malloc(2U * parsed.count * sizeof(*parsed.scaling_pairs));
            if (parsed.scaling_pairs == NULL) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
            status = parse_doubles(&cursor, parsed.scaling_pairs, 2U * parsed.count);
            if (status != ML_STATUS_OK) goto cleanup;
            for (size_t i = 0U; i < parsed.count; ++i)
                if (parsed.scaling_pairs[2U * i + 1U] <= 0.0) { status = ML_STATUS_NUMERIC_ERROR; goto cleanup; }
        }
        status = ML_STATUS_PARSE_ERROR;
        if (next_token(&cursor) != NULL) goto cleanup;
        *out = parsed;
        memset(&parsed, 0, sizeof(parsed));
        status = ML_STATUS_OK;
    }
cleanup:
    if (file != NULL) (void)fclose(file);
    parsed_model_destroy(&parsed);
    free(buffer);
    return status;
}

static MLStatus build_scaling(const ParsedModel *parsed, MLFeatureScaling *scaling)
{
    double *means, *scales;
    MLStatus status;
    if (parsed->scaling_pairs == NULL) return ML_STATUS_OK;
    means = malloc(parsed->count * sizeof(*means));
    scales = malloc(parsed->count * sizeof(*scales));
    if (means == NULL || scales == NULL) { free(means); free(scales); return ML_STATUS_OUT_OF_MEMORY; }
    for (size_t i = 0U; i < parsed->count; ++i) {
        means[i] = parsed->scaling_pairs[2U * i];
        scales[i] = parsed->scaling_pairs[2U * i + 1U];
    }
    status = ml_feature_scaling_create(means, scales, parsed->count, scaling);
    free(means);
    free(scales);
    return status;
}

MLStatus ml_model_save_linear_scaled(const char *path, const MLLinearRegression *model,
                                     const MLFeatureScaling *scaling)
{
    if (model == NULL) return ML_STATUS_INVALID_ARGUMENT;
    return save_model(path, "linear", ml_linear_regression_feature_count(model),
                      ml_linear_regression_weights(model), ml_linear_regression_bias(model), scaling);
}

MLStatus ml_model_save_logistic_scaled(const char *path, const MLLogisticRegression *model,
                                       const MLFeatureScaling *scaling)
{
    if (model == NULL) return ML_STATUS_INVALID_ARGUMENT;
    return save_model(path, "logistic", ml_logistic_regression_feature_count(model),
                      ml_logistic_regression_weights(model), ml_logistic_regression_bias(model),
                      scaling);
}

MLStatus ml_model_save_linear(const char *path, const MLLinearRegression *model)
{
    return ml_model_save_linear_scaled(path, model, NULL);
}

MLStatus ml_model_save_logistic(const char *path, const MLLogisticRegression *model)
{
    return ml_model_save_logistic_scaled(path, model, NULL);
}

MLStatus ml_model_load_linear_scaled(const char *path, MLLinearRegression **out,
                                     MLFeatureScaling *scaling)
{
    ParsedModel parsed;
    MLLinearRegression *model = NULL;
    MLStatus status;
    if (out == NULL || scaling == NULL) {
        if (out != NULL) *out = NULL;
        if (scaling != NULL) memset(scaling, 0, sizeof(*scaling));
        return ML_STATUS_INVALID_ARGUMENT;
    }
    *out = NULL;
    memset(scaling, 0, sizeof(*scaling));
    status = read_model(path, "linear", &parsed);
    if (status != ML_STATUS_OK) return status;
    status = ml_linear_regression_create(parsed.count, &model);
    if (status == ML_STATUS_OK)
        status = ml_linear_regression_set_parameters(model, parsed.weights, parsed.count, parsed.bias);
    if (status == ML_STATUS_OK) status = build_scaling(&parsed, scaling);
    parsed_model_destroy(&parsed);
    if (status != ML_STATUS_OK) {
        ml_linear_regression_destroy(model);
        ml_feature_scaling_destroy(scaling);
        return status;
    }
    *out = model;
    return ML_STATUS_OK;
}

MLStatus ml_model_load_logistic_scaled(const char *path, MLLogisticRegression **out,
                                       MLFeatureScaling *scaling)
{
    ParsedModel parsed;
    MLLogisticRegression *model = NULL;
    MLStatus status;
    if (out == NULL || scaling == NULL) {
        if (out != NULL) *out = NULL;
        if (scaling != NULL) memset(scaling, 0, sizeof(*scaling));
        return ML_STATUS_INVALID_ARGUMENT;
    }
    *out = NULL;
    memset(scaling, 0, sizeof(*scaling));
    status = read_model(path, "logistic", &parsed);
    if (status != ML_STATUS_OK) return status;
    status = ml_logistic_regression_create(parsed.count, &model);
    if (status == ML_STATUS_OK)
        status = ml_logistic_regression_set_parameters(model, parsed.weights, parsed.count, parsed.bias);
    if (status == ML_STATUS_OK) status = build_scaling(&parsed, scaling);
    parsed_model_destroy(&parsed);
    if (status != ML_STATUS_OK) {
        ml_logistic_regression_destroy(model);
        ml_feature_scaling_destroy(scaling);
        return status;
    }
    *out = model;
    return ML_STATUS_OK;
}

MLStatus ml_model_load_linear(const char *path, MLLinearRegression **out)
{
    MLFeatureScaling scaling = {0U, NULL, NULL};
    MLStatus status = ml_model_load_linear_scaled(path, out, &scaling);
    if (status == ML_STATUS_OK && scaling_present(&scaling)) {
        ml_linear_regression_destroy(*out);
        *out = NULL;
        status = ML_STATUS_INVALID_ARGUMENT;
    }
    ml_feature_scaling_destroy(&scaling);
    return status;
}

MLStatus ml_model_load_logistic(const char *path, MLLogisticRegression **out)
{
    MLFeatureScaling scaling = {0U, NULL, NULL};
    MLStatus status = ml_model_load_logistic_scaled(path, out, &scaling);
    if (status == ML_STATUS_OK && scaling_present(&scaling)) {
        ml_logistic_regression_destroy(*out);
        *out = NULL;
        status = ML_STATUS_INVALID_ARGUMENT;
    }
    ml_feature_scaling_destroy(&scaling);
    return status;
}
