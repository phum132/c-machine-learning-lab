"""Monthly expense forecasting with the C Machine Learning Lab CLI.

The tracker exports monthly totals per category as numeric CSV, trains a
Linear Regression model with ``cml-lab.exe``, and checks it against two simple
baselines on the most recent months before trusting it. Data stays local: the
CLI runs as a subprocess inside a temporary directory.
"""

from __future__ import annotations

import csv
import math
import os
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path

from .services import FinanceService, shift_month
from .validation import EXPENSE, validate_month

HISTORY_MONTHS = 24
LAGS = 3
TEST_MONTHS = 3
MIN_TRAIN_ROWS = 12
CLI_TIMEOUT_SECONDS = 60
# Column names must be ASCII letters, digits, or "_" (CML strict CSV header rule).
FEATURES = ("prev1", "prev2", "prev3", "month_sin", "month_cos")


@dataclass
class CategoryForecast:
    category: str
    last_month: float
    average3: float
    model: float
    chosen: float


@dataclass
class ForecastResult:
    target_month: str
    history_start: str
    history_end: str
    train_rows: int
    test_rows: int
    model_mae: float
    last_month_mae: float
    average3_mae: float
    method: str  # "model", "average3", or "last_month"
    categories: list[CategoryForecast] = field(default_factory=list)

    @property
    def total(self) -> float:
        return sum(item.chosen for item in self.categories)


def find_cli() -> Path:
    """Locate cml-lab: $CML_LAB_EXE, the repository build folder, then PATH."""
    candidates = []
    if os.environ.get("CML_LAB_EXE"):
        candidates.append(Path(os.environ["CML_LAB_EXE"]))
    repo_build = Path(__file__).resolve().parents[2] / "build"
    candidates += [repo_build / "cml-lab.exe", repo_build / "cml-lab"]
    for name in ("cml-lab", "cml-lab.exe"):
        found = shutil.which(name)
        if found:
            candidates.append(Path(found))
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(
        "ไม่พบ cml-lab.exe ให้ build ด้วย build.bat ในโฟลเดอร์ c-machine-learning-lab "
        "หรือกำหนดตัวแปร CML_LAB_EXE เป็น path ของโปรแกรม"
    )


def month_features(month: str) -> tuple[float, float]:
    """Encode the calendar month as a point on a circle so December sits next to January."""
    number = int(month[5:7])
    angle = 2.0 * math.pi * (number - 1) / 12.0
    return math.sin(angle), math.cos(angle)


def build_rows(totals: dict[str, list[float]], months: list[str]) -> list[tuple[str, str, list[float], float]]:
    """Turn per-category monthly totals into (category, month, features, target) rows.

    Each row predicts month t from months t-1..t-3. Totals are in baht.
    """
    rows = []
    for category, values in totals.items():
        for index in range(LAGS, len(months)):
            features = [values[index - 1], values[index - 2], values[index - 3], *month_features(months[index])]
            rows.append((category, months[index], features, values[index]))
    return rows


def _write_csv(path: Path, rows: list[list[float]], header: bool = True) -> None:
    with path.open("w", encoding="ascii", newline="") as file:
        writer = csv.writer(file, lineterminator="\n")
        if header:
            writer.writerow([*FEATURES, "target"])
        writer.writerows([[format(value, ".10g") for value in row] for row in rows])


def _run(cli: Path, *args: str) -> str:
    result = subprocess.run(
        [str(cli), *args], capture_output=True, text=True, timeout=CLI_TIMEOUT_SECONDS, check=False
    )
    if result.returncode != 0:
        raise RuntimeError(f"cml-lab {args[0]} ล้มเหลว: {result.stderr.strip()}")
    return result.stdout


def _train_and_predict(cli: Path, work: Path, train: list[list[float]], features: list[list[float]]) -> list[float]:
    train_csv, model, input_csv = work / "train.csv", work / "model.cml", work / "input.csv"
    _write_csv(train_csv, train)
    _write_csv(input_csv, features, header=False)
    _run(cli, "train", "--type", "linear", "--data", str(train_csv), "--target", str(len(FEATURES)),
         "--header", "--model", str(model), "--normalize", "--iterations", "5000",
         "--learning-rate", "0.05", "--tolerance", "1e-10")
    output = _run(cli, "predict", "--model", str(model), "--input", str(input_csv))
    return [float(value) for value in output.split()]


def _mae(predicted: list[float], actual: list[float]) -> float:
    return sum(abs(p - a) for p, a in zip(predicted, actual, strict=True)) / len(actual)


