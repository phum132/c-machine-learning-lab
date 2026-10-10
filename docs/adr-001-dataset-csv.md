# ADR-001: Bounded numeric CSV dataset API

- **Status:** Accepted for Phase D
- **Date:** 2026-10-10
- **Decision maker:** Project owner (authorized continuation to the next roadmap phase)
- **Context:** Linear Regression consumes a samples-by-features matrix and one target per sample. The project needs a CSV boundary that validates untrusted input before exposing that representation.
- **Alternatives:** Add a general RFC 4180 parser; add a dependency; or support a documented strict numeric subset using the C standard library.
- **Decision:** Implement an opaque owning `MLDataset` and `ml_dataset_load_csv(path, options, out)`. The caller explicitly selects whether a header exists and selects one target by original column index. Getters expose borrowed read-only feature matrix, targets, dimensions, and optional column names. Caller destroys the dataset.
- **CSV grammar:** ASCII records separated by LF or CRLF; a final newline is optional. Comma is the only delimiter. Fields may have surrounding ASCII spaces/tabs. Quoted fields, escapes, embedded newlines, bare carriage returns, blank records, empty fields, and inconsistent row widths are rejected. A header, when enabled, uses unique ASCII identifiers matching `[A-Za-z_][A-Za-z0-9_]*`. Data fields use decimal syntax `[+-]?((digits(\.digits*)?)|(\.digits+))([eE][+-]?digits)?`; NaN/Infinity and numeric overflow/underflow are numeric errors. Conversion uses `strtod`, so the process numeric locale must use `.` as its decimal point (the standard C locale).
- **Limits:** 16 MiB per file; 64 KiB per record; 4096 bytes per field; 4096 columns; 100,000 data rows; 2,000,000 total numeric cells. Oversized or malformed input returns `ML_STATUS_PARSE_ERROR`; I/O failure returns `ML_STATUS_IO_ERROR`; allocation failure returns `ML_STATUS_OUT_OF_MEMORY`; non-finite/out-of-range numeric values return `ML_STATUS_NUMERIC_ERROR`.
- **Consequences:** This API safely supports the project's numeric training datasets and keeps target selection independent of feature order. It intentionally does not accept general text CSV; quoted strings, locale-specific decimals, and embedded newlines require a future API/format decision.
- **Modules/APIs affected:** `include/dataset.h`, `src/dataset.c`, `include/linear_regression.h` consumer contract, CLI and examples in later phases.
