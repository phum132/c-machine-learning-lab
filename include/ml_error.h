#ifndef ML_ERROR_H
#define ML_ERROR_H

typedef enum {
    ML_STATUS_OK = 0,
    ML_STATUS_INVALID_ARGUMENT,
    ML_STATUS_OUT_OF_MEMORY,
    ML_STATUS_IO_ERROR,
    ML_STATUS_PARSE_ERROR,
    ML_STATUS_DIMENSION_MISMATCH,
    ML_STATUS_NUMERIC_ERROR,
    ML_STATUS_INTERNAL_ERROR
} MLStatus;

/* Return a static, human-readable message. The returned string is not owned. */
const char *ml_status_string(MLStatus status);

#endif
