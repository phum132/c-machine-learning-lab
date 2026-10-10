"""Launch the Personal Finance Tracker desktop application."""

from tkinter import messagebox

from finance.ui import FinanceApp


def main() -> None:
    try:
        FinanceApp().mainloop()
    except (OSError, RuntimeError) as error:
        messagebox.showerror("เปิดโปรแกรมไม่สำเร็จ", str(error))


if __name__ == "__main__":
    main()
