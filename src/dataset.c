#include "dataset.h"

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSV_MAX_FILE_BYTES (16U * 1024U * 1024U)
#define CSV_MAX_RECORD_BYTES (64U * 1024U)
#define CSV_MAX_FIELD_BYTES 4096U
#define CSV_MAX_COLUMNS 4096U
#define CSV_MAX_ROWS 100000U
#define CSV_MAX_CELLS 2000000U

typedef struct {
    size_t start;
    size_t length;
} CSVField;

struct MLDataset {
    size_t sample_count;
    size_t feature_count;
    size_t column_count;
    size_t target_column;
    MLMatrix *features;
    double *targets;
    char **column_names;
};

static void free_column_names(char **names, size_t count)
{
    if (names == NULL) return;
    for (size_t i = 0U; i < count; ++i) free(names[i]);
    free(names);
}

void ml_dataset_destroy(MLDataset *dataset)
{
    if (dataset == NULL) return;
    matrix_destroy(dataset->features);
    free(dataset->targets);
    free_column_names(dataset->column_names, dataset->column_count);
    free(dataset);
}

size_t ml_dataset_sample_count(const MLDataset *dataset)
{ return dataset == NULL ? 0U : dataset->sample_count; }

size_t ml_dataset_feature_count(const MLDataset *dataset)
{ return dataset == NULL ? 0U : dataset->feature_count; }

size_t ml_dataset_column_count(const MLDataset *dataset)
{ return dataset == NULL ? 0U : dataset->column_count; }

size_t ml_dataset_target_column(const MLDataset *dataset)
{ return dataset == NULL ? 0U : dataset->target_column; }

const MLMatrix *ml_dataset_features(const MLDataset *dataset)
{ return dataset == NULL ? NULL : dataset->features; }

const double *ml_dataset_targets(const MLDataset *dataset)
{ return dataset == NULL ? NULL : dataset->targets; }

const char *ml_dataset_column_name(const MLDataset *dataset, size_t column_index)
{
    if (dataset == NULL || dataset->column_names == NULL || column_index >= dataset->column_count)
        return NULL;
    return dataset->column_names[column_index];
}

static void trim_field(const char *buffer, CSVField *field)
{
    while (field->length > 0U &&
           (buffer[field->start] == ' ' || buffer[field->start] == '\t')) {
        ++field->start;
        --field->length;
    }
    while (field->length > 0U &&
           (buffer[field->start + field->length - 1U] == ' ' ||
            buffer[field->start + field->length - 1U] == '\t')) {
        --field->length;
    }
}

static MLStatus split_record(const char *buffer, size_t start, size_t length,
                             CSVField *fields, size_t *field_count)
{
    size_t count = 0U;
    size_t field_start = start;
    const size_t end = start + length;

    if (length == 0U || length > CSV_MAX_RECORD_BYTES) return ML_STATUS_PARSE_ERROR;
    for (size_t position = start; position <= end; ++position) {
        if (position != end && buffer[position] != ',') {
            if (buffer[position] == '"' || buffer[position] == '\r' || buffer[position] == '\0')
                return ML_STATUS_PARSE_ERROR;
            continue;
        }
        if (count >= CSV_MAX_COLUMNS) return ML_STATUS_PARSE_ERROR;
        fields[count].start = field_start;
        fields[count].length = position - field_start;
        trim_field(buffer, &fields[count]);
        if (fields[count].length == 0U || fields[count].length > CSV_MAX_FIELD_BYTES)
            return ML_STATUS_PARSE_ERROR;
        ++count;
        field_start = position + 1U;
    }
    *field_count = count;
    return ML_STATUS_OK;
}

static int valid_header_name(const char *buffer, CSVField field)
{
    const unsigned char first = (unsigned char)buffer[field.start];
    if (!((first >= (unsigned char)'A' && first <= (unsigned char)'Z') ||
          (first >= (unsigned char)'a' && first <= (unsigned char)'z') || first == (unsigned char)'_'))
        return 0;
    for (size_t i = 1U; i < field.length; ++i) {
        const unsigned char value = (unsigned char)buffer[field.start + i];
        if (!((value >= (unsigned char)'A' && value <= (unsigned char)'Z') ||
              (value >= (unsigned char)'a' && value <= (unsigned char)'z') ||
              (value >= (unsigned char)'0' && value <= (unsigned char)'9') ||
              value == (unsigned char)'_')) return 0;
    }
    return 1;
}

static int decimal_syntax_valid(const char *value)
{
    size_t position = 0U;
    size_t digits = 0U;
    if (value[position] == '+' || value[position] == '-') ++position;
    while (value[position] >= '0' && value[position] <= '9') { ++position; ++digits; }
    if (value[position] == '.') {
        ++position;
        while (value[position] >= '0' && value[position] <= '9') { ++position; ++digits; }
    }
    if (digits == 0U) return 0;
    if (value[position] == 'e' || value[position] == 'E') {
        size_t exponent_digits = 0U;
        ++position;
        if (value[position] == '+' || value[position] == '-') ++position;
        while (value[position] >= '0' && value[position] <= '9') {
            ++position;
            ++exponent_digits;
        }
        if (exponent_digits == 0U) return 0;
    }
    return value[position] == '\0';
}

