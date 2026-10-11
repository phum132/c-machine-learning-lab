#!/usr/bin/env python3
"""Train and evaluate CML on public Bangkok O-NET school averages."""

from __future__ import annotations

import csv
import math
import random
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
DATA = HERE / "data" / "o-net-m3-65.csv"
OUT = ROOT / "build" / "thai-onet"
SEED = 2565


def load_rows() -> list[tuple[list[float], float]]:
    rows: list[tuple[list[float], float]] = []
    with DATA.open("r", encoding="utf-8-sig", newline="") as file:
        for row in csv.DictReader(file):
            try:
                x = [float(row[key]) for key in ("Number", "thaiM3", "EngM3", "MatchM3")]
                y = float(row["ScienceM3"])
            except (ValueError, TypeError, KeyError):
                continue
            if all(math.isfinite(value) for value in x + [y]):
                rows.append((x, y))
    if len(rows) != 101:
        raise ValueError(f"Expected 101 complete school records, found {len(rows)}")
    return rows


def write_csv(path: Path, rows: list[list[float]], header: list[str] | None = None) -> None:
    with path.open("w", encoding="ascii", newline="") as file:
        writer = csv.writer(file, lineterminator="\n")
        if header:
            writer.writerow(header)
        writer.writerows([[format(value, ".12g") for value in row] for row in rows])


def make_svg(actual: list[float], predicted: list[float], mae: float, rmse: float, r2: float) -> str:
    width, height = 820, 620
    left, top, plot = 90.0, 75.0, 480.0
    low, high = 15.0, 55.0

    def sx(value: float) -> float:
        return left + (value - low) * plot / (high - low)

    def sy(value: float) -> float:
        return top + plot - (value - low) * plot / (high - low)

    grid = []
    for value in range(20, 56, 5):
        grid.append(f'<line x1="{sx(value):.1f}" y1="{top}" x2="{sx(value):.1f}" y2="{top+plot}" stroke="#e2e8f0"/>')
        grid.append(f'<line x1="{left}" y1="{sy(value):.1f}" x2="{left+plot}" y2="{sy(value):.1f}" stroke="#e2e8f0"/>')
        grid.append(f'<text x="{sx(value):.1f}" y="{top+plot+23}" text-anchor="middle">{value}</text>')
        grid.append(f'<text x="{left-14}" y="{sy(value)+5:.1f}" text-anchor="end">{value}</text>')
    dots = "".join(
        f'<circle cx="{sx(x):.2f}" cy="{sy(y):.2f}" r="4" fill="#2877b7" fill-opacity="0.65"/>'
        for x, y in zip(actual, predicted, strict=True)
    )
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<style>text{{font:14px Arial,sans-serif;fill:#243247}}.title{{font-size:23px;font-weight:bold}}.axis{{font-size:16px;font-weight:bold}}</style>
<rect width="100%" height="100%" fill="white"/><text class="title" x="90" y="36">Bangkok O-NET M.3: science score</text>
<text x="90" y="58">CML Linear Regression · 20 held-out schools · 81 train schools</text>
{''.join(grid)}
<line x1="{sx(low)}" y1="{sy(low)}" x2="{sx(high)}" y2="{sy(high)}" stroke="#ce3b37" stroke-width="2" stroke-dasharray="7 6"/>
{dots}
<text class="axis" x="330" y="600" text-anchor="middle">Actual science score</text>
<text class="axis" transform="translate(25 315) rotate(-90)" text-anchor="middle">Predicted science score</text>
<text x="600" y="100">MAE: {mae:.2f}</text><text x="600" y="125">RMSE: {rmse:.2f}</text><text x="600" y="150">R²: {r2:.2f}</text>
<text x="600" y="178">Red line = perfect prediction</text></svg>
'''


def main() -> int:
    cli = ROOT / "build" / "cml-lab.exe"
    if not cli.is_file():
        print("Build CML first with: .\\build.bat", file=sys.stderr)
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    rows = load_rows()
    random.Random(SEED).shuffle(rows)
    split = 81
    train, test = rows[:split], rows[split:]
    columns = 4
    means = [sum(row[0][i] for row in train) / len(train) for i in range(columns)]
    scales = []
    for i, mean in enumerate(means):
        variance = sum((row[0][i] - mean) ** 2 for row in train) / len(train)
        scales.append(math.sqrt(variance) or 1.0)

    def scale(row: tuple[list[float], float]) -> list[float]:
        return [(row[0][i] - means[i]) / scales[i] for i in range(columns)]

    train_path = OUT / "train.csv"
    feature_path = OUT / "test-features.csv"
    model_path = OUT / "science-score.cml"
    prediction_path = OUT / "predictions.csv"
    write_csv(train_path, [scale(row) + [row[1]] for row in train],
              ["students_z", "thai_z", "english_z", "math_z", "science"])
    write_csv(feature_path, [scale(row) for row in test])
    write_csv(OUT / "test-actual.csv", [[row[1]] for row in test])
    subprocess.run([str(cli), "train", "--type", "linear", "--data", str(train_path),
                    "--target", "4", "--header", "--model", str(model_path),
                    "--iterations", "3000", "--learning-rate", "0.05"], check=True)
    with prediction_path.open("w", encoding="ascii", newline="") as output:
        subprocess.run([str(cli), "predict", "--model", str(model_path), "--input", str(feature_path)],
                       check=True, stdout=output)
    predicted = [float(value) for value in prediction_path.read_text(encoding="ascii").splitlines()]
    actual = [row[1] for row in test]
    errors = [guess - truth for guess, truth in zip(predicted, actual, strict=True)]
    mae = sum(abs(error) for error in errors) / len(errors)
    rmse = math.sqrt(sum(error * error for error in errors) / len(errors))
    mean = sum(actual) / len(actual)
    r2 = 1.0 - sum(error * error for error in errors) / sum((value - mean) ** 2 for value in actual)
    chart = HERE / "prediction.svg"
    chart.write_text(make_svg(actual, predicted, mae, rmse, r2), encoding="utf-8")
    report = ("Source: Bangkok O-NET M.3, academic year 2565 (school averages)\n"
              "Complete records: 101; train/test: 81/20\n"
              f"MAE: {mae:.6f}\nRMSE: {rmse:.6f}\nR2: {r2:.6f}\n")
    (HERE / "metrics.txt").write_text(report, encoding="utf-8")
    print(report)
    print(f"Chart: {chart}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Demo failed: {error}", file=sys.stderr)
        raise SystemExit(1)
