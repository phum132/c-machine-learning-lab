# Phase K handoff: faster training, L2, cross-validation

## Summary

Training loops now read the data once per iteration and validate inputs once per call. Linear and logistic regression gained an L2 (ridge) penalty, and the CLI gained `--l2` and a `cross-validate` command. With `l2 = 0` the trained parameters, loss history, and saved model files are identical to before.

## Files Changed

- `src/linear_regression.c`, `src/logistic_regression.c` — single-pass loss + gradient per iteration; L2 term in loss and gradient (bias not penalized); one finiteness check per pass instead of per feature.
- `include/linear_regression.h`, `include/logistic_regression.h` — `double l2` appended to both config structs.
- `include/cross_validation.h`, `src/cross_validation.c` — `ml_kfold_assign` (seeded shuffle, fold sizes differ by at most one).
- `src/app_commands.c`, `src/app_cli.c` — `--l2`, `cross-validate --folds K [--seed N]` with per-fold scaling fitted on training rows only; output uses ASCII `+/-`.
- `tests/test_cross_validation.c`, `tests/test_regularization.c`, `tests/test_main.c`, existing tests' config initializers, `scripts/test_cli.py`.
- `Makefile`, `build.bat`, `README.md`, `docs/architecture.md`, `docs/implementation-roadmap.md`.

## Interface Changes

- `MLLinearRegressionConfig` and `MLLogisticRegressionConfig` have a new last member `double l2`. Positional initializers that omit it still compile and mean `l2 = 0`, but `-Wextra` warns; all in-repo initializers now set it explicitly. Negative or non-finite `l2` returns `ML_STATUS_INVALID_ARGUMENT`.
- With `l2 > 0`, the recorded history is the regularized loss.
- New public function `ml_kfold_assign`. New CLI command `cross-validate`.

## Verification

- `build.bat test` — exit 0: 12 C suites, 12 CLI subprocess checks, infrastructure validation.
- Existing exact-value tests (one-step MSE 1.685, one-step cross-entropy log1p(e^-0.1)) still pass unchanged.
- Ridge closed form: x = 0..3, y = 2x + 1, l2 = 1 → w = 2.5/2.25, b = 4 − 1.5w, matched within 1e-6 (C test); CLI ridge on x = 0..2 predicts 0.8·3 + 2.2 within 1e-3.
- Benchmark, 20,000 rows × 10 features, 1,000 iterations, same machine: linear 2.13 s → 0.80 s (2.7×), logistic 3.42 s → 1.61 s (2.1×). Model files byte-identical to the previous build.
- WSL Ubuntu mirror of CI: validate, `make test`, finance tests, `make test-analyzer`, `make test-sanitize` (ASan/UBSan/LeakSanitizer) — all exit 0.

## Issues

- Hosted GitHub Actions has not run on these changes yet.

## Risks

- External code that positionally initializes the config structs and builds with `-Wextra -Werror` will need the extra field.

## Next Steps

1. Phase L: decision tree, random forest, multi-class (softmax) models.