static int ascii_equal_ignore_case(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        char a = *left++;
        char b = *right++;
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
    }
    return *left == '\0' && *right == '\0';
}

static int is_non_finite_token(const char *value)
{
    const char *unsigned_value = value;
    if (*unsigned_value == '+' || *unsigned_value == '-') ++unsigned_value;
    return ascii_equal_ignore_case(unsigned_value, "nan") ||
           ascii_equal_ignore_case(unsigned_value, "inf") ||
           ascii_equal_ignore_case(unsigned_value, "infinity");
}

static MLStatus parse_number(const char *buffer, CSVField field, double *out)
{
    char text[CSV_MAX_FIELD_BYTES + 1U];
    char *end = NULL;
    double value;

    memcpy(text, buffer + field.start, field.length);
    text[field.length] = '\0';
    if (is_non_finite_token(text)) return ML_STATUS_NUMERIC_ERROR;
    if (!decimal_syntax_valid(text)) return ML_STATUS_PARSE_ERROR;
    errno = 0;
    value = strtod(text, &end);
    if (end == text || *end != '\0') return ML_STATUS_PARSE_ERROR;
    if (errno == ERANGE || !isfinite(value)) return ML_STATUS_NUMERIC_ERROR;
    *out = value;
    return ML_STATUS_OK;
}

static char *copy_field(const char *buffer, CSVField field)
{
    char *copy = malloc(field.length + 1U);
    if (copy == NULL) return NULL;
    memcpy(copy, buffer + field.start, field.length);
    copy[field.length] = '\0';
    return copy;
}

static MLStatus read_file(const char *path, char **out_buffer, size_t *out_length)
{
    FILE *file = fopen(path, "rb");
    long file_size;
    char *buffer = NULL;
    size_t bytes_read;
    MLStatus status = ML_STATUS_OK;

    if (file == NULL) return ML_STATUS_IO_ERROR;
    if (fseek(file, 0L, SEEK_END) != 0 || (file_size = ftell(file)) < 0L) {
        status = ML_STATUS_IO_ERROR;
        if (fclose(file) != 0) status = ML_STATUS_IO_ERROR;
        return status;
    }
    if ((unsigned long)file_size > (unsigned long)CSV_MAX_FILE_BYTES) {
        status = ML_STATUS_PARSE_ERROR;
        if (fclose(file) != 0) status = ML_STATUS_IO_ERROR;
        return status;
    }
    if (fseek(file, 0L, SEEK_SET) != 0) {
        status = ML_STATUS_IO_ERROR;
        if (fclose(file) != 0) status = ML_STATUS_IO_ERROR;
        return status;
    }
    buffer = malloc((size_t)file_size + 1U);
    if (buffer == NULL) {
        (void)fclose(file);
        return ML_STATUS_OUT_OF_MEMORY;
    }
    bytes_read = fread(buffer, 1U, (size_t)file_size, file);
    if (bytes_read != (size_t)file_size || ferror(file)) status = ML_STATUS_IO_ERROR;
    if (fclose(file) != 0) status = ML_STATUS_IO_ERROR;
    if (status != ML_STATUS_OK) {
        free(buffer);
        return status;
    }
    buffer[bytes_read] = '\0';
    *out_buffer = buffer;
    *out_length = bytes_read;
    return ML_STATUS_OK;
}

static MLStatus append_values(double **values, size_t *value_count, size_t *capacity,
                              const double *row_values, size_t row_count)
{
    size_t required;
    size_t new_capacity;
    double *new_values;

    if (row_count > CSV_MAX_CELLS - *value_count) return ML_STATUS_PARSE_ERROR;
    required = *value_count + row_count;
    if (required > *capacity) {
        new_capacity = *capacity == 0U ? 64U : *capacity;
        while (new_capacity < required) {
            if (new_capacity > CSV_MAX_CELLS / 2U) { new_capacity = CSV_MAX_CELLS; break; }
            new_capacity *= 2U;
        }
        if (new_capacity > SIZE_MAX / sizeof(**values)) return ML_STATUS_PARSE_ERROR;
        new_values = realloc(*values, new_capacity * sizeof(**values));
        if (new_values == NULL) return ML_STATUS_OUT_OF_MEMORY;
        *values = new_values;
        *capacity = new_capacity;
    }
    memcpy(*values + *value_count, row_values, row_count * sizeof(**values));
    *value_count = required;
    return ML_STATUS_OK;
}

