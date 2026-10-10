# Phase G handoff: CLI training and inference

## Scope delivered

- Added `train` for linear and logistic regression from numeric CSV with explicit target index and optional header.
- Added model persistence through the Phase F versioned serializer.
- Added `predict` for headerless, feature-only numeric CSV containing one or more rows. Linear output is one prediction per line; logistic output is probability and thresholded class per line.
- Preserved `--help` and `--version`; added strict option validation and documented exit codes.
- Input prediction CSV is capped at 16 MiB, 100,000 rows, 2,000,000 numeric cells, and 250,000 columns. Quoting, blank rows, inconsistent widths, non-finite values, and non-decimal tokens are rejected.

## Verification

Verified on 2026-10-10 with `build.bat test` using GCC 15.1: fresh CLI and test builds succeeded; all foundation tests, train/save/load/predict subprocess cases for both models, malformed input/dimension/non-finite/argument/work-limit checks, and infrastructure validation passed. `mingw32-make test CC=C:/msys64/mingw64/bin/gcc.exe` returned 0 using those already-built executables. A forced Make rebuild (`mingw32-make -B test`) could not launch the compiler in this sandbox (Windows access denied), so a fresh Make-driven compile is not verified here. The build reports GCC `-Wfloat-conversion` diagnostics from its `isfinite` macro expansion for double arguments; these warnings are not suppressed.

## Limits and next steps

- Training uses all supplied samples; there is no CLI validation split or normalization option.
- Training defaults are 1,000 iterations, patience 20, learning rate 0.01, and tolerance 1e-8. Dataset values must already be suitable for these settings.
- CLI accepts at most 1,000,000 iterations and caps sample-feature cell updates at 250,000,000 to bound runtime; each model training API also allocates an iteration history.
- Logistic prediction prints probabilities and classes; metrics and batch evaluation are not exposed by the CLI.
