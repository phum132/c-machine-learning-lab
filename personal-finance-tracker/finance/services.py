"""Application services: CRUD, summaries, charts, and budgets."""

from __future__ import annotations

import sqlite3
import calendar
from datetime import date

from .database import Database
from .validation import (
    EXPENSE, EXPENSE_CATEGORIES, validate_month, validate_transaction,
    parse_money,
)

FORECAST_MIN_MONTHS = 12
CLASSIFIER_MIN_LABELS_PER_CLASS = 50


def shift_month(month: str, offset: int) -> str:
    year, number = map(int, validate_month(month).split("-"))
    index = year * 12 + number - 1 + offset
    return f"{index // 12:04d}-{index % 12 + 1:02d}"


class FinanceService:
    def __init__(self, database: Database) -> None:
        self.db = database

    def add_transaction(
        self, tx_type: str, category: str, amount: str, tx_date: str, description: str
    ) -> int:
        values = validate_transaction(tx_type, category, amount, tx_date, description)
        with self.db.connection:
            cursor = self.db.connection.execute(
                "INSERT INTO transactions(type,category,amount_cents,date,description) VALUES(?,?,?,?,?)",
                values,
            )
        return int(cursor.lastrowid)

    def update_transaction(
        self, transaction_id: int, tx_type: str, category: str, amount: str,
        tx_date: str, description: str,
    ) -> bool:
        values = validate_transaction(tx_type, category, amount, tx_date, description)
        with self.db.connection:
            cursor = self.db.connection.execute(
                "UPDATE transactions SET type=?,category=?,amount_cents=?,date=?,description=?,"
                "suspicion_label=NULL WHERE id=?",
                (*values, transaction_id),
            )
        return cursor.rowcount > 0

    def delete_transaction(self, transaction_id: int) -> bool:
        with self.db.connection:
            cursor = self.db.connection.execute("DELETE FROM transactions WHERE id=?", (transaction_id,))
        return cursor.rowcount > 0

    def get_transaction(self, transaction_id: int) -> sqlite3.Row | None:
        return self.db.connection.execute(
            "SELECT * FROM transactions WHERE id=?", (transaction_id,)
        ).fetchone()

    def search_transactions(self, query: str = "", limit: int = 1000) -> list[sqlite3.Row]:
        limit = max(1, min(int(limit), 5000))
        text = query.strip()
        pattern = f"%{text}%"
        return self.db.connection.execute(
            "SELECT * FROM transactions WHERE ?='' OR description LIKE ? COLLATE NOCASE "
            "OR category LIKE ? COLLATE NOCASE OR type LIKE ? COLLATE NOCASE "
            "ORDER BY date DESC,id DESC LIMIT ?",
            (text, pattern, pattern, pattern, limit),
        ).fetchall()

    def summary(self, month: str) -> dict[str, int]:
        month = validate_month(month)
        monthly = self.db.connection.execute(
            "SELECT type,SUM(amount_cents) AS total FROM transactions "
            "WHERE substr(date,1,7)=? GROUP BY type", (month,),
        ).fetchall()
        lifetime = self.db.connection.execute(
            "SELECT type,SUM(amount_cents) AS total FROM transactions GROUP BY type"
        ).fetchall()
        month_totals = {row["type"]: row["total"] for row in monthly}
        all_totals = {row["type"]: row["total"] for row in lifetime}
        income = month_totals.get("รายรับ", 0)
        expense = month_totals.get("รายจ่าย", 0)
        total_income = all_totals.get("รายรับ", 0)
        total_expense = all_totals.get("รายจ่าย", 0)
        return {
            "month_income": income,
            "month_expense": expense,
            "month_balance": income - expense,
            "total_income": total_income,
            "total_expense": total_expense,
            "balance": total_income - total_expense,
        }

    def monthly_trend(self, month: str, count: int = 6) -> list[dict[str, int | str]]:
        month = validate_month(month)
        count = max(1, min(int(count), 36))
        months = [shift_month(month, offset) for offset in range(-count + 1, 1)]
        rows = self.db.connection.execute(
            "SELECT substr(date,1,7) AS month,type,SUM(amount_cents) AS total FROM transactions "
            "WHERE date>=? AND date<? GROUP BY month,type",
            (months[0] + "-01", shift_month(month, 1) + "-01"),
        ).fetchall()
        values = {item: {"รายรับ": 0, "รายจ่าย": 0} for item in months}
        for row in rows:
            if row["month"] in values:
                values[row["month"]][row["type"]] = row["total"]
        return [
            {"month": item, "income": values[item]["รายรับ"], "expense": values[item]["รายจ่าย"]}
            for item in months
        ]

    def category_expenses(self, month: str) -> list[dict[str, int | str]]:
        month = validate_month(month)
        rows = self.db.connection.execute(
            "SELECT category,SUM(amount_cents) AS total FROM transactions "
            "WHERE type=? AND substr(date,1,7)=? GROUP BY category ORDER BY total DESC",
            (EXPENSE, month),
        ).fetchall()
        return [{"category": row["category"], "total": row["total"]} for row in rows]

    def set_suspicion_label(self, transaction_id: int, label: int | None) -> bool:
        """Set a human-reviewed binary label on an expense transaction.

        Labels are 0 for normal and 1 for suspicious. `None` removes a label.
        Returns False when the transaction does not exist or is not an expense.
        """
        if type(transaction_id) is not int or transaction_id <= 0:
            raise ValueError("transaction_id must be a positive integer")
        if label is not None and (type(label) is not int or label not in (0, 1)):
            raise ValueError("label must be 0, 1, or None")
        with self.db.connection:
            cursor = self.db.connection.execute(
                "UPDATE transactions SET suspicion_label=? WHERE id=? AND type=?",
                (label, transaction_id, EXPENSE),
            )
        return cursor.rowcount > 0

    def monthly_expense_history(self, month: str, count: int = 12) -> list[dict[str, int | str]]:
        """Return month-by-category expense totals, including empty months as zero.

        This is the future Linear Regression training view. Empty months are
        explicit zero observations only for months within the requested window.
        """
        month = validate_month(month)
        count = max(1, min(int(count), 60))
        months = [shift_month(month, offset) for offset in range(-count + 1, 1)]
        rows = self.db.connection.execute(
            "SELECT substr(date,1,7) AS month,category,SUM(amount_cents) AS total "
            "FROM transactions WHERE type=? AND date>=? AND date<? "
            "GROUP BY substr(date,1,7),category ORDER BY month,category",
            (EXPENSE, months[0] + "-01", shift_month(month, 1) + "-01"),
        ).fetchall()
        totals = {(row["month"], row["category"]): row["total"] for row in rows}
        categories = sorted({*EXPENSE_CATEGORIES, *(row["category"] for row in rows)})
        return [
            {"month": item, "category": category, "total": totals.get((item, category), 0)}
            for item in months
            for category in categories
        ]

    def ml_data_readiness(self, month: str) -> dict[str, int | bool]:
        """Report data thresholds; this method does not train or predict."""
        month = validate_month(month)
        expense_months = self.db.connection.execute(
            "SELECT COUNT(DISTINCT substr(date,1,7)) FROM transactions "
            "WHERE type=? AND substr(date,1,7)<=?",
            (EXPENSE, month),
        ).fetchone()[0]
        label_counts = {
            row["suspicion_label"]: row["sample_count"]
            for row in self.db.connection.execute(
                "SELECT suspicion_label,COUNT(*) AS sample_count FROM transactions "
                "WHERE type=? AND suspicion_label IS NOT NULL GROUP BY suspicion_label",
                (EXPENSE,),
            )
        }
        normal_count = label_counts.get(0, 0)
        suspicious_count = label_counts.get(1, 0)
        return {
            "expense_months": expense_months,
            "forecast_months_required": FORECAST_MIN_MONTHS,
            "forecast_ready": expense_months >= FORECAST_MIN_MONTHS,
            "normal_labels": normal_count,
            "suspicious_labels": suspicious_count,
            "labels_required_per_class": CLASSIFIER_MIN_LABELS_PER_CLASS,
            "classifier_ready": (
                normal_count >= CLASSIFIER_MIN_LABELS_PER_CLASS
                and suspicious_count >= CLASSIFIER_MIN_LABELS_PER_CLASS
            ),
        }

    def set_budget(self, month: str, category: str, amount: str) -> None:
        month = validate_month(month)
        if category not in EXPENSE_CATEGORIES:
            raise ValueError("หมวดหมู่งบประมาณไม่ถูกต้อง")
        limit_cents = parse_money(amount)
        with self.db.connection:
            self.db.connection.execute(
                "INSERT INTO budgets(month,category,limit_cents) VALUES(?,?,?) "
                "ON CONFLICT(month,category) DO UPDATE SET limit_cents=excluded.limit_cents",
                (month, category, limit_cents),
            )

    def delete_budget(self, month: str, category: str) -> bool:
        month = validate_month(month)
        with self.db.connection:
            cursor = self.db.connection.execute(
                "DELETE FROM budgets WHERE month=? AND category=?", (month, category)
            )
        return cursor.rowcount > 0

    def budget_status(self, month: str) -> list[dict[str, int | str]]:
        month = validate_month(month)
        rows = self.db.connection.execute(
            "SELECT b.category,b.limit_cents,COALESCE(SUM(t.amount_cents),0) AS spent "
            "FROM budgets b LEFT JOIN transactions t ON t.category=b.category "
            "AND t.type=? AND substr(t.date,1,7)=b.month WHERE b.month=? "
            "GROUP BY b.category,b.limit_cents ORDER BY b.category",
            (EXPENSE, month),
        ).fetchall()
        year, month_number = map(int, month.split("-"))
        days_in_month = calendar.monthrange(year, month_number)[1]
        today = date.today()
        if (year, month_number) < (today.year, today.month):
            days_remaining = 0
            elapsed_days = days_in_month
        elif (year, month_number) > (today.year, today.month):
            days_remaining = days_in_month
            elapsed_days = 0
        else:
            elapsed_days = today.day
            days_remaining = days_in_month - today.day
        return [
            {
                "category": row["category"],
                "limit": row["limit_cents"],
                "spent": row["spent"],
                "remaining": row["limit_cents"] - row["spent"],
                "over_budget": row["spent"] > row["limit_cents"],
                "used_percent": round(row["spent"] * 100 / row["limit_cents"]),
                "daily_allowance": max(0, row["limit_cents"] - row["spent"]) // max(days_remaining, 1),
                "projected": (row["spent"] * days_in_month // elapsed_days) if elapsed_days else 0,
                "projected_over": bool(elapsed_days and row["spent"] * days_in_month > row["limit_cents"] * elapsed_days),
            }
            for row in rows
        ]

    def copy_budgets(self, source_month: str, target_month: str) -> int:
        """Copy a month's category limits into another month, replacing matching limits."""
        source_month = validate_month(source_month)
        target_month = validate_month(target_month)
        if source_month == target_month:
            raise ValueError("เดือนต้นทางและปลายทางต้องไม่ใช่เดือนเดียวกัน")
        with self.db.connection:
            cursor = self.db.connection.execute(
                "INSERT INTO budgets(month,category,limit_cents) "
                "SELECT ?,category,limit_cents FROM budgets WHERE month=? "
                "ON CONFLICT(month,category) DO UPDATE SET limit_cents=excluded.limit_cents",
                (target_month, source_month),
            )
        return cursor.rowcount
