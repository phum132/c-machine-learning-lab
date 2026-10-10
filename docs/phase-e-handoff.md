# Phase E handoff — Logistic Regression

## Summary

Added binary logistic regression with stable sigmoid and binary cross-entropy, full-batch gradient descent, probability/class prediction, training history, and standalone accuracy/precision/recall/F1 metrics. Detailed public behavior is recorded in [ADR-002](adr-002-logistic-regression.md).

## Contract

- Labels and predicted classes must be exactly `0.0` or `1.0`; non-finite and other values are rejected.
- Model parameters start at zero. The optimizer minimizes mean binary cross-entropy with the sample-mean logistic gradient.
- Stable sigmoid uses separate positive/negative branches. Loss uses `max(z,0) - z*y + log1p(exp(-abs(z)))`.
- The class threshold is finite and inclusive in `[0,1]`; a probability equal to the threshold predicts class 1.
- Training history includes initial loss and post-update losses. Stopping uses loss-difference tolerance/patience or the iteration limit; it does not guarantee monotonic loss decrease or convergence. Numeric failures roll back the model.
- Metric zero denominators produce 0 for precision, recall, and F1. Empty metric inputs are invalid.

## Files

- `include/logistic_regression.h`, `src/logistic_regression.c`
- `include/classification_metrics.h`, `src/classification_metrics.c`
- `tests/test_logistic_regression.c`, `tests/test_classification_metrics.c`, `tests/test_main.c`
- `Makefile`, `build.bat`
- `docs/adr-002-logistic-regression.md`, `docs/implementation-roadmap.md`, `docs/architecture.md`, `README.md`

## Verification

- `build.bat test`: passed using GCC 15.1 from MSYS2.
- `C:\msys64\mingw64\bin\mingw32-make.exe -B test CC=C:/msys64/mingw64/bin/gcc.exe`: passed after a full rebuild.
- C unit suite passed: error, CLI, matrix, linear regression, dataset, logistic regression, and classification metrics.
- CLI subprocess and infrastructure checks passed.
- Tests cover sigmoid at 0 and +/-1000, initial BCE, deterministic separable training, threshold equality, large-logit BCE, invalid labels/shapes/thresholds, numeric rollback, expected confusion counts, empty input, non-binary and non-finite metrics input, and zero-denominator behavior.
- GCC `-fanalyzer` completed for `src/logistic_regression.c` and `src/classification_metrics.c` without diagnostics.
- GCC 15.1 emits `-Wfloat-conversion` diagnostics at `isfinite` calls with the repository's `-Wconversion`; compilation and all checks still pass. No warning suppression was added.

## Remaining

No known Phase E feature remains after tests pass. Serialization and CLI model workflows remain Phase F/G. Sanitizers and hosted CI remain later quality checks.
