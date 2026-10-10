"""SQLite connection setup and schema management."""

from __future__ import annotations

import os
import sqlite3
from pathlib import Path


def default_database_path() -> Path:
    """Return a per-user database path, creating its parent directory."""
    base = Path(os.environ.get("LOCALAPPDATA", Path.home() / ".local"))
    folder = base / "PersonalFinanceTrackerPython"
    folder.mkdir(parents=True, exist_ok=True)
    return folder / "finance.db"


class Database:
    def __init__(self, path: str | Path | None = None) -> None:
        self.path = str(path if path is not None else default_database_path())
        if self.path != ":memory:":
            Path(self.path).expanduser().parent.mkdir(parents=True, exist_ok=True)
        self.connection = sqlite3.connect(self.path)
        self.connection.row_factory = sqlite3.Row
        self.connection.execute("PRAGMA foreign_keys = ON")
        self.connection.execute("PRAGMA busy_timeout = 5000")
        self.connection.execute("PRAGMA journal_mode = WAL")
        self.initialize()

    def initialize(self) -> None:
        self.connection.executescript("""
            CREATE TABLE IF NOT EXISTS transactions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                type TEXT NOT NULL CHECK(type IN ('รายรับ','รายจ่าย')),
                category TEXT NOT NULL,
                description TEXT NOT NULL CHECK(length(description) BETWEEN 1 AND 300),
                amount_cents INTEGER NOT NULL CHECK(amount_cents > 0),
                date TEXT NOT NULL CHECK(date GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'),
                suspicion_label INTEGER CHECK(suspicion_label IN (0, 1))
            );
            CREATE INDEX IF NOT EXISTS idx_transactions_date ON transactions(date);
            CREATE INDEX IF NOT EXISTS idx_transactions_category ON transactions(category);
            CREATE TABLE IF NOT EXISTS budgets (
                month TEXT NOT NULL CHECK(month GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]'),
                category TEXT NOT NULL,
                limit_cents INTEGER NOT NULL CHECK(limit_cents > 0),
                PRIMARY KEY(month, category)
            );
        """)
        transaction_columns = {
            row["name"] for row in self.connection.execute("PRAGMA table_info(transactions)")
        }
        if "suspicion_label" not in transaction_columns:
            self.connection.execute(
                "ALTER TABLE transactions ADD COLUMN suspicion_label INTEGER "
                "CHECK(suspicion_label IN (0, 1))"
            )
        self.connection.commit()

    def close(self) -> None:
        self.connection.close()

    def __enter__(self) -> "Database":
        return self

    def __exit__(self, *_exc: object) -> None:
        self.close()
