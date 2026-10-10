# Architecture

This document describes the implementation in the repository today. The public C API is declared in `include/`; implementations live in `src/`. Callers own objects returned by create/load functions unless a getter explicitly says it returns a borrowed view.

## Module map

| Module | Responsibility | Main dependency |
|---|---|---|
| `ml_error` | Shared `MLStatus` values and status messages | C standard library |
| `matrix` | Contiguous row-major float matrices and operations | `ml_error` |
| `dataset` | Strict numeric CSV parsing into features, targets, and optional column names | `matrix`, `ml_error` |
| `linear_regression` | Linear model, prediction, MSE, full-batch gradient descent, seeded split | `matrix`, `ml_error` |
| `logistic_regression` | Binary model, stable sigmoid/cross-entropy, prediction and training history | `matrix`, `ml_error` |
| `classification_metrics` | Binary confusion counts, accuracy, precision, recall, and F1 | `ml_error` |
| `model_serialization` | Versioned ASCII save/load for linear and logistic models | model APIs, `ml_error` |
| `app_cli`, `app_commands` | Argument parsing and `train`/`predict` workflows | dataset, models, serialization |

`main.c` is the executable entry point. The test suite uses a small C test harness, subprocess CLI checks, and `scripts/validate_infrastructure.py`. The build has no third-party C library dependency.

## Data flow

```text
CSV -> dataset -> model training -> model serialization
                         model file -> load -> feature CSV -> predictions
```

Matrices store `float` values in row-major order. Model APIs accept samples-by-features matrices. Dataset getters expose borrowed read-only views; the dataset retains ownership. Matrix and model create/load functions return caller-owned objects, released with the matching destroy function. Operations that return matrices allocate a new result and do not mutate inputs.

## Input and numerical behavior

- Public APIs validate pointers, dimensions, and finite numeric values where required. Allocation-size multiplication is checked before allocation.
- CSV and serialized model input are treated as untrusted. The CSV parser accepts numeric unquoted records and rejects missing values, blank records, inconsistent widths, and non-finite values. See [ADR-001](adr-001-dataset-csv.md) for grammar and caps.
- Logistic regression accepts binary labels `0` and `1`, uses a numerically stable sigmoid and binary cross-entropy, and applies the documented threshold rule in [ADR-002](adr-002-logistic-regression.md).
- Model files use a versioned ASCII format with hexadecimal floating-point values and configured size/feature limits. The process must use the C numeric locale. See [ADR-003](adr-003-model-serialization.md).
- Training stopping criteria indicate loss change within tolerance for the requested patience; they do not guarantee a global optimum. Model training rolls parameters back when an update fails numerically.

## Verification and limits

Run `build.bat test` on Windows or `make test` on POSIX. Windows GCC analyzer and Clang sanitizer invocations are recorded in [Phase H handoff](phase-h-handoff.md). GitHub Actions is configured in [ci.yml](../.github/workflows/ci.yml); a successful hosted run must be verified in GitHub and is not inferred from the local workflow configuration.

The CLI trains on all rows supplied to it and does not expose normalization, validation splitting, or batch metrics. It currently predicts one feature CSV at a time. These are implementation limits, not promised future behavior.
