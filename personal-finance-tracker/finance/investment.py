"""Investment planning calculations and best-effort market quotes."""

from __future__ import annotations

import json
import math
import re
from datetime import datetime, timezone
from urllib.error import HTTPError, URLError
from urllib.parse import quote
from urllib.request import Request, urlopen


def compound_value(initial: float, monthly: float, annual_rate_percent: float, years: float) -> dict[str, float]:
    """Project value with month-end contributions and monthly compounding."""
    if not all(math.isfinite(value) for value in (initial, monthly, annual_rate_percent, years)):
        raise ValueError("กรอกตัวเลขที่มีค่าจำกัดเท่านั้น")
    if initial < 0 or monthly < 0 or years <= 0:
        raise ValueError("เงินเริ่มต้น/เงินลงทุนต่อเดือนต้องไม่ติดลบ และระยะเวลาต้องมากกว่า 0")
    if annual_rate_percent <= -100 or annual_rate_percent > 1000:
        raise ValueError("ผลตอบแทนต่อปีต้องอยู่ระหว่าง -100% ถึง 1,000%")
    months = round(years * 12)
    monthly_rate = (1 + annual_rate_percent / 100) ** (1 / 12) - 1
    growth = (1 + monthly_rate) ** months
    contributions_value = monthly * ((growth - 1) / monthly_rate) if monthly_rate else monthly * months
    invested = initial + monthly * months
    future_value = initial * growth + contributions_value
    return {"months": months, "invested": invested, "future_value": future_value, "gain": future_value - invested}


def fetch_quote(symbol: str, timeout: float = 8) -> dict[str, str | float]:
    """Get a recent Yahoo Finance chart quote; this unofficial source may be delayed."""
    ticker = symbol.strip().upper()
    if not re.fullmatch(r"[A-Z0-9.^=_-]{1,20}", ticker):
        raise ValueError("รูปแบบสัญลักษณ์หุ้นไม่ถูกต้อง เช่น AAPL หรือ PTT.BK")
    url = f"https://query1.finance.yahoo.com/v8/finance/chart/{quote(ticker, safe='.^=_-')}?range=1d&interval=1m"
    request = Request(url, headers={"User-Agent": "Mozilla/5.0 PersonalFinanceTracker/1.0"})
    try:
        with urlopen(request, timeout=timeout) as response:
            payload = json.load(response)
    except (HTTPError, URLError, TimeoutError, OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"ดึงราคา {ticker} ไม่สำเร็จ: {exc}") from exc
    try:
        result = payload["chart"]["result"][0]
        meta = result["meta"]
        price = meta.get("regularMarketPrice")
        if price is None:
            closes = result["indicators"]["quote"][0]["close"]
            price = next(value for value in reversed(closes) if value is not None)
        timestamp = meta.get("regularMarketTime") or result["timestamp"][-1]
        updated = datetime.fromtimestamp(timestamp, timezone.utc).astimezone().strftime("%Y-%m-%d %H:%M")
        return {"symbol": ticker, "price": float(price), "currency": meta.get("currency", ""), "updated": updated}
    except (KeyError, IndexError, TypeError, StopIteration, ValueError) as exc:
        raise ValueError(f"ไม่พบราคาของ {ticker} ตรวจสอบสัญลักษณ์หุ้น") from exc
