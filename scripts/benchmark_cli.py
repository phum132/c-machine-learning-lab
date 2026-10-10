"""Repeatable end-to-end benchmark for linear-regression CLI training."""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def compiler_version(compiler: str) -> str:
    command = shutil.which(compiler) or (compiler if Path(compiler).is_file() else None)
    if command is None:
        return "not found (pass --compiler with the build compiler path)"
    try:
        result = subprocess.run([command, "--version"], capture_output=True, text=True, check=False)
    except OSError as exc:
        return f"version query unavailable: {exc.__class__.__name__}"
    if result.returncode != 0:
        return f"version query failed (exit {result.returncode})"
    return result.stdout.splitlines()[0] if result.stdout else "version output unavailable"


def run(executable: Path, *arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run([str(executable), *arguments], capture_output=True, text=True, check=False)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True, help="built cml-lab executable")
    parser.add_argument("--compiler", default="gcc", help="compiler used for the executable")
    parser.add_argument("--rows", type=int, default=5000)
    parser.add_argument("--iterations", type=int, default=200)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--repetitions", type=int, default=5)
    args = parser.parse_args()

    executable = args.exe.resolve()
    if (not executable.is_file() or args.rows < 2 or args.rows > 100_000 or
            args.iterations < 1 or args.iterations > 1_000_000 or
            args.rows * args.iterations > 250_000_000 or args.warmups < 0 or args.repetitions < 1):
        parser.error("executable must exist; rows, iterations, and repetitions must fit the CLI limits; warmups must be nonnegative")
    compiler = compiler_version(args.compiler)
    cli_version = run(executable, "--version")
    if cli_version.returncode != 0:
        print(f"FAIL: CLI --version: {cli_version.stderr}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="cml-benchmark-", dir=executable.parent) as temporary:
        root = Path(temporary)
        data_path = root / "train.csv"
        model_path = root / "model.cml"
        input_path = root / "predict.csv"
        lines = ["x,y"]
        for index in range(args.rows):
            x_value = ((index % 1000) - 500) / 500.0
            lines.append(f"{x_value:.6f},{2.0 * x_value + 1.0:.6f}")
        data = ("\n".join(lines) + "\n").encode("ascii")
        data_path.write_bytes(data)
        input_path.write_text("0.25\n", encoding="ascii")
        command = (
            "train", "--type", "linear", "--data", str(data_path), "--target", "1",
            "--header", "--model", str(model_path), "--iterations", str(args.iterations),
            "--learning-rate", "0.05", "--tolerance", "0",
        )
        for _ in range(args.warmups):
            result = run(executable, *command)
            if result.returncode != 0:
                print(f"FAIL: warm-up training: {result.stderr}", file=sys.stderr)
                return 1

        durations: list[float] = []
        for repetition in range(args.repetitions):
            started = time.perf_counter()
            result = run(executable, *command)
            durations.append(time.perf_counter() - started)
            if result.returncode != 0:
                print(f"FAIL: repetition {repetition + 1}: {result.stderr}", file=sys.stderr)
                return 1

        prediction = run(executable, "predict", "--model", str(model_path), "--input", str(input_path))
        try:
            predicted_value = float(prediction.stdout.strip())
        except ValueError:
            print(f"FAIL: prediction output: {prediction.stdout!r} {prediction.stderr!r}", file=sys.stderr)
            return 1
        if prediction.returncode != 0 or abs(predicted_value - 1.5) > 0.02:
            print(f"FAIL: correctness check expected 1.5 ± 0.02, got {predicted_value}", file=sys.stderr)
            return 1

    report = {
        "question": "End-to-end wall time for CLI linear-regression training and model serialization",
        "baseline": "same executable, generated deterministic dataset, repeated process runs; no prior version comparison",
        "environment": {
            "os": platform.platform(),
            "machine": platform.machine(),
            "processor": platform.processor() or "not available",
            "python": platform.python_version(),
            "compiler": compiler,
            "cli_version": cli_version.stdout.strip(),
            "compiler_flags": "-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow; compiler default optimization level",
        },
        "input": {
            "rows": args.rows,
            "features": 1,
            "target": "y = 2x + 1",
            "generation": "x = ((row_index mod 1000) - 500) / 500; no randomness",
            "sha256": hashlib.sha256(data).hexdigest(),
            "bytes": len(data),
            "iterations": args.iterations,
            "learning_rate": 0.05,
        },
        "method": {
            "warmups": args.warmups,
            "repetitions": args.repetitions,
            "clock": "time.perf_counter",
            "timed_scope": "subprocess startup, CSV loading, training, and model save",
            "summary": "median and minimum of raw wall times",
        },
        "raw_seconds": durations,
        "median_seconds": statistics.median(durations),
        "minimum_seconds": min(durations),
        "correctness": {"input_x": 0.25, "expected": 1.5, "actual": predicted_value, "tolerance": 0.02},
        "limitations": ["CLI process and filesystem overhead are included", "single local machine and synthetic one-feature dataset", "not a comparison against another implementation"],
    }
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
