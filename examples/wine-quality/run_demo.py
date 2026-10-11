#!/usr/bin/env python3
"""Prepare, train, evaluate, and chart the public UCI Wine Quality dataset."""

from __future__ import annotations

import csv
import math
import random
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
DATA_DIR = HERE / "data"
OUT_DIR = ROOT / "build" / "wine-quality"
ZIP_URL = "https://archive.ics.uci.edu/static/public/186/wine+quality.zip"
ZIP_PATH = OUT_DIR / "wine-quality-uci.zip"
FEATURE_NAMES = [
    "fixed_acidity", "volatile_acidity", "citric_acid", "residual_sugar",
    "chlorides", "free_sulfur_dioxide", "total_sulfur_dioxide", "density",
    "pH", "sulphates", "alcohol", "is_white",
]
SOURCE_FEATURE_NAMES = [
    "fixed acidity", "volatile acidity", "citric acid", "residual sugar",
    "chlorides", "free sulfur dioxide", "total sulfur dioxide", "density",
    "pH", "sulphates", "alcohol",
]
SEED = 20261010


def ensure_source_files() -> None:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    source_files = [DATA_DIR / "winequality-red.csv", DATA_DIR / "winequality-white.csv"]
    if all(path.is_file() for path in source_files):
        return
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    print(f"Downloading public UCI data: {ZIP_URL}")
    urllib.request.urlretrieve(ZIP_URL, ZIP_PATH)
    with zipfile.ZipFile(ZIP_PATH) as archive:
        for name in ("winequality-red.csv", "winequality-white.csv"):
            (DATA_DIR / name).write_bytes(archive.read(name))


def load_records() -> list[tuple[list[float], float]]:
    records: list[tuple[list[float], float]] = []
    for name, is_white in (("winequality-red.csv", 0.0), ("winequality-white.csv", 1.0)):
        with (DATA_DIR / name).open("r", newline="", encoding="utf-8-sig") as file:
            reader = csv.DictReader(file, delimiter=";")
            required = SOURCE_FEATURE_NAMES + ["quality"]
            if reader.fieldnames != required:
                raise ValueError(f"Unexpected columns in {name}: {reader.fieldnames}")
            for row in reader:
                features = [float(row[key]) for key in SOURCE_FEATURE_NAMES] + [is_white]
                target = float(row["quality"])
                if not all(math.isfinite(value) for value in features + [target]):
                    raise ValueError(f"Non-finite value found in {name}")
                records.append((features, target))
    if len(records) != 6497:
        raise ValueError(f"Expected 6,497 UCI rows, got {len(records)}")
    return records


def write_csv(path: Path, rows: list[list[float]], header: list[str] | None = None) -> None:
    with path.open("w", newline="", encoding="ascii") as file:
        writer = csv.writer(file, lineterminator="\n")
        if header:
            writer.writerow(header)
        writer.writerows([[format(value, ".12g") for value in row] for row in rows])


