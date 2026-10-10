import unittest

from finance.validation import format_money, parse_money, validate_date, validate_month


class MoneyValidationTests(unittest.TestCase):
    def test_parse_money_to_satang(self):
        self.assertEqual(parse_money("1250.50"), 125050)
        self.assertEqual(parse_money("1,250"), 125000)
        self.assertEqual(parse_money("0.01"), 1)

    def test_format_money(self):
        self.assertEqual(format_money(123456789), "1,234,567.89 บาท")
        self.assertEqual(format_money(-50), "-0.50 บาท")

    def test_reject_invalid_amounts(self):
        for value in ("", "abc", "0", "-1", "1.234", "NaN", "Infinity"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                parse_money(value)

    def test_validate_date_and_month(self):
        self.assertEqual(validate_date("2024-02-29"), "2024-02-29")
        self.assertEqual(validate_month("2026-10"), "2026-10")
        for value in ("2023-02-29", "2024-2-01", "2026-13-01"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_date(value)
        for value in ("2026-00", "2026-13", "2026-1"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_month(value)


if __name__ == "__main__":
    unittest.main()