static MLStatus create_dataset(const double *values, size_t sample_count, size_t column_count,
                               size_t target_column, char **column_names, MLDataset **out)
{
    MLDataset *dataset = calloc(1U, sizeof(*dataset));
    size_t feature_index = 0U;
    MLStatus status;

    if (dataset == NULL) return ML_STATUS_OUT_OF_MEMORY;
    dataset->sample_count = sample_count;
    dataset->column_count = column_count;
    dataset->target_column = target_column;
    dataset->feature_count = column_count - 1U;
    status = matrix_create(sample_count, dataset->feature_count, &dataset->features);
    if (status != ML_STATUS_OK) { free(dataset); return status; }
    if (sample_count > SIZE_MAX / sizeof(*dataset->targets)) {
        ml_dataset_destroy(dataset);
        return ML_STATUS_INVALID_ARGUMENT;
    }
    dataset->targets = malloc(sample_count * sizeof(*dataset->targets));
    if (dataset->targets == NULL) { ml_dataset_destroy(dataset); return ML_STATUS_OUT_OF_MEMORY; }

    for (size_t row = 0U; row < sample_count; ++row) {
        feature_index = 0U;
        for (size_t column = 0U; column < column_count; ++column) {
            const double value = values[row * column_count + column];
            if (column == target_column) dataset->targets[row] = value;
            else matrix_data(dataset->features)[row * dataset->feature_count + feature_index++] = value;
        }
    }
    dataset->column_names = column_names;
    *out = dataset;
    return ML_STATUS_OK;
}

MLStatus ml_dataset_load_csv(const char *path, const MLDatasetCsvOptions *options,
                             MLDataset **out)
{
    char *buffer = NULL;
    size_t file_length = 0U;
    CSVField fields[CSV_MAX_COLUMNS];
    double row_values[CSV_MAX_COLUMNS];
    double *values = NULL;
    size_t value_count = 0U, value_capacity = 0U;
    size_t column_count = 0U, row_count = 0U;
    char **column_names = NULL;
    size_t position = 0U;
    int saw_header = 0;
    MLStatus status;

    if (out == NULL) return ML_STATUS_INVALID_ARGUMENT;
    *out = NULL;
    if (path == NULL || path[0] == '\0' || options == NULL ||
        (options->has_header != 0 && options->has_header != 1)) return ML_STATUS_INVALID_ARGUMENT;

    status = read_file(path, &buffer, &file_length);
    if (status != ML_STATUS_OK) return status;
    if (file_length == 0U) { status = ML_STATUS_PARSE_ERROR; goto cleanup; }

    while (position < file_length) {
        size_t line_start = position;
        size_t line_length;
        size_t field_count = 0U;
        size_t line_end;
        int has_newline;

        while (position < file_length && buffer[position] != '\n') ++position;
        line_end = position;
        has_newline = position < file_length;
        if (has_newline) ++position;
        if (has_newline && line_end > line_start && buffer[line_end - 1U] == '\r') --line_end;
        line_length = line_end - line_start;
        status = split_record(buffer, line_start, line_length, fields, &field_count);
        if (status != ML_STATUS_OK) goto cleanup;

        if (options->has_header != 0 && saw_header == 0) {
            if (field_count < 2U || options->target_column >= field_count) {
                status = ML_STATUS_PARSE_ERROR;
                goto cleanup;
            }
            column_names = calloc(field_count, sizeof(*column_names));
            if (column_names == NULL) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
            column_count = field_count;
            for (size_t column = 0U; column < field_count; ++column) {
                if (!valid_header_name(buffer, fields[column])) { status = ML_STATUS_PARSE_ERROR; goto cleanup; }
                column_names[column] = copy_field(buffer, fields[column]);
                if (column_names[column] == NULL) { status = ML_STATUS_OUT_OF_MEMORY; goto cleanup; }
                for (size_t previous = 0U; previous < column; ++previous) {
                    if (strcmp(column_names[previous], column_names[column]) == 0) {
                        status = ML_STATUS_PARSE_ERROR;
                        goto cleanup;
                    }
                }
            }
            saw_header = 1;
            continue;
        }

        if (column_count == 0U) {
            if (field_count < 2U || options->target_column >= field_count) {
                status = ML_STATUS_PARSE_ERROR;
                goto cleanup;
            }
            column_count = field_count;
        }
        if (field_count != column_count || row_count >= CSV_MAX_ROWS) {
            status = ML_STATUS_PARSE_ERROR;
            goto cleanup;
        }
        for (size_t column = 0U; column < column_count; ++column) {
            status = parse_number(buffer, fields[column], &row_values[column]);
            if (status != ML_STATUS_OK) goto cleanup;
        }
        status = append_values(&values, &value_count, &value_capacity, row_values, column_count);
        if (status != ML_STATUS_OK) goto cleanup;
        ++row_count;
    }

    if (row_count == 0U || column_count < 2U || value_count != row_count * column_count) {
        status = ML_STATUS_PARSE_ERROR;
        goto cleanup;
    }
    status = create_dataset(values, row_count, column_count, options->target_column,
                            column_names, out);
    if (status == ML_STATUS_OK) column_names = NULL;

cleanup:
    free(buffer);
    free(values);
    free_column_names(column_names, column_count);
    return status;
}
