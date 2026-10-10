# ADR-002: Binary logistic regression and classification metrics

- **Status:** Accepted for Phase E
- **Date:** 2026-10-10
- **Decision maker:** Project owner (authorized continuation to the next roadmap phase)
- **Context:** The project needs deterministic binary classification APIs with explicit label, threshold, objective, stopping, and zero-division behavior.
- **Alternatives:** Store binary labels as booleans/integers; infer labels by rounding; use a general multi-class model; or use exact numeric labels and a strictly binary API.
- **Decision:** `MLLogisticRegression` initializes weights and bias to zero and accepts labels exactly `0.0` or `1.0`. It trains with full-batch gradient descent on mean binary cross-entropy. `MLLogisticHistory` contains the initial loss and each completed update. Training stops after `patience` consecutive absolute loss changes no greater than `tolerance`, or at `max_iterations`; this is a stability condition, not a monotonic-decrease or convergence guarantee. Numeric failure rolls parameters back.
- **Numerical formulas:** Sigmoid is evaluated with a sign-dependent exponential. Per-sample binary cross-entropy is `max(z,0) - z*y + log1p(exp(-abs(z)))`. Gradients are the sample mean of `(sigmoid(z)-y)*x` and `(sigmoid(z)-y)` for weights and bias.
- **Prediction policy:** `predict_probabilities` returns an N-by-1 matrix. `predict_classes` accepts a finite threshold in `[0,1]` and predicts class 1 when probability is greater than or equal to the threshold.
- **Metrics policy:** Metrics consume exact binary labels and predicted classes. Accuracy is correct/total. Precision, recall, and F1 are `0.0` when their respective denominator is zero. Empty inputs and non-binary/non-finite values are invalid arguments.
- **Consequences:** Binary label assumptions and edge behavior are explicit and testable. Multi-class classification, regularization, and validation-set early stopping are outside Phase E.
- **Modules/APIs affected:** `include/logistic_regression.h`, `src/logistic_regression.c`, `include/classification_metrics.h`, `src/classification_metrics.c`.
