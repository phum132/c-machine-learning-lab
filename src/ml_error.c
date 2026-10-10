#include "ml_error.h"

const char *ml_status_string(MLStatus status)
{
    switch (status) {
    case ML_STATUS_OK:
        return "success";
    case ML_STATUS_INVALID_ARGUMENT:
        return "invalid argument";
    case ML_STATUS_OUT_OF_MEMORY:
        return "out of memory";
    case ML_STATUS_IO_ERROR:
        return "I/O error";
    case ML_STATUS_PARSE_ERROR:
        return "parse error";
    case ML_STATUS_DIMENSION_MISMATCH:
        return "dimension mismatch";
    case ML_STATUS_NUMERIC_ERROR:
        return "numeric error";
    case ML_STATUS_INTERNAL_ERROR:
        return "internal error";
    default:
        return "unknown error";
    }
}
