import math
import os
import unittest
from datetime import date

from finance.database import Database
from finance.ml import (
    build_rows, find_cli, forecast_expenses, last_complete_month, month_features, prepare_forecast,
)
from finance.services import FinanceService, shift_month
from finance.validation import EXPENSE, INCOME

try:
    CLI = find_cli()
except FileNotFoundError:
    # CI sets CML_REQUIRE_CLI so a missing build fails loudly instead of skipping.
    if os.environ.get("CML_REQUIRE_CLI") == "1":
        raise
    CLI = None


class ForecastHelperTests(unittest.TestCase):
    def test_month_features_place_december_next_to_january(self):
        january = month_features("2026-01")
        self.assertAlmostEqual(january[0], 0.0)
        self.assertAlmostEqual(january[1], 1.0)
        december = month_features("2026-12")
        self.assertLess(math.dist(january, december), math.dist(january, month_features("2026-07")))

    def test_build_rows_uses_three_previous_months(self):
        months = ["2026-01", "2026-02", "2026-03", "2026-04", "2026-05"]
        rows = build_rows({"อาหาร": [10.0, 20.0, 30.0, 40.0, 50.0]}, months)
        self.assertEqual([row[1] for row in rows], ["2026-04", "2026-05"])
        category, month, features, target = rows[0]
        self.assertEqual((category, features[:3], target), ("อาหาร", [30.0, 20.0, 10.0], 40.0))
        self.assertEqual(len(features), 5)

    def test_last_complete_month_skips_the_month_in_progress(self):
        today = date(2026, 10, 11)
        self.assertEqual(last_complete_month("2026-10", today), "2026-09")
        self.assertEqual(last_complete_month("2026-12", today), "2026-09")
        self.assertEqual(last_complete_month("2026-05", today), "2026-05")


class ForecastDataTests(unittest.TestCase):
    def setUp(self):
        self.database = Database(":memory:")
        self.service = FinanceService(self.database)

    def tearDown(self):
        self.database.close()

    def add_months(self, category, start, amounts):
        for offset, amount in enumerate(amounts):
            month = shift_month(start, offset)
            self.service.add_transaction(EXPENSE, category, f"{amount:.2f}", month + "-10", "test")

    def test_requires_enough_history(self):
        self.add_months("อาหาร", "2026-01", [100, 120, 130])
        with self.assertRaises(ValueError):
            prepare_forecast(self.service, "2026-03", today=date(2026, 10, 1))

    def test_no_expenses_is_rejected(self):
        self.service.add_transaction(INCOME, "เงินเดือน", "1000", "2026-01-01", "salary")
        with self.assertRaises(ValueError):
            prepare_forecast(self.service, "2026-05", today=date(2026, 10, 1))

    def test_history_starts_at_first_expense_and_skips_unused_categories(self):
        self.add_months("อาหาร", "2025-06", [100] * 12)
        self.service.add_transaction(EXPENSE, "เดินทาง", "50", "2026-02-03", "bus")
        data = prepare_forecast(self.service, "2026-05", today=date(2026, 10, 1))
        self.assertEqual(data.months[0], "2025-06")
        self.assertEqual(data.months[-1], "2026-05")
        self.assertEqual(set(data.totals), {"อาหาร", "เดินทาง"})
        self.assertEqual(data.totals["เดินทาง"][data.months.index("2026-02")], 50.0)
        self.assertEqual(data.totals["เดินทาง"][0], 0.0)


@unittest.skipIf(CLI is None, "cml-lab executable not built")
class ForecastIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.database = Database(":memory:")
        self.service = FinanceService(self.database)

    def tearDown(self):
        self.database.close()

    def test_trend_is_learned_and_beats_last_month_baseline(self):
        # Steady growth: "same as last month" always lags by one step, a linear model does not.
        for offset in range(18):
            month = shift_month("2025-07", offset)
            self.service.add_transaction(EXPENSE, "อาหาร", f"{3000 + 200 * offset}", month + "-05", "food")
            self.service.add_transaction(EXPENSE, "เดินทาง", f"{1000 + 50 * offset}", month + "-07", "travel")
        result = forecast_expenses(self.service, "2026-12", cli=CLI, today=date(2027, 1, 15))
        self.assertEqual(result.target_month, "2027-01")
        self.assertEqual(result.method, "model")
        self.assertLess(result.model_mae, result.last_month_mae)
        food = next(item for item in result.categories if item.category == "อาหาร")
        self.assertAlmostEqual(food.chosen, 3000 + 200 * 18, delta=150)
        self.assertEqual(food.last_month, 3000 + 200 * 17)
        self.assertGreater(result.total, 0)


if __name__ == "__main__":
    unittest.main()