def last_complete_month(selected: str, today: date | None = None) -> str:
    """The current calendar month is still in progress, so history ends before it."""
    today = today or date.today()
    current = today.strftime("%Y-%m")
    selected = validate_month(selected)
    return selected if selected < current else shift_month(current, -1)


@dataclass
class ForecastData:
    """Monthly totals read from the database; safe to hand to a worker thread."""
    months: list[str]
    totals: dict[str, list[float]]


def prepare_forecast(service: FinanceService, selected_month: str, today: date | None = None) -> ForecastData:
    """Read and validate the history window. Runs on the thread that owns the database."""
    end = last_complete_month(selected_month, today)
    first = service.db.connection.execute(
        "SELECT MIN(substr(date,1,7)) FROM transactions WHERE type=?", (EXPENSE,)
    ).fetchone()[0]
    if first is None or first > end:
        raise ValueError("ยังไม่มีรายจ่ายในเดือนที่บันทึกครบแล้ว")
    # Only months after tracking began count as real zero-spend months.
    start = max(first, shift_month(end, -(HISTORY_MONTHS - 1)))
    count = sum(1 for _ in _month_range(start, end))
    if count < LAGS + TEST_MONTHS + 2:
        raise ValueError(f"ต้องมีประวัติรายจ่ายอย่างน้อย {LAGS + TEST_MONTHS + 2} เดือน (ตอนนี้มี {count} เดือน)")
    history = service.monthly_expense_history(end, count)
    months = list(_month_range(start, end))
    totals: dict[str, list[float]] = {}
    for item in history:
        totals.setdefault(str(item["category"]), [0.0] * len(months))
        totals[str(item["category"])][months.index(str(item["month"]))] = int(item["total"]) / 100.0
    totals = {category: values for category, values in totals.items() if any(values)}
    return ForecastData(months, totals)


def run_forecast(data: ForecastData, cli: Path | None = None) -> ForecastResult:
    """Train, check against baselines, and forecast the next month. No database access."""
    months, totals = data.months, data.totals
    start, end = months[0], months[-1]
    rows = build_rows(totals, months)
    test_start = months[-TEST_MONTHS]
    train = [features + [target] for _, month, features, target in rows if month < test_start]
    test = [(features, target) for _, month, features, target in rows if month >= test_start]
    if len(train) < MIN_TRAIN_ROWS:
        raise ValueError(f"ข้อมูลฝึกมี {len(train)} แถว ต้องมีอย่างน้อย {MIN_TRAIN_ROWS} แถว")

    cli = cli or find_cli()
    target_month = shift_month(end, 1)
    with tempfile.TemporaryDirectory(prefix="finance-ml-") as temporary:
        work = Path(temporary)
        # 1) Hold out the latest months and compare the model with simple baselines.
        actual = [target for _, target in test]
        model_test = _train_and_predict(cli, work, train, [features for features, _ in test])
        model_mae = _mae(model_test, actual)
        last_mae = _mae([features[0] for features, _ in test], actual)
        avg_mae = _mae([sum(features[:LAGS]) / LAGS for features, _ in test], actual)
        method = min((model_mae, "model"), (avg_mae, "average3"), (last_mae, "last_month"))[1]

        # 2) Retrain on every row and forecast the next month for each category.
        categories = sorted(totals)
        next_features = [
            [totals[c][-1], totals[c][-2], totals[c][-3], *month_features(target_month)] for c in categories
        ]
        all_rows = [features + [target] for _, _, features, target in rows]
        model_next = _train_and_predict(cli, work, all_rows, next_features)

    forecasts = []
    for category, features, model_value in zip(categories, next_features, model_next, strict=True):
        average3 = sum(features[:LAGS]) / LAGS
        model_value = max(0.0, model_value)  # spending cannot be negative
        chosen = {"model": model_value, "average3": average3, "last_month": features[0]}[method]
        forecasts.append(CategoryForecast(category, features[0], average3, model_value, chosen))
    return ForecastResult(target_month, start, end, len(train), len(test), model_mae, last_mae, avg_mae,
                          method, forecasts)


def forecast_expenses(service: FinanceService, selected_month: str, cli: Path | None = None,
                      today: date | None = None) -> ForecastResult:
    """Forecast the month after the last complete month, per expense category."""
    return run_forecast(prepare_forecast(service, selected_month, today), cli)


def _month_range(start: str, end: str):
    month = start
    while month <= end:
        yield month
        month = shift_month(month, 1)
