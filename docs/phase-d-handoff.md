# Phase D handoff — Dataset and CSV

## Summary

Added an owning numeric dataset abstraction and bounded CSV loader. The caller chooses an optional header and target column by original CSV index. The resulting read-only feature matrix and target vector plug directly into the existing Linear Regression API.

## Contract and limits

See [ADR-001](adr-001-dataset-csv.md) for the accepted grammar, status mapping, ownership, locale requirement, and limits. In particular, the parser deliberately rejects quoted fields and other general-purpose CSV features. Limits are checked before unbounded allocation; the file itself is capped before reading.

## Files

- `include/dataset.h`, `src/dataset.c`
- `tests/test_dataset.c`, `tests/test_main.c`
- `Makefile`, `build.bat`
- `docs/adr-001-dataset-csv.md`, `docs/implementation-roadmap.md`, `docs/architecture.md`, `README.md`

## Verification

- `build.bat test`: passed using GCC 15.1 from the detected MSYS2 installation.
- `C:\msys64\mingw64\bin\mingw32-make.exe -B test CC=C:/msys64/mingw64/bin/gcc.exe`: passed after forcing a complete rebuild.
- C unit suite passed: `ml_error`, `app_cli`, `matrix`, `linear_regression`, and `dataset`.
- CLI subprocess cases (help, version, invalid option, multiple options) passed.
- Infrastructure validation passed: 10 roles, 12 skills, and 8 templates.
- `gcc -std=c11 -Wall -Wextra -Wpedantic -fanalyzer -Iinclude -c src/dataset.c -o build/dataset-analyzer.o`: completed with no diagnostics.
- Dataset fixtures cover header and no-header input, CRLF/final newline behavior, a one-row dataset, target selection and Linear Regression interoperability, empty/blank/missing/ragged/malformed input, duplicate/invalid headers, non-finite/out-of-range numbers, oversized field/file, invalid options, and I/O failure.
- GCC reports `-Wfloat-conversion` warnings at `isfinite` calls under the repository's `-Wconversion` flags in matrix, regression, and dataset sources. Builds and tests still pass; warnings were not suppressed.

## Remaining

No known Phase D feature remains after the documented parser grammar/limits and test suite pass. Sanitizer/static-analysis and hosted CI remain in the later quality/release checks.