def make_svg(actual: list[float], predicted: list[float], mae: float, rmse: float,
             r_squared: float, train_count: int) -> str:
    width, height = 920, 680
    left, top, plot = 90, 90, 550
    low, high = 2.5, 9.5

    def sx(value: float) -> float:
        return left + (value - low) * plot / (high - low)

    def sy(value: float) -> float:
        return top + plot - (value - low) * plot / (high - low)

    points = []
    # Plot a deterministic subset for legibility; metrics still use all test rows.
    stride = max(1, len(actual) // 700)
    for index in range(0, len(actual), stride):
        points.append(
            f'<circle cx="{sx(actual[index]):.2f}" cy="{sy(predicted[index]):.2f}" '
            'r="3" fill="#2673b8" fill-opacity="0.42" />'
        )
    ticks = []
    for value in range(3, 10):
        ticks.append(f'<line x1="{sx(value):.1f}" y1="{top}" x2="{sx(value):.1f}" '
                     f'y2="{top + plot}" stroke="#e1e7ef"/>')
        ticks.append(f'<line x1="{left}" y1="{sy(value):.1f}" x2="{left + plot}" '
                     f'y2="{sy(value):.1f}" stroke="#e1e7ef"/>')
        ticks.append(f'<text x="{sx(value):.1f}" y="{top + plot + 24}" text-anchor="middle">{value}</text>')
        ticks.append(f'<text x="{left - 16}" y="{sy(value) + 5:.1f}" text-anchor="end">{value}</text>')
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<style>text{{font:15px Arial,sans-serif;fill:#243247}}.title{{font-size:25px;font-weight:bold}}.subtitle{{font-size:15px;fill:#52647a}}.axis{{font-size:17px;font-weight:bold}}</style>
<rect width="100%" height="100%" fill="#ffffff"/>
<text class="title" x="90" y="38">Wine Quality: actual vs predicted</text>
<text class="subtitle" x="90" y="65">CML Linear Regression · {len(actual):,} held-out test samples · {train_count:,} training samples</text>
{''.join(ticks)}
<line x1="{sx(low)}" y1="{sy(low)}" x2="{sx(high)}" y2="{sy(high)}" stroke="#d43f3a" stroke-width="2" stroke-dasharray="7 6"/>
{''.join(points)}
<rect x="675" y="105" width="205" height="112" rx="8" fill="#f4f7fa" stroke="#d7e0ea"/>
<text x="692" y="137">MAE: {mae:.3f}</text><text x="692" y="167">RMSE: {rmse:.3f}</text><text x="692" y="197">R²: {r_squared:.3f}</text>
<text class="axis" x="{left + plot / 2}" y="{top + plot + 62}" text-anchor="middle">Actual quality score</text>
<text class="axis" transform="translate(26 {top + plot / 2}) rotate(-90)" text-anchor="middle">Predicted quality score</text>
<text class="subtitle" x="660" y="245">Dashed line = perfect prediction</text>
</svg>
'''


def main() -> int:
    cli = ROOT / "build" / "cml-lab.exe"
    if not cli.is_file():
        print("CML CLI is missing. Build it first with: .\\build.bat", file=sys.stderr)
        return 2
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    ensure_source_files()
    records = load_records()
    random.Random(SEED).shuffle(records)
    split = int(len(records) * 0.8)
    train, test = records[:split], records[split:]
    feature_count = len(FEATURE_NAMES)
    means = [sum(row[0][col] for row in train) / len(train) for col in range(feature_count)]
    scales = []
    for col, mean in enumerate(means):
        variance = sum((row[0][col] - mean) ** 2 for row in train) / len(train)
        scales.append(math.sqrt(variance) or 1.0)

    def standardized(sample: tuple[list[float], float]) -> list[float]:
        return [(sample[0][col] - means[col]) / scales[col] for col in range(feature_count)]

    train_path = OUT_DIR / "train.csv"
    feature_path = OUT_DIR / "test-features.csv"
    actual_path = OUT_DIR / "test-actual.csv"
    model_path = OUT_DIR / "wine-quality.cml"
    prediction_path = OUT_DIR / "prediction.csv"
    write_csv(train_path, [standardized(row) + [row[1]] for row in train], FEATURE_NAMES + ["quality"])
    write_csv(feature_path, [standardized(row) for row in test])
    write_csv(actual_path, [[row[1]] for row in test])

    subprocess.run([str(cli), "train", "--type", "linear", "--data", str(train_path),
                    "--target", str(feature_count), "--header", "--model", str(model_path),
                    "--iterations", "3000", "--learning-rate", "0.05"], check=True)
    with prediction_path.open("w", encoding="ascii", newline="") as output:
        subprocess.run([str(cli), "predict", "--model", str(model_path), "--input", str(feature_path)],
                       check=True, stdout=output)

    predictions = [float(line.strip()) for line in prediction_path.read_text(encoding="ascii").splitlines()]
    actuals = [row[1] for row in test]
    errors = [pred - target for pred, target in zip(predictions, actuals, strict=True)]
    mae = sum(abs(error) for error in errors) / len(errors)
    rmse = math.sqrt(sum(error * error for error in errors) / len(errors))
    target_mean = sum(actuals) / len(actuals)
    total = sum((target - target_mean) ** 2 for target in actuals)
    r_squared = 1.0 - sum(error * error for error in errors) / total
    chart_path = HERE / "prediction.svg"
    chart_path.write_text(make_svg(actuals, predictions, mae, rmse, r_squared, len(train)), encoding="utf-8")
    report = (f"Dataset: UCI Wine Quality (red + white), {len(records)} rows\n"
              f"Train/test: {len(train)}/{len(test)} (seed {SEED})\n"
              f"MAE: {mae:.6f}\nRMSE: {rmse:.6f}\nR2: {r_squared:.6f}\n")
    (OUT_DIR / "metrics.txt").write_text(report, encoding="utf-8")
    print(report)
    print(f"Chart: {chart_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, zipfile.BadZipFile, subprocess.CalledProcessError) as error:
        print(f"Demo failed: {error}", file=sys.stderr)
        raise SystemExit(1)
