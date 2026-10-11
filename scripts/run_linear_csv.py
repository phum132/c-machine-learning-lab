#!/usr/bin/env python3
"""Train CML Linear Regression on a numeric CSV and chart held-out predictions."""

from __future__ import annotations

import argparse
import csv
import math
import random
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAX_INPUT_BYTES = 16 * 1024 * 1024
MAX_ROWS = 100_000
MAX_CELLS = 2_000_000
TRAIN_CELL_UPDATE_LIMIT = 250_000_000


def read_dataset(path: Path, target_column: int, has_header: bool) -> tuple[list[list[float]], int]:
    if path.stat().st_size > MAX_INPUT_BYTES:
        raise ValueError("CSV is over the CML 16 MiB input limit")
    content = path.read_text(encoding="utf-8-sig")
    if '"' in content:
        raise ValueError("Quoted CSV fields are not supported; remove quotes around values")
    rows: list[list[float]] = []
    column_count: int | None = None
    for line_number, record in enumerate(csv.reader(content.splitlines()), start=1):
        if line_number == 1 and has_header:
            if not record or any(not name.strip() for name in record):
                raise ValueError("Header has an empty column name")
            column_count = len(record)
            continue
        if not record or any(not field.strip() for field in record):
            raise ValueError(f"Blank row or missing value at line {line_number}")
        if column_count is None:
            column_count = len(record)
        if len(record) != column_count:
            raise ValueError(f"Inconsistent number of columns at line {line_number}")
        try:
            row = [float(field.strip()) for field in record]
        except ValueError as error:
            raise ValueError(f"Non-numeric value at line {line_number}") from error
        if not all(math.isfinite(value) for value in row):
            raise ValueError(f"Non-finite value at line {line_number}")
        rows.append(row)
        if len(rows) > MAX_ROWS:
            raise ValueError("CSV is over the CML 100,000 row limit")
    if column_count is None or len(rows) < 5:
        raise ValueError("Need a header (if selected) and at least five numeric data rows")
    if target_column < 0 or target_column >= column_count:
        raise ValueError(f"Target index must be from 0 to {column_count - 1}")
    if column_count < 2:
        raise ValueError("Need at least one feature column and one target column")
    if len(rows) * column_count > MAX_CELLS:
        raise ValueError("Dataset exceeds the CML 2,000,000 numeric cell limit")
    return rows, column_count


def write_csv(path: Path, rows: list[list[float]], header: list[str] | None = None) -> None:
    with path.open("w", encoding="ascii", newline="") as file:
        writer = csv.writer(file, lineterminator="\n")
        if header:
            writer.writerow(header)
        writer.writerows([[format(value, ".12g") for value in row] for row in rows])


def read_prediction_rows(path: Path, expected_columns: int) -> list[list[float]]:
    if path.stat().st_size > MAX_INPUT_BYTES:
        raise ValueError("Prediction CSV is over the 16 MiB input limit")
    content = path.read_text(encoding="utf-8-sig")
    if '"' in content:
        raise ValueError("Quoted CSV fields are not supported; remove quotes around values")
    rows: list[list[float]] = []
    for line_number, record in enumerate(csv.reader(content.splitlines()), start=1):
        if not record or any(not field.strip() for field in record):
            raise ValueError(f"Blank row or missing value in prediction CSV at line {line_number}")
        if len(record) != expected_columns:
            raise ValueError(f"Prediction CSV needs exactly {expected_columns} feature columns")
        try:
            row = [float(field.strip()) for field in record]
        except ValueError as error:
            raise ValueError(f"Non-numeric prediction value at line {line_number}") from error
        if not all(math.isfinite(value) for value in row):
            raise ValueError(f"Non-finite prediction value at line {line_number}")
        rows.append(row)
        if len(rows) > MAX_ROWS:
            raise ValueError("Prediction CSV is over the 100,000 row limit")
    if not rows:
        raise ValueError("Prediction CSV is empty")
    return rows


