import unittest

from finance.database import Database
from finance.services import FinanceService
from finance.validation import EXPENSE, INCOME


class FinanceServiceTests(unittest.TestCase):
    def setUp(self):
        self.database = Database(":memory:")
        self.service = FinanceService(self.database)

    def tearDown(self):
        self.database.close()

    def test_crud_summary_categories_and_budget(self):
        income_id = self.service.add_transaction(INCOME, "เงินเดือน", "50000", "2026-10-01", "เงินเดือน")
        expense_id = self.service.add_transaction(EXPENSE, "อาหาร", "1250.50", "2026-10-02", "มื้อกลางวัน")

        self.assertEqual(self.service.get_transaction(income_id)["amount_cents"], 5_000_000)
        result = self.service.summary("2026-10")
        self.assertEqual(result["month_income"], 5_000_000)
        self.assertEqual(result["month_expense"], 125_050)
        self.assertEqual(result["balance"], 4_874_950)
        self.assertEqual(self.service.category_expenses("2026-10"), [{"category": "อาหาร", "total": 125_050}])
        self.assertEqual(len(self.service.search_transactions("กลางวัน")), 1)
        self.assertEqual(self.service.search_transactions("' OR 1=1 --"), [])

        self.assertTrue(self.service.update_transaction(expense_id, EXPENSE, "เดินทาง", "2000", "2026-10-03", "รถไฟฟ้า"))
        self.assertEqual(self.service.get_transaction(expense_id)["category"], "เดินทาง")
        self.service.set_budget("2026-10", "เดินทาง", "1500")
        budget = self.service.budget_status("2026-10")[0]
        self.assertEqual(budget["spent"], 200_000)
        self.assertEqual(budget["remaining"], -50_000)
        self.assertTrue(budget["over_budget"])
        self.assertTrue(self.service.delete_budget("2026-10", "เดินทาง"))
        self.assertEqual(self.service.budget_status("2026-10"), [])
        self.assertTrue(self.service.delete_transaction(income_id))
        self.assertFalse(self.service.delete_transaction(income_id))

    def test_trend_returns_chronological_six_months(self):
        self.service.add_transaction(INCOME, "เงินเดือน", "100", "2026-06-05", "มิถุนายน")
        trend = self.service.monthly_trend("2026-10")
        self.assertEqual([row["month"] for row in trend], ["2026-05", "2026-06", "2026-07", "2026-08", "2026-09", "2026-10"])
        self.assertEqual(trend[1]["income"], 10_000)

    def test_invalid_input_does_not_write(self):
        with self.assertRaises(ValueError):
            self.service.add_transaction(EXPENSE, "อาหาร", "1.234", "2026-10-01", "รายการ")
        self.assertEqual(self.service.search_transactions(), [])

    def test_manual_suspicion_labels_are_expense_only_and_validated(self):
        expense_id = self.service.add_transaction(
            EXPENSE, "อาหาร", "25", "2026-10-02", "meal"
        )
        income_id = self.service.add_transaction(
            INCOME, "เงินเดือน", "1000", "2026-10-02", "salary"
        )

        self.assertTrue(self.service.set_suspicion_label(expense_id, 1))
        self.assertEqual(self.service.get_transaction(expense_id)["suspicion_label"], 1)
        self.assertTrue(self.service.set_suspicion_label(expense_id, 0))
        self.assertEqual(self.service.get_transaction(expense_id)["suspicion_label"], 0)
        self.assertTrue(self.service.update_transaction(
            expense_id, EXPENSE, "อาหาร", "25", "2026-10-02", "reviewed then edited"
        ))
        self.assertIsNone(self.service.get_transaction(expense_id)["suspicion_label"])
        self.assertTrue(self.service.set_suspicion_label(expense_id, 0))
        self.assertTrue(self.service.set_suspicion_label(expense_id, None))
        self.assertIsNone(self.service.get_transaction(expense_id)["suspicion_label"])
        self.assertFalse(self.service.set_suspicion_label(income_id, 0))
        self.assertFalse(self.service.set_suspicion_label(9999, 0))
        for invalid_label in (-1, 2, True, "1"):
            with self.subTest(label=invalid_label), self.assertRaises(ValueError):
                self.service.set_suspicion_label(expense_id, invalid_label)

    def test_ml_readiness_requires_history_and_both_review_classes(self):
        months = tuple(f"2026-{month:02d}" for month in range(1, 13))
        for index, month in enumerate(months):
            self.service.add_transaction(
                EXPENSE, "อาหาร", "10", f"{month}-01", f"monthly-{index}"
            )
        readiness = self.service.ml_data_readiness("2026-12")
        self.assertEqual(readiness["expense_months"], 12)
        self.assertTrue(readiness["forecast_ready"])
        self.assertFalse(readiness["classifier_ready"])

        for index in range(50):
            normal = self.service.add_transaction(
                EXPENSE, "อาหาร", "1", "2026-12-02", f"normal-{index}"
            )
            suspicious = self.service.add_transaction(
                EXPENSE, "อาหาร", "1", "2026-12-03", f"suspicious-{index}"
            )
            self.service.set_suspicion_label(normal, 0)
            self.service.set_suspicion_label(suspicious, 1)
        readiness = self.service.ml_data_readiness("2026-12")
        self.assertEqual(readiness["normal_labels"], 50)
        self.assertEqual(readiness["suspicious_labels"], 50)
        self.assertTrue(readiness["classifier_ready"])

    def test_monthly_expense_history_fills_missing_months_with_zero(self):
        self.service.add_transaction(EXPENSE, "อาหาร", "10", "2026-09-04", "food")
        history = self.service.monthly_expense_history("2026-10", count=2)
        food_rows = [row for row in history if row["category"] == "อาหาร"]
        self.assertEqual(food_rows, [
            {"month": "2026-09", "category": "อาหาร", "total": 1000},
            {"month": "2026-10", "category": "อาหาร", "total": 0},
        ])


if __name__ == "__main__":
    unittest.main()
