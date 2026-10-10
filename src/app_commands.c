#include "app_commands.h"

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "classification_metrics.h"
#include "dataset.h"
#include "model_serialization.h"

#define CLI_MAX_INPUT_BYTES (16U * 1024U * 1024U)
#define CLI_MAX_ROWS 100000U
#define CLI_MAX_CELLS 2000000U
#define CLI_MAX_ITERATIONS 1000000U
#define CLI_MAX_TRAIN_CELL_UPDATES 250000000U

typedef struct {
    const char *type;
    const char *data_path;
    const char *model_path;
    size_t target;
    size_t iterations;
    size_t patience;
    double learning_rate;
    double tolerance;
    int has_target;
    int has_header;
    int has_type;
    int has_data;
    int has_model;
    int has_iterations;
    int has_patience;
    int has_learning_rate;
    int has_tolerance;
} TrainOptions;

static int parse_size(const char *text, size_t *out)
{
    size_t value = 0U;
    if (text == NULL || *text == '\0') return 0;
    for (const unsigned char *p = (const unsigned char *)text; *p != 0U; ++p) {
        size_t digit;
        if (*p < '0' || *p > '9') return 0;
        digit = (size_t)(*p - '0');
        if (value > (SIZE_MAX - digit) / 10U) return 0;
        value = value * 10U + digit;
    }
    *out = value;
    return 1;
}

static int parse_finite_double(const char *text, double *out)
{
    char *end;
    double value;
    if (text == NULL || *text == '\0') return 0;
    errno = 0;
    value = strtod(text, &end);
    if (end == text || *end != '\0' || errno == ERANGE || !isfinite(value)) return 0;
    *out = value;
    return 1;
}

static int parse_train_options(int argc, char *const argv[], TrainOptions *options)
{
    memset(options, 0, sizeof(*options));
    options->iterations = 1000U;
    options->patience = 20U;
    options->learning_rate = 0.01;
    options->tolerance = 1e-8;
    for (int i = 2; i < argc; ++i) {
        const char *key = argv[i];
        const char *value = NULL;
        size_t parsed_size;
        double parsed_double;
        if (key == NULL) return 0;
        if (strcmp(key, "--header") == 0) {
            if (options->has_header) return 0;
            options->has_header = 1;
            continue;
        }
        if (i + 1 >= argc || argv[i + 1] == NULL) return 0;
        value = argv[++i];
        if (strcmp(key, "--type") == 0) {
            if (options->has_type || (strcmp(value, "linear") != 0 && strcmp(value, "logistic") != 0)) return 0;
            options->has_type = 1; options->type = value;
        } else if (strcmp(key, "--data") == 0) {
            if (options->has_data || value[0] == '\0') return 0;
            options->has_data = 1; options->data_path = value;
        } else if (strcmp(key, "--target") == 0) {
            if (options->has_target || !parse_size(value, &options->target)) return 0;
            options->has_target = 1;
        } else if (strcmp(key, "--model") == 0) {
            if (options->has_model || value[0] == '\0') return 0;
            options->has_model = 1; options->model_path = value;
        } else if (strcmp(key, "--iterations") == 0) {
            if (options->has_iterations || !parse_size(value, &parsed_size) ||
                parsed_size == 0U || parsed_size > CLI_MAX_ITERATIONS) return 0;
            options->has_iterations = 1; options->iterations = parsed_size;
        } else if (strcmp(key, "--patience") == 0) {
            if (options->has_patience || !parse_size(value, &parsed_size) || parsed_size == 0U) return 0;
            options->has_patience = 1; options->patience = parsed_size;
        } else if (strcmp(key, "--learning-rate") == 0) {
            if (options->has_learning_rate || !parse_finite_double(value, &parsed_double) || parsed_double <= 0.0) return 0;
            options->has_learning_rate = 1; options->learning_rate = parsed_double;
        } else if (strcmp(key, "--tolerance") == 0) {
            if (options->has_tolerance || !parse_finite_double(value, &parsed_double) || parsed_double < 0.0) return 0;
            options->has_tolerance = 1; options->tolerance = parsed_double;
        } else return 0;
    }
    return options->has_type && options->has_data && options->has_target && options->has_model;
}

