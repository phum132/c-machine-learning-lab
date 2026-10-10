"""Input validation and formatting helpers for finance data."""

from __future__ import annotations

from datetime import date
from decimal import Decimal, InvalidOperation

INCOME = "รายรับ"
EXPENSE = "รายจ่าย"
INCOME_CATEGORIES = ("เงินเดือน", "งานเสริม", "ลงทุน", "ของขวัญ", "รายรับอื่น ๆ")
EXPENSE_CATEGORIES = (
    "อาหาร", "เดินทาง", "ที่พัก", "ค่าน้ำ/ไฟ/เน็ต", "สุขภาพ", "ช้อปปิ้ง",
    "การศึกษา", "บันเทิง", "รายจ่ายอื่น ๆ",
)


def parse_money(value: str) -> int:
    """Convert a positive amount to integer satang (hundredths of a baht)."""
    try:
        amount = Decimal(value.strip().replace(",", ""))
    except (InvalidOperation, AttributeError) as exc:
        raise ValueError("กรุณากรอกจำนวนเงินเป็นตัวเลข เช่น 1250.50") from exc
    if not amount.is_finite() or amount <= 0:
        raise ValueError("จำนวนเงินต้องมากกว่า 0")
    if amount.as_tuple().exponent < -2:
        raise ValueError("จำนวนเงินมีทศนิยมได้ไม่เกิน 2 ตำแหน่ง")
    cents = int(amount * 100)
    if cents > 9_000_000_000_000_000:
        raise ValueError("จำนวนเงินสูงเกินกว่าที่ระบบรองรับ")
    return cents


def format_money(cents: int) -> str:
    sign = "-" if cents < 0 else ""
    absolute = abs(cents)
    return f"{sign}{absolute // 100:,}.{absolute % 100:02d} บาท"


def validate_date(value: str) -> str:
    try:
        parsed = date.fromisoformat(value.strip())
    except (ValueError, AttributeError) as exc:
        raise ValueError("วันที่ไม่ถูกต้อง กรุณาใช้รูปแบบ YYYY-MM-DD") from exc
    normalized = parsed.isoformat()
    if normalized != value.strip():
        raise ValueError("วันที่ไม่ถูกต้อง กรุณาใช้รูปแบบ YYYY-MM-DD")
    return normalized


def validate_month(value: str) -> str:
    month = value.strip()
    try:
        parsed = date.fromisoformat(f"{month}-01")
    except (ValueError, AttributeError) as exc:
        raise ValueError("กรุณาระบุเดือนในรูปแบบ YYYY-MM เช่น 2026-10") from exc
    normalized = parsed.strftime("%Y-%m")
    if normalized != month:
        raise ValueError("กรุณาระบุเดือนในรูปแบบ YYYY-MM เช่น 2026-10")
    return normalized


def validate_transaction(
    tx_type: str, category: str, amount: str, tx_date: str, description: str
) -> tuple[str, str, int, str, str]:
    if tx_type not in (INCOME, EXPENSE):
        raise ValueError("กรุณาเลือกประเภทรายการ")
    categories = INCOME_CATEGORIES if tx_type == INCOME else EXPENSE_CATEGORIES
    if category not in categories:
        raise ValueError("หมวดหมู่ไม่ตรงกับประเภทรายการ")
    clean_description = description.strip()
    if not clean_description:
        raise ValueError("กรุณากรอกรายละเอียดรายการ")
    if len(clean_description) > 300:
        raise ValueError("รายละเอียดต้องไม่เกิน 300 ตัวอักษร")
    return tx_type, category, parse_money(amount), validate_date(tx_date), clean_description
