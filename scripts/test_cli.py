"""Exercise the built CLI as a subprocess on Windows, macOS, or Linux."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path


def run(executable: Path, *arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(executable), *arguments],
        check=False,
        capture_output=True,
        text=True,
    )


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python scripts/test_cli.py <executable>", file=sys.stderr)
        return 2

    executable = Path(sys.argv[1]).resolve()
    if not executable.is_file():
        print(f"Executable not found: {executable}", file=sys.stderr)
        return 2

    cases = (
        (("--help",), 0, "Usage:", ""),
        (("--version",), 0, "c-machine-learning-lab 0.1.0", ""),
        (("--invalid",), 2, "", "unknown argument: --invalid"),
        (("--help", "--version"), 2, "", "--help does not accept additional arguments"),
    )
    for arguments, expected_code, expected_stdout, expected_stderr in cases:
        result = run(executable, *arguments)
        if result.returncode != expected_code:
            print(
                f"FAIL {arguments}: exit {result.returncode}, expected {expected_code}",
                file=sys.stderr,
            )
            return 1
        if expected_stdout not in result.stdout or expected_stderr not in result.stderr:
            print(f"FAIL {arguments}: unexpected output", file=sys.stderr)
            print(f"stdout: {result.stdout!r}", file=sys.stderr)
            print(f"stderr: {result.stderr!r}", file=sys.stderr)
            return 1
        print(f"PASS CLI {arguments}")

    with tempfile.TemporaryDirectory(prefix="cml-cli-", dir=executable.parent) as temporary:
        directory = Path(temporary)
        train_csv = directory / "linear-train.csv"
        linear_model = directory / "linear.model"
        predict_csv = directory / "predict.csv"
        train_csv.write_text("x,y\n0,1\n1,3\n2,5\n", encoding="ascii")
        predict_csv.write_text("3\n4\n", encoding="ascii")
        trained = run(
            executable,
            "train", "--type", "linear", "--data", str(train_csv), "--target", "1",
            "--header", "--model", str(linear_model), "--iterations", "3000",
            "--learning-rate", "0.05", "--tolerance", "1e-12",
        )
        if trained.returncode != 0 or not linear_model.is_file():
            print(f"FAIL linear train: {trained.returncode} {trained.stdout!r} {trained.stderr!r}", file=sys.stderr)
            return 1
        predicted = run(executable, "predict", "--model", str(linear_model), "--input", str(predict_csv))
        try:
            values = [float(line) for line in predicted.stdout.splitlines()]
        except ValueError:
            values = []
        if predicted.returncode != 0 or len(values) != 2 or abs(values[0] - 7.0) > 0.02 or abs(values[1] - 9.0) > 0.02:
            print(f"FAIL linear predict: {predicted.returncode} {predicted.stdout!r} {predicted.stderr!r}", file=sys.stderr)
            return 1
        print("PASS CLI linear train/save/load/predict")

        logistic_train = directory / "logistic-train.csv"
        logistic_model = directory / "logistic.model"
        logistic_train.write_text("x,label\n-2,0\n-1,0\n1,1\n2,1\n", encoding="ascii")
        trained = run(
            executable,
            "train", "--type", "logistic", "--data", str(logistic_train), "--target", "1",
            "--header", "--model", str(logistic_model), "--iterations", "1200",
            "--learning-rate", "0.1",
        )
        predicted = run(executable, "predict", "--model", str(logistic_model), "--input", str(predict_csv))
        if trained.returncode != 0 or predicted.returncode != 0:
            print(f"FAIL logistic flow: {trained.stderr!r} {predicted.stderr!r}", file=sys.stderr)
            return 1
        lines = predicted.stdout.splitlines()
        if len(lines) != 2 or not lines[0].endswith(",1") or not lines[1].endswith(",1"):
            print(f"FAIL logistic predictions: {predicted.stdout!r}", file=sys.stderr)
            return 1
        print("PASS CLI logistic train/save/load/predict")

        bad_csv = directory / "bad.csv"
        bad_csv.write_text("1,2\n3\n", encoding="ascii")
        malformed = run(executable, "predict", "--model", str(linear_model), "--input", str(bad_csv))
        mismatch = run(executable, "predict", "--model", str(linear_model), "--input", str(logistic_train))
        bad_number_csv = directory / "bad-number.csv"
        bad_number_csv.write_text("NaN\n", encoding="ascii")
        non_finite = run(executable, "predict", "--model", str(linear_model), "--input", str(bad_number_csv))
        invalid_threshold = run(
            executable, "predict", "--model", str(logistic_model), "--input", str(predict_csv),
            "--threshold", "2",
        )
        linear_threshold = run(
            executable, "predict", "--model", str(linear_model), "--input", str(predict_csv),
            "--threshold", "0.5",
        )
        invalid_train = run(executable, "train", "--type", "linear")
        excessive_work = run(
            executable, "train", "--type", "linear", "--data", str(train_csv), "--target", "1",
            "--header", "--model", str(linear_model), "--iterations", "1000001",
        )
        if (malformed.returncode != 1 or mismatch.returncode != 1 or non_finite.returncode != 1 or
                invalid_threshold.returncode != 2 or linear_threshold.returncode != 2 or invalid_train.returncode != 2 or
                excessive_work.returncode != 2):
            print("FAIL CLI error exit codes", file=sys.stderr)
            return 1
        print("PASS CLI malformed input, dimensions, and argument errors")

        # Large-magnitude feature: raw gradient descent at this rate diverges,
        # so a good fit shows standardization is applied in train and predict.
        scaled_train = directory / "scaled-train.csv"
        scaled_model = directory / "scaled.model"
        scaled_predict = directory / "scaled-predict.csv"
        scaled_train.write_text(
            "x,y\n" + "".join(f"{x * 1000},{2 * x + 1}\n" for x in range(20)), encoding="ascii"
        )
        scaled_predict.write_text("25000\n", encoding="ascii")
        raw = run(
            executable,
            "train", "--type", "linear", "--data", str(scaled_train), "--target", "1", "--header",
            "--model", str(directory / "raw.model"), "--iterations", "3000", "--learning-rate", "0.1",
        )
        if raw.returncode != 1 or (directory / "raw.model").exists():
            print(f"FAIL CLI raw large-magnitude training should diverge: {raw.stdout!r}", file=sys.stderr)
            return 1
        trained = run(
            executable,
            "train", "--type", "linear", "--data", str(scaled_train), "--target", "1", "--header",
            "--model", str(scaled_model), "--iterations", "3000", "--learning-rate", "0.1",
            "--normalize", "--validation-split", "0.25", "--seed", "7",
        )
        predicted = run(executable, "predict", "--model", str(scaled_model), "--input", str(scaled_predict))
        evaluated = run(
            executable, "evaluate", "--model", str(scaled_model), "--data", str(scaled_train),
            "--target", "1", "--header",
        )
        try:
            value = float(predicted.stdout.strip())
            r2 = float(evaluated.stdout.rsplit("R2=", 1)[1])
        except (ValueError, IndexError):
            value = r2 = float("nan")
        if (trained.returncode != 0 or "on 15 rows" not in trained.stdout or
                "Training: rows=15 " not in trained.stdout or "Validation: rows=5 " not in trained.stdout or
                not scaled_model.read_text(encoding="ascii").startswith("CMLMODEL 2\n") or
                predicted.returncode != 0 or abs(value - 51.0) > 0.05 or
                evaluated.returncode != 0 or "Evaluation: rows=20 " not in evaluated.stdout or
                not r2 > 0.9999):
            print(
                f"FAIL CLI normalize/split/evaluate: {trained.stdout!r} {trained.stderr!r} "
                f"{predicted.stdout!r} {evaluated.stdout!r} {evaluated.stderr!r}",
                file=sys.stderr,
            )
            return 1
        print("PASS CLI normalize, validation split, and evaluate")

        evaluated = run(
            executable, "evaluate", "--model", str(logistic_model), "--data", str(logistic_train),
            "--target", "1", "--header", "--threshold", "0.5",
        )
        if (evaluated.returncode != 0 or
                "accuracy=1 precision=1 recall=1 F1=1 TP=2 TN=2 FP=0 FN=0" not in evaluated.stdout):
            print(f"FAIL CLI logistic evaluate: {evaluated.stdout!r} {evaluated.stderr!r}", file=sys.stderr)
            return 1
        print("PASS CLI logistic evaluate")

        seed_without_split = run(
            executable, "train", "--type", "linear", "--data", str(train_csv), "--target", "1",
            "--header", "--model", str(linear_model), "--seed", "3",
        )
        bad_fraction = run(
            executable, "train", "--type", "linear", "--data", str(train_csv), "--target", "1",
            "--header", "--model", str(linear_model), "--validation-split", "1",
        )
        empty_train = run(
            executable, "train", "--type", "linear", "--data", str(train_csv), "--target", "1",
            "--header", "--model", str(linear_model), "--validation-split", "0.99",
        )
        evaluate_missing_target = run(
            executable, "evaluate", "--model", str(linear_model), "--data", str(train_csv),
        )
        evaluate_linear_threshold = run(
            executable, "evaluate", "--model", str(linear_model), "--data", str(train_csv),
            "--target", "1", "--header", "--threshold", "0.5",
        )
        mismatch_with_extra = directory / "wide.csv"
        mismatch_with_extra.write_text("1,2,3\n", encoding="ascii")
        evaluate_mismatch = run(
            executable, "evaluate", "--model", str(linear_model), "--data", str(mismatch_with_extra),
            "--target", "0",
        )
        if (seed_without_split.returncode != 2 or bad_fraction.returncode != 2 or
                empty_train.returncode != 1 or evaluate_missing_target.returncode != 2 or
                evaluate_linear_threshold.returncode != 2 or evaluate_mismatch.returncode != 1):
            print("FAIL CLI split/evaluate error exit codes", file=sys.stderr)
            return 1
        print("PASS CLI split/evaluate argument and data errors")

        cv = run(
            executable, "cross-validate", "--type", "linear", "--data", str(scaled_train), "--target", "1",
            "--header", "--folds", "4", "--seed", "3", "--normalize", "--iterations", "3000",
            "--learning-rate", "0.1",
        )
        fold_lines = [line for line in cv.stdout.splitlines() if line.startswith("Fold ")]
        try:
            mean_r2 = float(cv.stdout.split("R2=")[-1].split("+/-")[0])
        except (ValueError, IndexError):
            mean_r2 = float("nan")
        cv_again = run(
            executable, "cross-validate", "--type", "linear", "--data", str(scaled_train), "--target", "1",
            "--header", "--folds", "4", "--seed", "3", "--normalize", "--iterations", "3000",
            "--learning-rate", "0.1",
        )
        cv_logistic = run(
            executable, "cross-validate", "--type", "logistic", "--data", str(logistic_train), "--target", "1",
            "--header", "--folds", "2", "--iterations", "1200", "--learning-rate", "0.1",
        )
        if (cv.returncode != 0 or len(fold_lines) != 4 or "Mean over 4 folds" not in cv.stdout or
                not mean_r2 > 0.999 or cv_again.stdout != cv.stdout or cv_logistic.returncode != 0 or
                "accuracy=" not in cv_logistic.stdout):
            print(f"FAIL CLI cross-validate: {cv.stdout!r} {cv.stderr!r} {cv_logistic.stderr!r}", file=sys.stderr)
            return 1
        print("PASS CLI cross-validate (deterministic folds, per-fold scaling)")

        ridge_model = directory / "ridge.model"
        ridge = run(
            executable, "train", "--type", "linear", "--data", str(train_csv), "--target", "1", "--header",
            "--model", str(ridge_model), "--iterations", "20000", "--learning-rate", "0.05",
            "--tolerance", "1e-15", "--l2", "1",
        )
        ridge_pred = run(executable, "predict", "--model", str(ridge_model), "--input", str(predict_csv))
        try:
            ridge_value = float(ridge_pred.stdout.split()[0])
        except (ValueError, IndexError):
            ridge_value = float("nan")
        # x = 0,1,2 and y = 2x+1 with l2 = 1: w = cov/(var+l2) = (4/3)/(2/3+1) = 0.8, b = 3 - 0.8 = 2.2.
        bad_cv = [
            run(executable, "cross-validate", "--type", "linear", "--data", str(train_csv), "--target", "1",
                "--header", "--folds", "1"),
            run(executable, "cross-validate", "--type", "linear", "--data", str(train_csv), "--target", "1",
                "--header", "--model", str(ridge_model)),
            run(executable, "train", "--type", "linear", "--data", str(train_csv), "--target", "1",
                "--header", "--model", str(ridge_model), "--l2", "-1"),
            run(executable, "train", "--type", "linear", "--data", str(train_csv), "--target", "1",
                "--header", "--model", str(ridge_model), "--folds", "3"),
        ]
        too_many_folds = run(executable, "cross-validate", "--type", "linear", "--data", str(train_csv),
                             "--target", "1", "--header", "--folds", "4")
        if (ridge.returncode != 0 or abs(ridge_value - (0.8 * 3 + 2.2)) > 1e-3 or
                any(r.returncode != 2 for r in bad_cv) or too_many_folds.returncode != 2):
            print(f"FAIL CLI l2/cv errors: {ridge_pred.stdout!r} {[r.returncode for r in bad_cv]} "
                  f"{too_many_folds.returncode}", file=sys.stderr)
            return 1
        print("PASS CLI --l2 ridge value and cross-validate argument errors")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