static int report_status(FILE *err, const char *operation, MLStatus status)
{
    (void)fprintf(err, "Error: %s: %s\n", operation, ml_status_string(status));
    return 1;
}

static int train_command(int argc, char *const argv[], FILE *out, FILE *err)
{
    TrainOptions options;
    MLDatasetCsvOptions csv_options;
    MLDataset *dataset = NULL;
    MLTrainingHistory linear_history = {0U, NULL};
    MLLogisticHistory logistic_history = {0U, NULL};
    MLLinearRegression *linear = NULL;
    MLLogisticRegression *logistic = NULL;
    MLStatus status;
    if (!parse_train_options(argc, argv, &options)) {
        (void)fprintf(err, "Error: invalid or missing train option\n");
        return 2;
    }
    csv_options.has_header = options.has_header;
    csv_options.target_column = options.target;
    status = ml_dataset_load_csv(options.data_path, &csv_options, &dataset);
    if (status != ML_STATUS_OK) return report_status(err, "loading training CSV", status);
    {
        const size_t cells = ml_dataset_sample_count(dataset) * ml_dataset_feature_count(dataset);
        if (cells == 0U || options.iterations > CLI_MAX_TRAIN_CELL_UPDATES / cells) {
            ml_dataset_destroy(dataset);
            (void)fprintf(err, "Error: training workload exceeds the CLI safety limit\n");
            return 2;
        }
    }
    if (options.type[0] == 'l' && strcmp(options.type, "linear") == 0) {
        MLLinearRegressionConfig config = {options.iterations, options.patience,
                                           options.learning_rate, options.tolerance};
        status = ml_linear_regression_create(ml_dataset_feature_count(dataset), &linear);
        if (status == ML_STATUS_OK)
            status = ml_linear_regression_train(linear, ml_dataset_features(dataset),
                                                ml_dataset_targets(dataset),
                                                ml_dataset_sample_count(dataset), &config,
                                                &linear_history);
        if (status == ML_STATUS_OK) status = ml_model_save_linear(options.model_path, linear);
        if (status == ML_STATUS_OK)
            (void)fprintf(out, "Trained linear model on %zu rows with %zu features; saved %s\n",
                          ml_dataset_sample_count(dataset), ml_dataset_feature_count(dataset),
                          options.model_path);
        ml_training_history_destroy(&linear_history);
        ml_linear_regression_destroy(linear);
    } else {
        MLLogisticRegressionConfig config = {options.iterations, options.patience,
                                             options.learning_rate, options.tolerance};
        status = ml_logistic_regression_create(ml_dataset_feature_count(dataset), &logistic);
        if (status == ML_STATUS_OK)
            status = ml_logistic_regression_train(logistic, ml_dataset_features(dataset),
                                                  ml_dataset_targets(dataset),
                                                  ml_dataset_sample_count(dataset), &config,
                                                  &logistic_history);
        if (status == ML_STATUS_OK) status = ml_model_save_logistic(options.model_path, logistic);
        if (status == ML_STATUS_OK)
            (void)fprintf(out, "Trained logistic model on %zu rows with %zu features; saved %s\n",
                          ml_dataset_sample_count(dataset), ml_dataset_feature_count(dataset),
                          options.model_path);
        ml_logistic_history_destroy(&logistic_history);
        ml_logistic_regression_destroy(logistic);
    }
    ml_dataset_destroy(dataset);
    if (status != ML_STATUS_OK) return report_status(err, "training/saving model", status);
    return ferror(out) ? 1 : 0;
}

static int is_space(char value)
{ return value == ' ' || value == '\t' || value == '\r'; }

static int decimal_token_valid(const char *text)
{
    size_t position = 0U, digits = 0U;
    if (text[position] == '+' || text[position] == '-') ++position;
    while (text[position] >= '0' && text[position] <= '9') { ++position; ++digits; }
    if (text[position] == '.') {
        ++position;
        while (text[position] >= '0' && text[position] <= '9') { ++position; ++digits; }
    }
    if (digits == 0U) return 0;
    if (text[position] == 'e' || text[position] == 'E') {
        size_t exponent_digits = 0U;
        ++position;
        if (text[position] == '+' || text[position] == '-') ++position;
        while (text[position] >= '0' && text[position] <= '9') {
            ++position; ++exponent_digits;
        }
        if (exponent_digits == 0U) return 0;
    }
    return text[position] == '\0';
}

