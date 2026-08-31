from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "projects/emos/verify_uart_baud.py"
SPEC = importlib.util.spec_from_file_location("verify_uart_baud", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
uart_baud = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(uart_baud)


class UartBaudTests(unittest.TestCase):
    def test_source_widens_before_both_uart_products(self) -> None:
        self.assertEqual(uart_baud.verify_source(ROOT), (18_432_000, 1_152_000, 1))

    def test_source_check_rejects_one_unwidened_uart_product(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "src").mkdir()
            for relative in ("main.c", "src/uart.c", "src/uart.h"):
                source = ROOT / relative
                target = root / relative
                target.write_bytes(source.read_bytes())
            uart = root / "src/uart.c"
            text = uart.read_text(encoding="utf-8")
            uart.write_text(
                text.replace(
                    uart_baud.WIDENED_PRODUCT,
                    "CLOCK_DIVISOR_16 * pUART->baudRate",
                    1,
                ),
                encoding="utf-8",
            )
            with self.assertRaises(uart_baud.UartBaudError):
                uart_baud.verify_source(root)

    def test_symbol_parser_and_required_call_are_fail_closed(self) -> None:
        linked = uart_baud.symbols(
            "000001 T _open_UART0\n000123 T __lshl\n000456 T __ldivu\n"
        )
        self.assertEqual(linked["__lshl"], 0x123)
        uart_baud.require_call("100: cd 23 01 00 call 0x123", 0x123, "test")
        with self.assertRaises(uart_baud.UartBaudError):
            uart_baud.require_call("100: ret", 0x123, "test")


if __name__ == "__main__":
    unittest.main()
