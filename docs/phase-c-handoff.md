# Phase C handoff — Linear Regression

## Summary

Phase C implementation and documentation are present. The public API supports a zero-initialized linear model, prediction, training MSE, full-batch gradient descent with an MSE history, and deterministic seeded train/validation splitting. Tests now cover a known linear toy dataset, repeatability, invalid training configuration, non-finite feature rejection, and rollback after a non-finite update.

## Contract

- Features are a non-empty samples-by-features matrix; targets contain one finite value per sample.
- Model weights and bias start at zero. Prediction and MSE borrow model/data; prediction returns an owned N-by-1 matrix.
- Training is full-batch gradient descent with `2/n` times the sum of residual feature products and residuals for the bias. History contains initial training MSE and each completed update.
- Training stops at `max_iterations` or after `patience` consecutive updates where the absolute difference between successive training MSE values is at most `tolerance`. This measures loss stability and does not guarantee decreasing loss or convergence to an optimum.
- A numeric error during training restores the original parameters. Callers should initialize history to `{0, NULL}` and destroy it before reusing it; split outputs likewise must be zero-initialized or destroyed before reuse.
- Split uses Fisher–Yates with a fixed xorshift64* generator; seed zero maps to a documented internal nonzero default. The train count is truncation of `sample_count * train_fraction` and must leave at least one item in both partitions.

## Files

- `include/linear_regression.h`, `src/linear_regression.c`
- `tests/test_linear_regression.c`
- `docs/implementation-roadmap.md`, `README.md`

## Verification

- `C:\msys64\mingw64\bin\mingw32-make.exe -B test CC=C:/msys64/mingw64/bin/gcc.exe`: passed after rebuilding all targets.
- `build.bat test`: passed using the detected MSYS2 compiler path.
- C unit suite: `PASS: ml_error`, `PASS: app_cli`, `PASS: matrix`, `PASS: linear_regression`.
- CLI subprocess cases: help, version, invalid option, and conflicting options all passed.
- `python scripts/validate_infrastructure.py`: passed (10 roles, 12 skills, 8 templates).
- GCC 15.1 emitted `-Wfloat-conversion` diagnostics at existing `isfinite` calls in `src/matrix.c` and `src/linear_regression.c`; they did not prevent compilation. They were left visible rather than suppressed.
- Sanitizers, static analysis, and hosted CI were not run.

The CLI unit test previously depended on `tmpfile()`, which failed with the installed MinGW runtime in this environment. It now uses two files under `build/` and removes them during cleanup, so the test remains inside the project workspace.

## Remaining

Phase C functional acceptance checks pass locally. Review the `isfinite` conversion diagnostics and run static analysis/sanitizers and hosted CI as part of the later quality phase.
