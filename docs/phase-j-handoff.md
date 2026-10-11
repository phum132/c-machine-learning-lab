# Phase J handoff: CLI evaluation workflow

## Summary

Closed the known limitations listed in the Phase I README: the CLI can standardize features (statistics fitted on training rows only and stored in the model), hold out a seeded validation split, and evaluate a labeled CSV with regression or classification metrics. Model saves now write a temporary file and replace the destination only on success. The `-Wfloat-conversion` diagnostics from MinGW's `isfinite` macro are gone.

## Files Changed

- `include/ml_numeric.h` — `ml_is_finite`, using `__builtin_isfinite` on GCC/Clang; replaces `isfinite` in every `src/*.c` file.
- `include/feature_scaling.h`, `src/feature_scaling.c` — fit/create/apply/destroy for mean/std standardization.
- `include/regression_metrics.h`, `src/regression_metrics.c` — MAE, RMSE, R² (undefined R² is reported, not divided by zero).
- `include/model_serialization.h`, `src/model_serialization.c` — format version 2 with scaling, `*_scaled` save/load functions, temporary-file replacement, file cap raised to 32 MiB.
- `src/app_commands.c`, `src/app_cli.c` — `--normalize`, `--validation-split`, `--seed`, training/validation metrics output, `evaluate` command, scaling applied in `predict`.
- `tests/test_feature_scaling.c`, `tests/test_regression_metrics.c`, `tests/test_model_serialization.c`, `tests/test_main.c`, `scripts/test_cli.py` — new unit and subprocess tests.
- `Makefile`, `build.bat` — new sources and tests.
- `README.md`, `docs/architecture.md`, `docs/adr-003-model-serialization.md`, `docs/implementation-roadmap.md` — updated to match.

## Interface Changes

- New public headers: `feature_scaling.h`, `regression_metrics.h`, `ml_numeric.h`.
- New functions: `ml_model_save_{linear,logistic}_scaled`, `ml_model_load_{linear,logistic}_scaled`.
- Behavior change: `ml_model_load_linear`/`ml_model_load_logistic` return `ML_STATUS_INVALID_ARGUMENT` for a version 2 file. Version 1 files load as before.
- `ML_MODEL_SERIALIZATION_MAX_FILE_SIZE` is now 32 MiB.
- CLI `train` prints extra `Training:`/`Validation:` metric lines after the existing first line. Exit codes are unchanged (0/1/2).

## Verification

- `build.bat test` (WinLibs GCC 16.2.0) — exit 0; 10 C test suites, 10 CLI subprocess checks, infrastructure validation passed.
- MSYS2 GCC 15.1.0 `-fsyntax-only` with the project warning flags over `main.c`, `src/*.c`, `tests/*.c` — no warnings (previously 49 `-Wfloat-conversion` warnings in 7 source files).
- WinLibs GCC 16 `-fanalyzer` per source file — only two `-Wanalyzer-imprecise-fp-arithmetic` warnings in `src/linear_regression.c` (split allocation). Both also appear on the unmodified code, so this phase did not introduce them.
- `build.bat test` with MSYS2 Clang and `-fsanitize=address,undefined` — exit 0; full suite passed.
- README commands for `--normalize --validation-split 0.2 --seed 7`, `evaluate`, and `predict` — run manually, exit 0, predictions ≈ -2, 2, 7.
- Manual check: on `x*1000` data, training without `--normalize` fails with a numeric error (exit 1) and writes no model; with `--normalize`, R² = 1. The CLI tests now cover this.

## Issues

- Hosted GitHub Actions has not run on these changes yet.
- `--version` still reports `0.1.0`.

## Risks

- No fsync: a power loss right after a save could leave an empty or partial file on some filesystems.
- Concurrent saves to the same path share `<path>.tmp`.
- Validation splits are random, not stratified; tiny datasets can produce a one-row validation set where R² is `n/a`.

## Next Steps

1. Switch `examples/*/run_demo.py` and `scripts/run_linear_csv.py` to `--normalize`/`evaluate` instead of scaling in Python.
2. Bump the CLI version once these changes ship.
3. Connect personal-finance-tracker to the CLI (train/evaluate on exported monthly totals).
