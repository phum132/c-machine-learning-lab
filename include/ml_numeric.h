#ifndef ML_NUMERIC_H
#define ML_NUMERIC_H

#include <math.h>

/* Finite check for doubles. MinGW's type-generic isfinite macro expands to
 * float/long double branches that trigger -Wfloat-conversion, so GCC and
 * Clang use the builtin directly. */
static inline int ml_is_finite(double value)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_isfinite(value);
#else
    return isfinite(value);
#endif
}

#endif
