import tempfile
import unittest
import sqlite3
from pathlib import Path

from finance.database import Database
from finance.validation import EXPENSE


class DatabaseTests(unittest.TestCase):
    def test_initializes_schema_and_persists_rows(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "finance.db"
            db = Database(path)
            tables = {
                row[0]
                for row in db.connection.execute(
                    "SELECT name FROM sqlite_master WHERE type='table'"
                )
            }
            self.assertTrue({"transactions", "budgets"}.issubset(tables))
            db.connection.execute(
                "INSERT INTO transactions(type,category,description,amount_cents,date) VALUES(?,?,?,?,?)",
                ("รายรับ", "เงินเดือน", "ทดสอบ", 10000, "2026-10-01"),
            )
            db.connection.commit()
            db.close()

            reopened = Database(path)
            row = reopened.connection.execute("SELECT amount_cents FROM transactions").fetchone()
            self.assertEqual(row["amount_cents"], 10000)
            reopened.close()

    def test_rejects_non_positive_amount_at_database_layer(self):
        db = Database(":memory:")
        with self.assertRaises(sqlite3.IntegrityError):
            db.connection.execute(
                "INSERT INTO transactions(type,category,description,amount_cents,date) VALUES(?,?,?,?,?)",
                ("รายจ่าย", "อาหาร", "ผิดเงื่อนไข", 0, "2026-10-01"),
            )
        db.close()

    def test_adds_ml_label_column_to_existing_database(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "legacy-finance.db"
            connection = sqlite3.connect(path)
            connection.execute(
                "CREATE TABLE transactions ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, type TEXT NOT NULL, "
                "category TEXT NOT NULL, description TEXT NOT NULL, "
                "amount_cents INTEGER NOT NULL, date TEXT NOT NULL)"
            )
            connection.execute(
                "INSERT INTO transactions(type,category,description,amount_cents,date) "
                "VALUES(?,?,?,?,?)",
                (EXPENSE, "food", "legacy", 1000, "2026-10-01"),
            )
            connection.commit()
            connection.close()

            db = Database(path)
            columns = {
                row["name"] for row in db.connection.execute("PRAGMA table_info(transactions)")
            }
            row = db.connection.execute(
                "SELECT amount_cents,suspicion_label FROM transactions"
            ).fetchone()
            self.assertIn("suspicion_label", columns)
            self.assertEqual(row["amount_cents"], 1000)
            self.assertIsNone(row["suspicion_label"])
            db.close()


if __name__ == "__main__":
    unittest.main()
