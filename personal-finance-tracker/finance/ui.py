"""Tkinter user interface for the finance tracker."""

from __future__ import annotations

import queue
import sqlite3
import subprocess
import tkinter as tk
import threading
from datetime import date
from tkinter import messagebox, ttk

from .database import Database
from .services import FinanceService
from .investment import compound_value, fetch_quote
from .ml import ForecastResult, prepare_forecast, run_forecast
from .validation import (
    EXPENSE, EXPENSE_CATEGORIES, INCOME, INCOME_CATEGORIES, format_money,
    validate_month,
)

GREEN = "#16845b"
RED = "#d14f52"
BLUE = "#3976c5"
MUTED = "#687386"


class FinanceApp(tk.Tk):
    def __init__(self, database: Database | None = None) -> None:
        super().__init__()
        self.title("ตัวติดตามการเงินส่วนบุคคล")
        self.geometry("1180x800")
        self.minsize(960, 680)
        self.configure(bg="#f4f6fa")
        self.database = database or Database()
        self.service = FinanceService(self.database)
        self.selected_transaction: int | None = None
        self.month = tk.StringVar(value=date.today().strftime("%Y-%m"))
        self.search_text = tk.StringVar()
        self.tx_type = tk.StringVar(value=EXPENSE)
        self.tx_category = tk.StringVar()
        self.tx_amount = tk.StringVar()
        self.tx_date = tk.StringVar(value=date.today().isoformat())
        self.tx_description = tk.StringVar()
        self.budget_category = tk.StringVar(value=EXPENSE_CATEGORIES[0])
        self.budget_amount = tk.StringVar()
        self._setup_style()
        self._build_ui()
        self._update_categories()
        self.refresh_all()
        self.protocol("WM_DELETE_WINDOW", self.close)

    def _setup_style(self) -> None:
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure(".", font=("Tahoma", 10))
        style.configure("TFrame", background="#f4f6fa")
        style.configure("Card.TFrame", background="white")
        style.configure("TLabel", background="#f4f6fa", foreground="#202938")
        style.configure("Card.TLabel", background="white")
        style.configure("Title.TLabel", font=("Tahoma", 22, "bold"), background="#f4f6fa")
        style.configure("Sub.TLabel", foreground=MUTED, background="#f4f6fa")
        style.configure("CardTitle.TLabel", foreground=MUTED, background="white")
        style.configure("CardValue.TLabel", font=("Tahoma", 17, "bold"), background="white")
        style.configure("TButton", padding=(12, 7))
        style.configure("Accent.TButton", background=BLUE, foreground="white")
        style.map("Accent.TButton", background=[("active", "#2d65ad")])
        style.configure("Treeview", rowheight=29, font=("Tahoma", 10), background="white", fieldbackground="white")
        style.configure("Treeview.Heading", font=("Tahoma", 10, "bold"), padding=7)
        style.configure("TNotebook", background="#f4f6fa", borderwidth=0)
        style.configure("TNotebook.Tab", padding=(18, 10), font=("Tahoma", 10, "bold"))

    def _build_ui(self) -> None:
        header = ttk.Frame(self, padding=(24, 18, 24, 10))
        header.pack(fill="x")
        title_area = ttk.Frame(header)
        title_area.pack(side="left", fill="x", expand=True)
        ttk.Label(title_area, text="ตัวติดตามการเงินส่วนบุคคล", style="Title.TLabel").pack(anchor="w")
        ttk.Label(title_area, text="บันทึก วางแผน และดูภาพรวมการเงินของคุณ", style="Sub.TLabel").pack(anchor="w", pady=(3, 0))
        month_area = ttk.Frame(header)
        month_area.pack(side="right", anchor="e", padx=(12, 0))
        ttk.Label(month_area, text="เดือนที่ดู").pack(side="left", padx=(0, 8))
        ttk.Entry(month_area, textvariable=self.month, width=10, justify="center").pack(side="left")
        ttk.Button(month_area, text="อัปเดต", command=self.refresh_all, style="Accent.TButton").pack(side="left", padx=(8, 0))

        tabs = ttk.Notebook(self)
        tabs.pack(fill="both", expand=True, padx=22, pady=(6, 18))
        self.dashboard_tab = ttk.Frame(tabs, padding=16)
        self.transactions_tab = ttk.Frame(tabs, padding=16)
        self.budgets_tab = ttk.Frame(tabs, padding=16)
        self.ml_tab = ttk.Frame(tabs, padding=16)
        self.investment_tab = ttk.Frame(tabs, padding=16)
        tabs.add(self.dashboard_tab, text="ภาพรวม")
        tabs.add(self.transactions_tab, text="รายการรับ-จ่าย")
        tabs.add(self.budgets_tab, text="งบประมาณ")
        tabs.add(self.ml_tab, text="ML และพยากรณ์")
        tabs.add(self.investment_tab, text="แผนลงทุน")
        self._build_dashboard()
        self._build_transactions()
        self._build_budgets()
        self._build_ml_insights()
        self._build_investment()

    def _build_investment(self) -> None:
        ttk.Label(self.investment_tab, text="จำลองเงินลงทุนและดอกผลทบต้น ตัวเลขเป็นประมาณการ ไม่รับประกันผลตอบแทน", style="Sub.TLabel").pack(anchor="w", pady=(0, 10))
        inputs = ttk.LabelFrame(self.investment_tab, text="เงินลงทุนและสมมติฐาน", padding=12)
        inputs.pack(fill="x", pady=(0, 10))
        self.investment_initial = tk.StringVar(value="0")
        self.investment_monthly = tk.StringVar(value="5000")
        self.investment_rate = tk.StringVar(value="7")
        self.investment_years = tk.StringVar(value="20")
        fields = (("เงินก้อนเริ่มต้น (บาท)", self.investment_initial), ("ลงทุนต่อเดือน (บาท)", self.investment_monthly), ("ผลตอบแทนเฉลี่ย/ปี (%)", self.investment_rate), ("ระยะเวลา (ปี)", self.investment_years))
        for column, (label, variable) in enumerate(fields):
            ttk.Label(inputs, text=label).grid(row=0, column=column, sticky="w", padx=5)
            ttk.Entry(inputs, textvariable=variable, width=18).grid(row=1, column=column, sticky="ew", padx=5, pady=(4, 8))
            inputs.columnconfigure(column, weight=1)
        ttk.Button(inputs, text="คำนวณทบต้น", command=self.calculate_investment, style="Accent.TButton").grid(row=1, column=4, padx=8)
        self.investment_result = ttk.Label(inputs, text="", style="CardValue.TLabel")
        self.investment_result.grid(row=2, column=0, columnspan=5, sticky="w", padx=5, pady=(4, 0))

        stock_form = ttk.LabelFrame(self.investment_tab, text="หุ้นในแผนจัดสรร", padding=10)
        stock_form.pack(fill="x", pady=(0, 10))
        self.stock_symbol = tk.StringVar()
        self.stock_weight = tk.StringVar(value="100")
        self.stock_rate = tk.StringVar(value="7")
        ttk.Label(stock_form, text="สัญลักษณ์ (เช่น AAPL หรือ PTT.BK)").pack(side="left", padx=(4, 6))
        ttk.Entry(stock_form, textvariable=self.stock_symbol, width=18).pack(side="left", padx=5)
        ttk.Label(stock_form, text="สัดส่วน %").pack(side="left", padx=(12, 4))
        ttk.Entry(stock_form, textvariable=self.stock_weight, width=8).pack(side="left", padx=5)
        ttk.Label(stock_form, text="คาดหวัง/ปี %").pack(side="left", padx=(10, 4))
        ttk.Entry(stock_form, textvariable=self.stock_rate, width=8).pack(side="left", padx=5)
        ttk.Button(stock_form, text="เพิ่มหุ้น", command=self.add_planned_stock).pack(side="left", padx=5)
        self.quote_button = ttk.Button(stock_form, text="อัปเดตราคา", command=self.refresh_stock_quotes)
        self.quote_button.pack(side="left", padx=5)
        ttk.Button(stock_form, text="ลบหุ้นที่เลือก", command=self.remove_planned_stock).pack(side="left", padx=5)

        columns = ("symbol", "weight", "rate", "price", "currency", "monthly", "shares", "forecast", "updated")
        self.stock_tree = ttk.Treeview(self.investment_tab, columns=columns, show="headings", selectmode="browse", height=5)
        headings = {"symbol": "หุ้น", "weight": "สัดส่วน", "rate": "คาดหวัง/ปี", "price": "ราคาล่าสุด", "currency": "สกุลเงิน", "monthly": "ลงทุน/เดือน (บาท)", "shares": "หุ้นเทียบเท่า/เดือน", "forecast": "มูลค่าคาดการณ์", "updated": "อัปเดตเมื่อ"}
        widths = {"symbol": 85, "weight": 70, "rate": 95, "price": 100, "currency": 75, "monthly": 120, "shares": 130, "forecast": 130, "updated": 120}
        for column in columns:
            self.stock_tree.heading(column, text=headings[column])
            self.stock_tree.column(column, width=widths[column], anchor="center")
        self.stock_tree.pack(fill="both", expand=True)
        self.investment_chart = tk.Canvas(self.investment_tab, height=220, bg="white", highlightthickness=0)
        self.investment_chart.pack(fill="both", expand=True, pady=(8, 0))
        self.investment_chart.bind("<Configure>", lambda _event: self._draw_investment_chart())
        self.quote_note = ttk.Label(self.investment_tab, text="กราฟเป็นการจำลองจากผลตอบแทนที่กรอก ไม่ใช่การทำนายราคาหุ้น · ราคาอ้างอิง Yahoo Finance อาจล่าช้า · จำนวนหุ้นคำนวณได้เฉพาะราคาสกุลบาท", style="Sub.TLabel")
        self.quote_note.pack(anchor="w", pady=(5, 0))
        self.investment_series = []

    def calculate_investment(self) -> None:
        try:
            result = compound_value(float(self.investment_initial.get().replace(",", "")), float(self.investment_monthly.get().replace(",", "")), float(self.investment_rate.get().replace(",", "")), float(self.investment_years.get().replace(",", "")))
            self.investment_result.configure(text=(
                f"มูลค่าพอร์ตใน {result['months']} เดือน: {result['future_value']:,.2f} บาท   ·   "
                f"เงินต้น {result['invested']:,.2f} บาท   ·   ผลตอบแทนประมาณ {result['gain']:,.2f} บาท"
            ))
            self._calculate_stock_projection()
            self._update_planned_stock_allocations()
            self._draw_investment_chart()
        except (ValueError, OverflowError) as exc:
            messagebox.showerror("ข้อมูลไม่ถูกต้อง", str(exc), parent=self)

    def _calculate_stock_projection(self) -> None:
        initial = float(self.investment_initial.get().replace(",", ""))
        monthly = float(self.investment_monthly.get().replace(",", ""))
        general_rate = float(self.investment_rate.get().replace(",", ""))
        years = float(self.investment_years.get().replace(",", ""))
        steps = max(1, int(years) + (1 if years % 1 else 0))
        rows = self.stock_tree.get_children()
        total_weight = sum(float(str(self.stock_tree.set(item, "weight")).rstrip("%")) / 100 for item in rows)
        parts = []
        for item in rows:
            weight = float(str(self.stock_tree.set(item, "weight")).rstrip("%")) / 100
            rate = float(str(self.stock_tree.set(item, "rate")).rstrip("%"))
            parts.append((item, initial * weight, monthly * weight, rate))
        if total_weight < 0.999999:
            parts.append(("เงินที่ยังไม่จัดสรร", initial * (1-total_weight), monthly * (1-total_weight), general_rate))
        if not parts:
            parts = [("พอร์ตรวม", initial, monthly, general_rate)]

        colors = (BLUE, GREEN, RED, "#8b5cf6", "#d97706", "#0891b2", "#db2777", "#65a30d")
        series = []
        future_total = 0.0
        for index, (name, start, contribution, rate) in enumerate(parts):
            forecast = []
            for step in range(steps + 1):
                elapsed = min(float(step), years)
                if elapsed == 0:
                    forecast.append(start)
                else:
                    forecast.append(compound_value(start, contribution, rate, elapsed)["future_value"])
            future_total += forecast[-1]
            series.append({"name": name, "values": forecast, "color": colors[index % len(colors)]})
            if name in rows:
                stock_row = list(self.stock_tree.item(name, "values"))
                stock_row[7] = f"{forecast[-1]:,.2f} บาท"
                self.stock_tree.item(name, values=stock_row)
        self.investment_series = series
        months = round(years * 12)
        invested = initial + monthly * months
        self.investment_result.configure(text=(
            f"มูลค่าพอร์ตใน {months} เดือน: {future_total:,.2f} บาท   ·   "
            f"เงินต้น {invested:,.2f} บาท   ·   ผลตอบแทนประมาณ {future_total-invested:,.2f} บาท"
        ))

    def _draw_investment_chart(self) -> None:
        canvas = getattr(self, "investment_chart", None)
        if canvas is None:
            return
        canvas.delete("all")
        series = getattr(self, "investment_series", [])
        width, height = max(canvas.winfo_width(), 600), max(canvas.winfo_height(), 190)
        if not series:
            canvas.create_text(width/2, height/2, text="กรอกสมมติฐานแล้วกด ‘คำนวณทบต้น’ เพื่อดูกราฟ", fill=MUTED, font=("Tahoma", 10))
            return
        values = [value for item in series for value in item["values"]]
        maximum = max(values, default=0) or 1
        left, right, top, bottom = 72, 18, 34, 30
        plot_w, plot_h = width-left-right, height-top-bottom
        for index in range(5):
            amount = maximum * (4-index) / 4
            y = top + plot_h * index / 4
            canvas.create_line(left, y, width-right, y, fill="#e8ecf2")
            if amount >= 1_000_000:
                label = f"{amount/1_000_000:.1f} ลบ."
            elif amount >= 1_000:
                label = f"{amount/1_000:.0f} พัน"
            else:
                label = f"{amount:.0f}"
            canvas.create_text(left-8, y, text=label, anchor="e", fill=MUTED, font=("Tahoma", 8))
        count = len(series[0]["values"])
        for index, item in enumerate(series):
            points = []
            for step, amount in enumerate(item["values"]):
                x = left + plot_w * step / max(count-1, 1)
                y = top + plot_h * (1 - amount/maximum)
                points.extend((x, y))
            if len(points) >= 4:
                canvas.create_line(*points, fill=item["color"], width=2, smooth=True)
            legend_x = left + (index % 5) * max(95, plot_w/5)
            legend_y = 12 + (index // 5) * 16
            canvas.create_line(legend_x, legend_y, legend_x+16, legend_y, fill=item["color"], width=3)
            canvas.create_text(legend_x+20, legend_y, text=item["name"][:12], anchor="w", fill="#344054", font=("Tahoma", 8))
        years = float(self.investment_years.get().replace(",", ""))
        for index in range(6):
            x = left + plot_w * index / 5
            canvas.create_text(x, height-8, text=f"{years*index/5:g} ปี", fill=MUTED, font=("Tahoma", 8))

    def add_planned_stock(self) -> None:
        symbol = self.stock_symbol.get().strip().upper()
        try:
            weight = float(self.stock_weight.get())
            if not 0 < weight <= 100:
                raise ValueError("สัดส่วนต้องมากกว่า 0 และไม่เกิน 100")
            expected_rate = float(self.stock_rate.get())
            if not -100 < expected_rate <= 1000:
                raise ValueError("ผลตอบแทนคาดหวังต้องมากกว่า -100% และไม่เกิน 1,000%")
            if not symbol:
                raise ValueError("กรอกสัญลักษณ์หุ้นก่อน")
            # Reuse quote validation without making a network request.
            import re
            if not re.fullmatch(r"[A-Z0-9.^=_-]{1,20}", symbol):
                raise ValueError("รูปแบบสัญลักษณ์ไม่ถูกต้อง เช่น AAPL หรือ PTT.BK")
            if self.stock_tree.exists(symbol):
                raise ValueError("เพิ่มสัญลักษณ์นี้ไว้แล้ว")
            used = sum(float(self.stock_tree.set(item, "weight").rstrip("%")) for item in self.stock_tree.get_children())
            if used + weight > 100.000001:
                raise ValueError(f"สัดส่วนรวมเกิน 100% (จัดสรรแล้ว {used:g}%)")
            self.stock_tree.insert("", "end", iid=symbol, values=(symbol, f"{weight:g}%", f"{expected_rate:g}%", "—", "—", "—", "—", "—", "—"))
            self.stock_symbol.set("")
            self._update_planned_stock_allocations()
        except ValueError as exc:
            messagebox.showerror("เพิ่มหุ้นไม่สำเร็จ", str(exc), parent=self)

    def remove_planned_stock(self) -> None:
        selection = self.stock_tree.selection()
        if selection:
            self.stock_tree.delete(selection[0])

    def _update_planned_stock_allocations(self) -> None:
        try:
            monthly = float(self.investment_monthly.get().replace(",", ""))
        except ValueError:
            return
        for item in self.stock_tree.get_children():
            row = list(self.stock_tree.item(item, "values"))
            allocation = monthly * float(str(row[1]).rstrip("%")) / 100
            row[5] = f"{allocation:,.2f}"
            try:
                price = float(str(row[3]).replace(",", ""))
                row[6] = f"{allocation / price:,.6f}" if price > 0 and row[4] == "THB" else "—"
            except (ValueError, TypeError):
                row[6] = "—"
            self.stock_tree.item(item, values=row)

    def refresh_stock_quotes(self) -> None:
        symbols = list(self.stock_tree.get_children())
        if not symbols:
            messagebox.showinfo("ยังไม่มีหุ้น", "เพิ่มสัญลักษณ์หุ้นก่อนอัปเดตราคา", parent=self)
            return
        self.quote_button.configure(state="disabled", text="กำลังดึงราคา…")
        def worker() -> None:
            results = []
            for symbol in symbols:
                try:
                    results.append((symbol, fetch_quote(symbol), None))
                except ValueError as exc:
                    results.append((symbol, None, str(exc)))
            self.after(0, lambda: self._apply_stock_quotes(results))
        threading.Thread(target=worker, daemon=True).start()

    def _apply_stock_quotes(self, results: list[tuple[str, dict | None, str | None]]) -> None:
        errors = []
        for symbol, quote_data, error in results:
            if not self.stock_tree.exists(symbol):
                continue
            if error:
                errors.append(error)
                continue
            row = list(self.stock_tree.item(symbol, "values"))
            row[3] = f"{quote_data['price']:,.4f}"
            row[4] = quote_data["currency"]
            row[8] = quote_data["updated"]
            self.stock_tree.item(symbol, values=row)
        self._update_planned_stock_allocations()
        self.quote_button.configure(state="normal", text="อัปเดตราคา")
        self.quote_note.configure(text=(" · ".join(errors) if errors else "ราคาอ้างอิง Yahoo Finance อาจล่าช้า/คลาดเคลื่อน · คำนวณจำนวนหุ้นได้เฉพาะราคาสกุลบาท · ไม่ควรใช้ส่งคำสั่งซื้อขาย"))

    def _build_dashboard(self) -> None:
        cards = ttk.Frame(self.dashboard_tab)
        cards.pack(fill="x", pady=(0, 14))
        self.card_values: dict[str, ttk.Label] = {}
        for key, label, color in (("income", "รายรับเดือนนี้", GREEN), ("expense", "รายจ่ายเดือนนี้", RED), ("balance", "คงเหลือสะสม", BLUE)):
            card = ttk.Frame(cards, style="Card.TFrame", padding=(16, 12))
            card.pack(side="left", fill="x", expand=True, padx=(0, 10))
            ttk.Label(card, text=label, style="CardTitle.TLabel").pack(anchor="w")
            value = ttk.Label(card, text="0.00 บาท", style="CardValue.TLabel", foreground=color)
            value.pack(anchor="w", pady=(8, 0))
            self.card_values[key] = value

        charts = ttk.Frame(self.dashboard_tab)
        charts.pack(fill="both", expand=True)
        trend_box = ttk.Frame(charts, style="Card.TFrame", padding=12)
        trend_box.pack(side="left", fill="both", expand=True, padx=(0, 8))
        ttk.Label(trend_box, text="รายรับและรายจ่ายย้อนหลัง 6 เดือน", style="Card.TLabel", font=("Tahoma", 12, "bold")).pack(anchor="w", pady=(0, 8))
        self.trend_canvas = tk.Canvas(trend_box, height=390, bg="white", highlightthickness=0)
        self.trend_canvas.pack(fill="both", expand=True)
        self.trend_canvas.bind("<Configure>", lambda _event: self._draw_trend())

        category_box = ttk.Frame(charts, style="Card.TFrame", padding=12)
        category_box.pack(side="left", fill="both", expand=True, padx=(8, 0))
        ttk.Label(category_box, text="รายจ่ายแยกตามหมวดหมู่", style="Card.TLabel", font=("Tahoma", 12, "bold")).pack(anchor="w", pady=(0, 8))
        self.category_canvas = tk.Canvas(category_box, height=390, bg="white", highlightthickness=0)
        self.category_canvas.pack(fill="both", expand=True)
        self.category_canvas.bind("<Configure>", lambda _event: self._draw_categories())

    def _build_transactions(self) -> None:
        toolbar = ttk.Frame(self.transactions_tab)
        toolbar.pack(fill="x", pady=(0, 10))
        ttk.Label(toolbar, text="ค้นหา").pack(side="left")
        search = ttk.Entry(toolbar, textvariable=self.search_text, width=34)
        search.pack(side="left", padx=8)
        search.bind("<Return>", lambda _event: self.refresh_transactions())
        ttk.Button(toolbar, text="ค้นหา", command=self.refresh_transactions).pack(side="left")
        ttk.Button(toolbar, text="แสดงทั้งหมด", command=self.clear_search).pack(side="left", padx=6)

        table_area = ttk.Frame(self.transactions_tab)
        table_area.pack(fill="both", expand=True)
        columns = ("id", "date", "type", "category", "amount", "description", "ml_label")
        self.tx_tree = ttk.Treeview(table_area, columns=columns, show="headings", selectmode="browse")
        headings = {"id": "เลขที่", "date": "วันที่", "type": "ประเภท", "category": "หมวดหมู่", "amount": "จำนวนเงิน", "description": "รายละเอียด", "ml_label": "ML label"}
        widths = {"id": 60, "date": 105, "type": 90, "category": 145, "amount": 130, "description": 260, "ml_label": 110}
        for column in columns:
            self.tx_tree.heading(column, text=headings[column])
            self.tx_tree.column(column, width=widths[column], anchor="w" if column in ("category", "description") else "center")
        self.tx_tree.tag_configure(INCOME, foreground=GREEN)
        self.tx_tree.tag_configure(EXPENSE, foreground=RED)
        scroll = ttk.Scrollbar(table_area, orient="vertical", command=self.tx_tree.yview)
        self.tx_tree.configure(yscrollcommand=scroll.set)
        self.tx_tree.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")
        self.tx_tree.bind("<<TreeviewSelect>>", self.select_transaction)

        form = ttk.LabelFrame(self.transactions_tab, text="เพิ่มหรือแก้ไขรายการ", padding=10)
        form.pack(fill="x", pady=(12, 0))
        fields = (("ประเภท", 0), ("หมวดหมู่", 1), ("จำนวนเงิน (บาท)", 2), ("วันที่ (YYYY-MM-DD)", 3), ("รายละเอียด", 4))
        for label, column in fields:
            ttk.Label(form, text=label).grid(row=0, column=column, sticky="w", padx=4)
        type_box = ttk.Combobox(form, textvariable=self.tx_type, values=(INCOME, EXPENSE), state="readonly", width=12)
        type_box.grid(row=1, column=0, sticky="ew", padx=4, pady=(3, 8))
        type_box.bind("<<ComboboxSelected>>", lambda _event: self._update_categories())
        self.category_box = ttk.Combobox(form, textvariable=self.tx_category, state="readonly", width=18)
        self.category_box.grid(row=1, column=1, sticky="ew", padx=4, pady=(3, 8))
        ttk.Entry(form, textvariable=self.tx_amount, width=16).grid(row=1, column=2, sticky="ew", padx=4, pady=(3, 8))
        ttk.Entry(form, textvariable=self.tx_date, width=16).grid(row=1, column=3, sticky="ew", padx=4, pady=(3, 8))
        ttk.Entry(form, textvariable=self.tx_description, width=30).grid(row=1, column=4, sticky="ew", padx=4, pady=(3, 8))
        for column in range(5):
            form.columnconfigure(column, weight=1)
        actions = ttk.Frame(form)
        actions.grid(row=2, column=0, columnspan=5, sticky="w")
        ttk.Button(actions, text="เพิ่มรายการ", command=self.add_transaction, style="Accent.TButton").pack(side="left", padx=(4, 6))
        ttk.Button(actions, text="บันทึกการแก้ไข", command=self.update_transaction).pack(side="left", padx=6)
        ttk.Button(actions, text="ลบรายการที่เลือก", command=self.delete_transaction).pack(side="left", padx=6)
        ttk.Button(actions, text="ล้างแบบฟอร์ม", command=self.clear_form).pack(side="left", padx=6)
        self.ml_label_status = tk.StringVar(value="Select an expense to label for future training.")
        label_actions = ttk.Frame(form)
        label_actions.grid(row=3, column=0, columnspan=5, sticky="w", pady=(8, 0))
        ttk.Label(label_actions, text="Manual review label:").pack(side="left", padx=(4, 8))
        ttk.Button(label_actions, text="Mark normal", command=lambda: self.set_suspicion_label(0)).pack(side="left", padx=4)
        ttk.Button(label_actions, text="Mark suspicious", command=lambda: self.set_suspicion_label(1)).pack(side="left", padx=4)
        ttk.Button(label_actions, text="Clear label", command=lambda: self.set_suspicion_label(None)).pack(side="left", padx=4)
        ttk.Label(label_actions, textvariable=self.ml_label_status, style="Sub.TLabel").pack(side="left", padx=10)

    def _build_ml_insights(self) -> None:
        ttk.Label(
            self.ml_tab,
            text="Models are not trained until enough real history and reviewed labels are available.",
            style="Sub.TLabel",
        ).pack(anchor="w", pady=(0, 14))
        forecast = ttk.LabelFrame(self.ml_tab, text="Expense forecast · Linear Regression", padding=14)
        forecast.pack(fill="x", pady=(0, 12))
        self.forecast_readiness = ttk.Label(forecast, text="", style="CardValue.TLabel")
        self.forecast_readiness.pack(anchor="w")
        ttk.Label(
            forecast,
            text="ใช้ยอดรายจ่ายรายเดือนแยกหมวด ฝึกด้วย cml-lab แล้ววัดผลกับ 3 เดือนล่าสุดเทียบกับการเดาแบบง่าย "
                 "ระบบจะใช้วิธีที่คลาดเคลื่อนน้อยที่สุดเท่านั้น เดือนที่ยังไม่จบจะไม่ถูกนำมาฝึก",
            style="Sub.TLabel",
            wraplength=900,
        ).pack(anchor="w", pady=(6, 8))
        forecast_actions = ttk.Frame(forecast)
        forecast_actions.pack(fill="x")
        self.forecast_button = ttk.Button(forecast_actions, text="ฝึกโมเดลและพยากรณ์", command=self.run_forecast, style="Accent.TButton")
        self.forecast_button.pack(side="left")
        self.forecast_result = ttk.Label(forecast_actions, text="", style="Sub.TLabel", wraplength=760)
        self.forecast_result.pack(side="left", padx=12)
        columns = ("category", "last", "average3", "model", "chosen")
        self.forecast_tree = ttk.Treeview(forecast, columns=columns, show="headings", height=7, selectmode="none")
        headings = {"category": "หมวดหมู่", "last": "เดือนล่าสุด", "average3": "เฉลี่ย 3 เดือน", "model": "โมเดล CML", "chosen": "คาดการณ์ที่ใช้"}
        for column in columns:
            self.forecast_tree.heading(column, text=headings[column])
            self.forecast_tree.column(column, width=150, anchor="w" if column == "category" else "e")
        self.forecast_tree.pack(fill="x", pady=(10, 0))

        classifier = ttk.LabelFrame(self.ml_tab, text="Transaction review · Logistic Regression", padding=14)
        classifier.pack(fill="x", pady=(0, 12))
        self.classifier_readiness = ttk.Label(classifier, text="", style="CardValue.TLabel")
        self.classifier_readiness.pack(anchor="w")
        ttk.Label(
            classifier,
            text="Use the transaction table to mark expenses as normal or suspicious. Labels are human review data; no automatic risk decision is made yet.",
            style="Sub.TLabel",
            wraplength=900,
        ).pack(anchor="w", pady=(6, 0))
        ttk.Button(self.ml_tab, text="Refresh readiness", command=self.refresh_ml_readiness).pack(anchor="w")

    def _build_budgets(self) -> None:
        ttk.Label(self.budgets_tab, text="กำหนดวงเงินรายหมวดสำหรับเดือนที่เลือกด้านบน", style="Sub.TLabel").pack(anchor="w", pady=(0, 12))
        self.budget_summary_label = ttk.Label(self.budgets_tab, text="", style="CardValue.TLabel", padding=12)
        self.budget_summary_label.pack(fill="x", pady=(0, 12))
        form = ttk.LabelFrame(self.budgets_tab, text="ตั้งงบประมาณ", padding=12)
        form.pack(fill="x", pady=(0, 14))
        ttk.Label(form, text="หมวดหมู่").grid(row=0, column=0, sticky="w", padx=5)
        ttk.Label(form, text="งบต่อเดือน (บาท)").grid(row=0, column=1, sticky="w", padx=5)
        self.budget_category_box = ttk.Combobox(form, textvariable=self.budget_category, values=EXPENSE_CATEGORIES, state="readonly", width=24)
        self.budget_category_box.grid(row=1, column=0, padx=5, pady=6, sticky="w")
        ttk.Entry(form, textvariable=self.budget_amount, width=20).grid(row=1, column=1, padx=5, pady=6, sticky="w")
        ttk.Button(form, text="บันทึกงบประมาณ", command=self.save_budget, style="Accent.TButton").grid(row=1, column=2, padx=8)
        ttk.Button(form, text="ลบงบหมวดที่เลือก", command=self.delete_budget).grid(row=1, column=3, padx=8)
        ttk.Button(form, text="คัดลอกจากเดือนก่อน", command=self.copy_previous_budgets).grid(row=1, column=4, padx=8)

        columns = ("category", "limit", "spent", "remaining", "daily", "forecast", "status")
        self.budget_tree = ttk.Treeview(self.budgets_tab, columns=columns, show="headings", selectmode="browse", height=15)
        headings = {"category": "หมวดหมู่", "limit": "งบ/เดือน", "spent": "ใช้ไปแล้ว", "remaining": "คงเหลือ", "daily": "ใช้ได้/วัน", "forecast": "คาดการณ์สิ้นเดือน", "status": "สถานะ"}
        widths = {"category": 150, "limit": 110, "spent": 110, "remaining": 110, "daily": 110, "forecast": 150, "status": 120}
        for column in columns:
            self.budget_tree.heading(column, text=headings[column])
            self.budget_tree.column(column, width=widths[column], anchor="w" if column == "category" else "center")
        self.budget_tree.tag_configure("over", foreground=RED)
        self.budget_tree.tag_configure("ok", foreground=GREEN)
        self.budget_tree.pack(fill="both", expand=True)
        self.budget_tree.bind("<<TreeviewSelect>>", self.select_budget)

    def _selected_month(self) -> str:
        return validate_month(self.month.get())

    def _update_categories(self) -> None:
        values = INCOME_CATEGORIES if self.tx_type.get() == INCOME else EXPENSE_CATEGORIES
        self.category_box.configure(values=values)
        if self.tx_category.get() not in values:
            self.tx_category.set(values[0])

    def refresh_all(self) -> None:
        try:
            self._selected_month()
            self.refresh_dashboard()
            self.refresh_transactions()
            self.refresh_budgets()
            self.refresh_ml_readiness()
        except ValueError as exc:
            messagebox.showerror("เดือนที่ระบุไม่ถูกต้อง", str(exc), parent=self)
        except sqlite3.Error as exc:
            messagebox.showerror("อ่านฐานข้อมูลไม่สำเร็จ", str(exc), parent=self)

    def refresh_dashboard(self) -> None:
        month = self._selected_month()
        totals = self.service.summary(month)
        self.card_values["income"].configure(text=format_money(totals["month_income"]))
        self.card_values["expense"].configure(text=format_money(totals["month_expense"]))
        self.card_values["balance"].configure(text=format_money(totals["balance"]))
        self.trend_data = self.service.monthly_trend(month)
        self.category_data = self.service.category_expenses(month)
        self._draw_trend()
        self._draw_categories()

    def _draw_trend(self) -> None:
        canvas = self.trend_canvas
        canvas.delete("all")
        if not hasattr(self, "trend_data"):
            return
        width, height = max(canvas.winfo_width(), 420), max(canvas.winfo_height(), 280)
        left, right, top, bottom = 60, 18, 36, 42
        plot_h, plot_w = height - top - bottom, width - left - right
        max_value = max((max(row["income"], row["expense"]) for row in self.trend_data), default=0) or 1
        for i in range(5):
            y = top + plot_h * i / 4
            canvas.create_line(left, y, width-right, y, fill="#e8ecf2")
            canvas.create_text(left-8, y, text=self._short_money(max_value * (4-i) / 4), anchor="e", fill=MUTED, font=("Tahoma", 8))
        group_w = plot_w / max(len(self.trend_data), 1)
        bar_w = min(19, group_w * 0.28)
        for index, row in enumerate(self.trend_data):
            center = left + group_w * (index + 0.5)
            for offset, value, color in ((-bar_w/2, row["income"], GREEN), (bar_w/2, row["expense"], RED)):
                bar_h = plot_h * value / max_value
                x1, x2 = center + offset - bar_w/2 + 1, center + offset + bar_w/2 - 1
                canvas.create_rectangle(x1, top+plot_h-bar_h, x2, top+plot_h, fill=color, outline="")
            canvas.create_text(center, height-bottom+16, text=row["month"][2:], fill=MUTED, font=("Tahoma", 8))
        canvas.create_text(left, 12, text="■ รายรับ", anchor="w", fill=GREEN, font=("Tahoma", 9, "bold"))
        canvas.create_text(left+90, 12, text="■ รายจ่าย", anchor="w", fill=RED, font=("Tahoma", 9, "bold"))
        canvas.create_line(left, top, left, top+plot_h, fill="#bbc4d1")

    @staticmethod
    def _short_money(cents: float) -> str:
        amount = cents / 100
        if amount >= 1_000_000:
            return f"{amount/1_000_000:.1f} ล."
        if amount >= 1_000:
            return f"{amount/1_000:.0f} พัน"
        return f"{amount:.0f}"

    def _draw_categories(self) -> None:
        canvas = self.category_canvas
        canvas.delete("all")
        if not hasattr(self, "category_data"):
            return
        width, height = max(canvas.winfo_width(), 420), max(canvas.winfo_height(), 280)
        rows = self.category_data[:9]
        if not rows:
            canvas.create_text(width/2, height/2, text="เดือนนี้ยังไม่มีรายจ่าย", fill=MUTED, font=("Tahoma", 11))
            return
        max_value = max(row["total"] for row in rows) or 1
        label_w, amount_w = 112, 95
        bar_x, bar_w = label_w + 8, width - label_w - amount_w - 36
        row_h = min(34, (height - 20) / len(rows))
        for index, row in enumerate(rows):
            y = 18 + index * row_h
            canvas.create_text(label_w, y+9, text=row["category"], anchor="e", fill="#344054", font=("Tahoma", 9))
            canvas.create_rectangle(bar_x, y+2, bar_x+bar_w, y+18, fill="#edf1f6", outline="")
            canvas.create_rectangle(bar_x, y+2, bar_x+bar_w*row["total"]/max_value, y+18, fill=BLUE, outline="")
            canvas.create_text(width-8, y+10, text=self._short_money(row["total"]), anchor="e", fill=MUTED, font=("Tahoma", 8))
        canvas.create_text(width-10, height-6, text="หน่วย: บาท (ย่อ)", anchor="se", fill=MUTED, font=("Tahoma", 8))

    def refresh_transactions(self) -> None:
        for item in self.tx_tree.get_children():
            self.tx_tree.delete(item)
        for row in self.service.search_transactions(self.search_text.get()):
            review_label = {None: "Unlabeled", 0: "Normal", 1: "Suspicious"}.get(
                row["suspicion_label"], "Unlabeled"
            )
            self.tx_tree.insert("", "end", iid=str(row["id"]), values=(
                row["id"], row["date"], row["type"], row["category"],
                format_money(row["amount_cents"]), row["description"], review_label,
            ), tags=(row["type"],))

    def clear_search(self) -> None:
        self.search_text.set("")
        self.refresh_transactions()

    def _form_values(self) -> tuple[str, str, str, str, str]:
        return self.tx_type.get(), self.tx_category.get(), self.tx_amount.get(), self.tx_date.get(), self.tx_description.get()

    def add_transaction(self) -> None:
        try:
            self.service.add_transaction(*self._form_values())
            self.clear_form()
            self.refresh_all()
            messagebox.showinfo("บันทึกแล้ว", "เพิ่มรายการเรียบร้อย", parent=self)
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showerror("บันทึกไม่สำเร็จ", str(exc), parent=self)

    def update_transaction(self) -> None:
        if self.selected_transaction is None:
            messagebox.showwarning("ยังไม่ได้เลือกรายการ", "เลือกรายการจากตารางก่อนแก้ไข", parent=self)
            return
        try:
            self.service.update_transaction(self.selected_transaction, *self._form_values())
            self.clear_form()
            self.refresh_all()
            messagebox.showinfo("บันทึกแล้ว", "แก้ไขรายการเรียบร้อย", parent=self)
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showerror("บันทึกไม่สำเร็จ", str(exc), parent=self)

    def delete_transaction(self) -> None:
        if self.selected_transaction is None:
            messagebox.showwarning("ยังไม่ได้เลือกรายการ", "เลือกรายการจากตารางก่อนลบ", parent=self)
            return
        if not messagebox.askyesno("ยืนยันการลบ", "ต้องการลบรายการนี้หรือไม่?", parent=self):
            return
        self.service.delete_transaction(self.selected_transaction)
        self.clear_form()
        self.refresh_all()

    def select_transaction(self, _event: object = None) -> None:
        selection = self.tx_tree.selection()
        if not selection:
            return
        self.selected_transaction = int(selection[0])
        row = self.service.get_transaction(self.selected_transaction)
        if row is None:
            return
        self.tx_type.set(row["type"])
        self._update_categories()
        self.tx_category.set(row["category"])
        self.tx_date.set(row["date"])
        self.tx_amount.set(f"{row['amount_cents'] / 100:.2f}")
        self.tx_description.set(row["description"])
        label = row["suspicion_label"]
        label_text = {None: "Unlabeled", 0: "Normal", 1: "Suspicious"}.get(label, "Unlabeled")
        self.ml_label_status.set(f"Selected expense label: {label_text}")

    def set_suspicion_label(self, label: int | None) -> None:
        if self.selected_transaction is None:
            messagebox.showwarning(
                "Select a transaction", "Select an expense transaction first.", parent=self
            )
            return
        try:
            if not self.service.set_suspicion_label(self.selected_transaction, label):
                messagebox.showwarning(
                    "Expense only", "Only expense transactions can be labeled.", parent=self
                )
                return
            label_text = {None: "Unlabeled", 0: "Normal", 1: "Suspicious"}[label]
            self.ml_label_status.set(f"Selected expense label: {label_text}")
            self.refresh_transactions()
            self.refresh_ml_readiness()
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showerror("Could not save label", str(exc), parent=self)

    def refresh_ml_readiness(self) -> None:
        try:
            readiness = self.service.ml_data_readiness(self._selected_month())
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showerror("Could not read ML data readiness", str(exc), parent=self)
            return
        forecast_status = "พร้อมพยากรณ์" if readiness["forecast_ready"] else "ประวัติยังน้อย (ฝึกได้ตั้งแต่ 8 เดือน แต่ผลจะแกว่ง)"
        classifier_status = "ready for model implementation" if readiness["classifier_ready"] else "not enough reviewed labels yet"
        self.forecast_readiness.configure(
            text=(f"{readiness['expense_months']} / {readiness['forecast_months_required']} expense months · {forecast_status}")
        )
        self.classifier_readiness.configure(
            text=(
                f"Normal {readiness['normal_labels']} / {readiness['labels_required_per_class']} · "
                f"Suspicious {readiness['suspicious_labels']} / {readiness['labels_required_per_class']} · "
                f"{classifier_status}"
            )
        )

    def run_forecast(self) -> None:
        try:
            data = prepare_forecast(self.service, self._selected_month())
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showinfo("ยังพยากรณ์ไม่ได้", str(exc), parent=self)
            return
        self.forecast_button.configure(state="disabled", text="กำลังฝึกโมเดล…")
        outcome: queue.Queue = queue.Queue(maxsize=1)

        def worker() -> None:
            # Runs the CLI only; Tk widgets are touched on the main thread via polling.
            try:
                outcome.put((run_forecast(data), None))
            except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as exc:
                outcome.put((None, str(exc)))

        def poll() -> None:
            try:
                result, error = outcome.get_nowait()
            except queue.Empty:
                self.after(100, poll)
                return
            self._apply_forecast(result, error)
        threading.Thread(target=worker, daemon=True).start()
        self.after(100, poll)

    def _apply_forecast(self, result: ForecastResult | None, error: str | None) -> None:
        self.forecast_button.configure(state="normal", text="ฝึกโมเดลและพยากรณ์")
        if error or result is None:
            self.forecast_result.configure(text="")
            messagebox.showerror("พยากรณ์ไม่สำเร็จ", error or "ไม่ทราบสาเหตุ", parent=self)
            return
        self.forecast_tree.delete(*self.forecast_tree.get_children())
        baht = lambda value: format_money(round(value * 100))
        for item in result.categories:
            self.forecast_tree.insert("", "end", values=(
                item.category, baht(item.last_month), baht(item.average3), baht(item.model), baht(item.chosen)))
        method = {"model": "โมเดล CML", "average3": "ค่าเฉลี่ย 3 เดือน", "last_month": "เท่าเดือนล่าสุด"}[result.method]
        self.forecast_result.configure(text=(
            f"คาดการณ์รายจ่ายเดือน {result.target_month}: {baht(result.total)} · ใช้{method} · "
            f"ทดสอบ {result.test_rows} แถว MAE: โมเดล {baht(result.model_mae)}, "
            f"เฉลี่ย 3 เดือน {baht(result.average3_mae)}, เท่าเดือนล่าสุด {baht(result.last_month_mae)}"
        ))

    def clear_form(self) -> None:
        self.selected_transaction = None
        self.tx_type.set(EXPENSE)
        self._update_categories()
        self.tx_amount.set("")
        self.tx_date.set(date.today().isoformat())
        self.tx_description.set("")
        self.ml_label_status.set("Select an expense to label for future training.")
        for selected in self.tx_tree.selection():
            self.tx_tree.selection_remove(selected)

    def refresh_budgets(self) -> None:
        for item in self.budget_tree.get_children():
            self.budget_tree.delete(item)
        rows = self.service.budget_status(self._selected_month())
        total_limit = sum(row["limit"] for row in rows)
        total_spent = sum(row["spent"] for row in rows)
        total_remaining = total_limit - total_spent
        total_daily = sum(row["daily_allowance"] for row in rows)
        self.budget_summary_label.configure(text=(
            f"งบรวม {format_money(total_limit)}   ·   ใช้ไป {format_money(total_spent)}   ·   "
            f"คงเหลือ {format_money(total_remaining)}   ·   ใช้ได้เฉลี่ย {format_money(total_daily)}/วัน"
        ))
        for row in rows:
            status = "เกินงบ" if row["over_budget"] else "อยู่ในงบ"
            if row["projected_over"] and not row["over_budget"]:
                status = f"เสี่ยงเกิน ({row['used_percent']}%)"
            elif row["over_budget"]:
                status = f"เกินงบ ({row['used_percent']}%)"
            else:
                status = f"อยู่ในงบ ({row['used_percent']}%)"
            tag = "over" if row["over_budget"] or row["projected_over"] else "ok"
            self.budget_tree.insert("", "end", iid=row["category"], values=(
                row["category"], format_money(row["limit"]), format_money(row["spent"]),
                format_money(row["remaining"]), format_money(row["daily_allowance"]),
                format_money(row["projected"]), status,
            ), tags=(tag,))

    def copy_previous_budgets(self) -> None:
        from .services import shift_month
        try:
            target = self._selected_month()
            source = shift_month(target, -1)
            copied = self.service.copy_budgets(source, target)
            self.refresh_budgets()
            if copied:
                messagebox.showinfo("คัดลอกงบแล้ว", f"นำงบ {copied} หมวดจาก {source} มาไว้ใน {target}", parent=self)
            else:
                messagebox.showinfo("ไม่พบงบเดือนก่อน", f"เดือน {source} ยังไม่มีงบประมาณให้คัดลอก", parent=self)
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showerror("คัดลอกงบไม่สำเร็จ", str(exc), parent=self)

    def save_budget(self) -> None:
        try:
            self.service.set_budget(self._selected_month(), self.budget_category.get(), self.budget_amount.get())
            self.budget_amount.set("")
            self.refresh_budgets()
            messagebox.showinfo("บันทึกแล้ว", "บันทึกงบประมาณเรียบร้อย", parent=self)
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showerror("บันทึกไม่สำเร็จ", str(exc), parent=self)

    def select_budget(self, _event: object = None) -> None:
        selection = self.budget_tree.selection()
        if selection:
            row = self.budget_tree.item(selection[0], "values")
            self.budget_category.set(row[0])
            self.budget_amount.set(row[1].replace(",", "").replace(" บาท", ""))

    def delete_budget(self) -> None:
        selection = self.budget_tree.selection()
        if not selection:
            messagebox.showwarning("ยังไม่ได้เลือกงบ", "เลือกงบประมาณจากตารางก่อนลบ", parent=self)
            return
        category = selection[0]
        if not messagebox.askyesno("ยืนยันการลบ", f"ลบงบหมวด {category} ของเดือน {self.month.get()} หรือไม่?", parent=self):
            return
        try:
            self.service.delete_budget(self._selected_month(), category)
            self.refresh_budgets()
        except (ValueError, sqlite3.Error) as exc:
            messagebox.showerror("ลบไม่สำเร็จ", str(exc), parent=self)

    def close(self) -> None:
        self.database.close()
        self.destroy()