static MLStatus append_cli_value(double **values, size_t *count, size_t *capacity,
                                 double value)
{
    if (*count >= CLI_MAX_CELLS) return ML_STATUS_PARSE_ERROR;
    if (*count == *capacity) {
        size_t next = *capacity == 0U ? 128U : *capacity * 2U;
        double *grown;
        if (next > CLI_MAX_CELLS) next = CLI_MAX_CELLS;
        if (next > SIZE_MAX / sizeof(**values)) return ML_STATUS_OUT_OF_MEMORY;
        grown = realloc(*values, next * sizeof(**values));
        if (grown == NULL) return ML_STATUS_OUT_OF_MEMORY;
        *values = grown; *capacity = next;
    }
    (*values)[(*count)++] = value;
    return ML_STATUS_OK;
}

static MLStatus load_feature_csv(const char *path, MLMatrix **out)
{
    FILE *file = NULL;
    char *buffer = NULL;
    double *values = NULL;
    size_t value_count = 0U, capacity = 0U, rows = 0U, columns = 0U;
    long file_size;
    MLStatus status = ML_STATUS_PARSE_ERROR;
    *out = NULL;
    file = fopen(path, "rb");
    if (file == NULL) return ML_STATUS_IO_ERROR;
    if (fseek(file, 0L, SEEK_END) != 0 || (file_size = ftell(file)) < 0L ||
        (unsigned long)file_size > (unsigned long)CLI_MAX_INPUT_BYTES ||
        fseek(file, 0L, SEEK_SET) != 0) { status = ML_STATUS_PARSE_ERROR; goto cleanup; }
    buffer = malloc((size_t)file_size + 1U);
    if (buffer == NULL) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
    if (fread(buffer, 1U, (size_t)file_size, file) != (size_t)file_size || ferror(file)) {
        status = ML_STATUS_IO_ERROR; goto cleanup;
    }
    if (memchr(buffer, '\0', (size_t)file_size) != NULL) goto cleanup;
    buffer[file_size] = '\0';
    if (file_size == 0L) goto cleanup;
    {
        size_t position = 0U;
        const size_t length = (size_t)file_size;
        while (position < length) {
            size_t record_start = position, record_end, fields = 0U, field_start;
            status = ML_STATUS_PARSE_ERROR;
            while (position < length && buffer[position] != '\n') ++position;
            record_end = position;
            if (position < length) ++position;
            if (record_end > record_start && buffer[record_end - 1U] == '\r') --record_end;
            if (record_end == record_start || record_end - record_start > 65536U) goto cleanup;
            field_start = record_start;
            for (size_t p = record_start; p <= record_end; ++p) {
                if (p != record_end && buffer[p] != ',') {
                    if (buffer[p] == '"') goto cleanup;
                    continue;
                }
                status = ML_STATUS_PARSE_ERROR;
                size_t left = field_start, right = p;
                char *end;
                double number;
                while (left < right && is_space(buffer[left])) ++left;
                while (right > left && is_space(buffer[right - 1U])) --right;
                if (left == right || right - left > 128U) goto cleanup;
                {
                    char saved = buffer[right];
                    buffer[right] = '\0'; errno = 0;
                    if (!decimal_token_valid(buffer + left)) {
                        buffer[right] = saved; goto cleanup;
                    }
                    number = strtod(buffer + left, &end);
                    if (end == buffer + left || *end != '\0' || errno == ERANGE || !isfinite(number)) {
                        buffer[right] = saved; status = ML_STATUS_NUMERIC_ERROR; goto cleanup;
                    }
                    buffer[right] = saved;
                }
                status = append_cli_value(&values, &value_count, &capacity, number);
                if (status != ML_STATUS_OK) goto cleanup;
                ++fields; field_start = p + 1U;
            }
            if (columns == 0U) columns = fields;
            if (fields != columns || columns == 0U || columns > ML_MODEL_SERIALIZATION_MAX_FEATURES ||
                ++rows > CLI_MAX_ROWS) { status = ML_STATUS_PARSE_ERROR; goto cleanup; }
        }
    }
    if (rows == 0U || columns == 0U || value_count != rows * columns) goto cleanup;
    status = matrix_create(rows, columns, out);
    if (status != ML_STATUS_OK) goto cleanup;
    memcpy(matrix_data(*out), values, value_count * sizeof(*values));
cleanup:
    if (file != NULL && fclose(file) != 0 && status == ML_STATUS_OK) status = ML_STATUS_IO_ERROR;
    if (status != ML_STATUS_OK) { matrix_destroy(*out); *out = NULL; }
    free(buffer); free(values);
    return status;
}

