#include "ml_error.h"

#include <string.h>

int test_ml_error(void)
{
    static const struct {
        MLStatus status;
        const char *expected;
    } cases[] = {
        {ML_STATUS_OK, "success"},
        {ML_STATUS_INVALID_ARGUMENT, "invalid argument"},
        {ML_STATUS_OUT_OF_MEMORY, "out of memory"},
        {ML_STATUS_IO_ERROR, "I/O error"},
        {ML_STATUS_PARSE_ERROR, "parse error"},
        {ML_STATUS_DIMENSION_MISMATCH, "dimension mismatch"},
        {ML_STATUS_NUMERIC_ERROR, "numeric error"},
        {ML_STATUS_INTERNAL_ERROR, "internal error"},
        {(MLStatus)999, "unknown error"},
    };

    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (strcmp(ml_status_string(cases[index].status), cases[index].expected) != 0) {
            return 1;
        }
    }
    return 0;
}