def make_svg(actual: list[float], predicted: list[float], mae: float,
             rmse: float, r_squared: float, train_count: int) -> str:
    width, height = 900, 680
    left, top, plot = 90.0, 85.0, 520.0
    low = min(min(actual), min(predicted))
    high = max(max(actual), max(predicted))
    span = high - low
    padding = span * 0.08 if span > 0.0 else 1.0
    low -= padding
    high += padding

    def sx(value: float) -> float:
        return left + (value - low) * plot / (high - low)

    def sy(value: float) -> float:
        return top + plot - (value - low) * plot / (high - low)

    ticks = []
    for i in range(6):
        value = low + (high - low) * i / 5
        x, y = sx(value), sy(value)
        ticks.append(f'<line x1="{x:.1f}" y1="{top}" x2="{x:.1f}" y2="{top+plot}" stroke="#e2e8f0"/>')
        ticks.append(f'<line x1="{left}" y1="{y:.1f}" x2="{left+plot}" y2="{y:.1f}" stroke="#e2e8f0"/>')
        ticks.append(f'<text x="{x:.1f}" y="{top+plot+23}" text-anchor="middle">{value:.3g}</text>')
        ticks.append(f'<text x="{left-12}" y="{y+5:.1f}" text-anchor="end">{value:.3g}</text>')
    points = "".join(
        f'<circle cx="{sx(truth):.2f}" cy="{sy(guess):.2f}" r="3.2" fill="#2877b7" fill-opacity="0.58"/>'
        for truth, guess in zip(actual, predicted, strict=True)
    )
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<style>text{{font:14px Arial,sans-serif;fill:#243247}}.title{{font-size:24px;font-weight:bold}}.axis{{font-size:16px;font-weight:bold}}</style>
<rect width="100%" height="100%" fill="white"/><text class="title" x="90" y="38">Linear Regression: actual vs predicted</text>
<text x="90" y="62">Held-out test rows: {len(actual)} · training rows: {train_count}</text>
{''.join(ticks)}
<line x1="{sx(low)}" y1="{sy(low)}" x2="{sx(high)}" y2="{sy(high)}" stroke="#ce3b37" stroke-width="2" stroke-dasharray="7 6"/>
{points}
<text class="axis" x="{left+plot/2}" y="{top+plot+60}" text-anchor="middle">Actual target value</text>
<text class="axis" transform="translate(25 {top+plot/2}) rotate(-90)" text-anchor="middle">Predicted target value</text>
<rect x="650" y="95" width="220" height="120" rx="8" fill="#f4f7fa" stroke="#d7e0ea"/>
<text x="668" y="128">MAE: {mae:.4g}</text><text x="668" y="158">RMSE: {rmse:.4g}</text><text x="668" y="188">R²: {r_squared:.4g}</text>
<text x="645" y="240">Dashed line = exact prediction</text></svg>
'''


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", required=True, type=Path, help="numeric training CSV")
    parser.add_argument("--target", required=True, type=int, help="zero-based target column index")
    parser.add_argument("--header", action="store_true", help="CSV has one header row")
    parser.add_argument("--predict", type=Path,
                        help="optional feature-only CSV to predict after training (no header or target column)")
    parser.add_argument("--output", type=Path, default=Path("build/user-regression"),
                        help="output folder (default: build/user-regression)")
    parser.add_argument("--test-fraction", type=float, default=0.2)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--iterations", type=int, default=3000)
    parser.add_argument("--learning-rate", type=float, default=0.05)
    args = parser.parse_args()

    cli = ROOT / "build" / "cml-lab.exe"
    if not cli.is_file():
        raise ValueError("CML CLI not found; run .\\build.bat from the repository root first")
    if not 0.05 <= args.test_fraction <= 0.5:
        raise ValueError("--test-fraction must be between 0.05 and 0.5")
    if args.iterations < 1 or args.iterations > 1_000_000:
        raise ValueError("--iterations must be from 1 to 1,000,000")
    rows, column_count = read_dataset(args.data.resolve(), args.target, args.header)
    feature_columns = [index for index in range(column_count) if index != args.target]
    random.Random(args.seed).shuffle(rows)
    split = max(2, min(len(rows) - 2, int(len(rows) * (1.0 - args.test_fraction))))
    train, test = rows[:split], rows[split:]
    means = [sum(row[col] for row in train) / len(train) for col in feature_columns]
    scales = []
    for col, mean in zip(feature_columns, means, strict=True):
        variance = sum((row[col] - mean) ** 2 for row in train) / len(train)
        scales.append(math.sqrt(variance) or 1.0)

    def features(row: list[float]) -> list[float]:
        return [(row[col] - mean) / scale for col, mean, scale in zip(feature_columns, means, scales, strict=True)]

    out_dir = args.output if args.output.is_absolute() else ROOT / args.output
    out_dir.mkdir(parents=True, exist_ok=True)
    train_path = out_dir / "train.csv"
    test_features_path = out_dir / "test-features.csv"
    actual_path = out_dir / "test-actual.csv"
    model_path = out_dir / "model.cml"
    predictions_path = out_dir / "predictions.csv"
    write_csv(train_path, [features(row) + [row[args.target]] for row in train],
              [f"feature_{i}" for i in range(len(feature_columns))] + ["target"])
    write_csv(test_features_path, [features(row) for row in test])
    actual = [row[args.target] for row in test]
    write_csv(actual_path, [[value] for value in actual])
    safe_iterations = min(args.iterations, max(1, TRAIN_CELL_UPDATE_LIMIT // (len(train) * len(feature_columns))))
    subprocess.run([str(cli), "train", "--type", "linear", "--data", str(train_path),
                    "--target", str(len(feature_columns)), "--header", "--model", str(model_path),
                    "--iterations", str(safe_iterations), "--learning-rate", str(args.learning_rate)], check=True)
    with predictions_path.open("w", encoding="ascii", newline="") as output:
        subprocess.run([str(cli), "predict", "--model", str(model_path), "--input", str(test_features_path)],
                       check=True, stdout=output)
    predicted = [float(value) for value in predictions_path.read_text(encoding="ascii").splitlines()]
    errors = [guess - truth for guess, truth in zip(predicted, actual, strict=True)]
    mae = sum(abs(error) for error in errors) / len(errors)
    rmse = math.sqrt(sum(error * error for error in errors) / len(errors))
    target_mean = sum(actual) / len(actual)
    total_sum_squares = sum((value - target_mean) ** 2 for value in actual)
    r_squared = 1.0 - sum(error * error for error in errors) / total_sum_squares if total_sum_squares else 0.0
    (out_dir / "prediction.svg").write_text(make_svg(actual, predicted, mae, rmse, r_squared, len(train)), encoding="utf-8")
    report = (f"Rows: {len(rows)}; train/test: {len(train)}/{len(test)}\n"
              f"Target column: {args.target}; features: {len(feature_columns)}\n"
              f"MAE: {mae:.6f}\nRMSE: {rmse:.6f}\nR2: {r_squared:.6f}\n")
    (out_dir / "metrics.txt").write_text(report, encoding="utf-8")
    print(report)
    print(f"Graph: {out_dir / 'prediction.svg'}")
    print(f"Predictions: {predictions_path}")
    if args.predict is not None:
        new_rows = read_prediction_rows(args.predict.resolve(), len(feature_columns))
        new_features_path = out_dir / "new-features.csv"
        new_predictions_path = out_dir / "new-predictions.csv"
        scaled_rows = [[(row[i] - mean) / scale for i, (mean, scale) in enumerate(zip(means, scales, strict=True))]
                       for row in new_rows]
        write_csv(new_features_path, scaled_rows)
        with new_predictions_path.open("w", encoding="ascii", newline="") as output:
            subprocess.run([str(cli), "predict", "--model", str(model_path), "--input", str(new_features_path)],
                           check=True, stdout=output)
        print(f"New-data predictions: {new_predictions_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1)