static int predict_command(int argc, char *const argv[], FILE *out, FILE *err)
{
    const char *model_path = NULL, *input_path = NULL;
    double threshold = 0.5;
    int has_threshold = 0;
    MLLinearRegression *linear = NULL;
    MLLogisticRegression *logistic = NULL;
    MLMatrix *features = NULL, *predictions = NULL;
    MLStatus status;
    if (argc < 6) goto invalid;
    for (int i = 2; i < argc; ++i) {
        const char *key = argv[i];
        if (key == NULL || i + 1 >= argc || argv[i + 1] == NULL) goto invalid;
        if (strcmp(key, "--model") == 0 && model_path == NULL) model_path = argv[++i];
        else if (strcmp(key, "--input") == 0 && input_path == NULL) input_path = argv[++i];
        else if (strcmp(key, "--threshold") == 0 && !has_threshold &&
                 parse_finite_double(argv[i + 1], &threshold) && threshold >= 0.0 && threshold <= 1.0) {
            has_threshold = 1; ++i;
        } else goto invalid;
    }
    if (model_path == NULL || input_path == NULL) goto invalid;
    status = ml_model_load_linear(model_path, &linear);
    if (status == ML_STATUS_PARSE_ERROR) status = ml_model_load_logistic(model_path, &logistic);
    if (status != ML_STATUS_OK) return report_status(err, "loading model", status);
    status = load_feature_csv(input_path, &features);
    if (status != ML_STATUS_OK) goto runtime_error;
    if (linear != NULL) {
        if (has_threshold) {
            matrix_destroy(features);
            ml_linear_regression_destroy(linear);
            (void)fprintf(err, "Error: --threshold is only valid for logistic models\n");
            return 2;
        }
        if (matrix_cols(features) != ml_linear_regression_feature_count(linear)) {
            status = ML_STATUS_DIMENSION_MISMATCH; goto runtime_error;
        }
        status = ml_linear_regression_predict(linear, features, &predictions);
        if (status == ML_STATUS_OK)
            for (size_t row = 0U; row < matrix_rows(predictions); ++row)
                (void)fprintf(out, "%.17g\n", matrix_const_data(predictions)[row]);
    } else {
        if (matrix_cols(features) != ml_logistic_regression_feature_count(logistic)) {
            status = ML_STATUS_DIMENSION_MISMATCH; goto runtime_error;
        }
        status = ml_logistic_regression_predict_probabilities(logistic, features, &predictions);
        if (status == ML_STATUS_OK)
            for (size_t row = 0U; row < matrix_rows(predictions); ++row) {
                double probability = matrix_const_data(predictions)[row];
                (void)fprintf(out, "%.17g,%d\n", probability, probability >= threshold ? 1 : 0);
            }
    }
    matrix_destroy(predictions); matrix_destroy(features);
    ml_linear_regression_destroy(linear); ml_logistic_regression_destroy(logistic);
    if (status != ML_STATUS_OK) return report_status(err, "predicting", status);
    return ferror(out) ? 1 : 0;
runtime_error:
    matrix_destroy(predictions); matrix_destroy(features);
    ml_linear_regression_destroy(linear); ml_logistic_regression_destroy(logistic);
    return report_status(err, "loading prediction input/predicting", status);
invalid:
    (void)fprintf(err, "Error: invalid predict options; expected --model FILE --input FEATURES.csv [--threshold X]\n");
    return 2;
}

int app_commands_run(int argc, char *const argv[], FILE *out, FILE *err)
{
    if (argc < 2 || argv == NULL || argv[1] == NULL) return 2;
    if (strcmp(argv[1], "train") == 0) return train_command(argc, argv, out, err);
    if (strcmp(argv[1], "predict") == 0) return predict_command(argc, argv, out, err);
    (void)fprintf(err, "Error: unknown argument: %s\n", argv[1]);
    return 2;
}
