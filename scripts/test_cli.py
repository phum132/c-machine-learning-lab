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

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
