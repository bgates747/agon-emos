from __future__ import annotations

import importlib.util
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]
MOS_SOURCE = ROOT
OFFICIAL_MOS_BASE = "8336409351ee5314e02801a7b72a4f1bb5282519"
SCRIPT = ROOT / "projects" / "emos" / "verify_vdu.py"
SPEC = importlib.util.spec_from_file_location("verify_emos_vdu", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
vdu = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(vdu)


class EmosVduTests(unittest.TestCase):
    def fixture(self) -> tuple[bytearray, dict[str, int]]:
        image = bytearray(0x900)
        linked = {
            "EMOS_vdu_PUTCH": 0x100,
            "EMOS_vdu_port008": 0x10F,
            "EMOS_vdu_onboard": 0x114,
            "EMOS_vdu_WRITE": 0x120,
            "PORT008_vdu_PUTCH": 0x160,
            "PORT008_send": 0x180,
            "UART0_serial_PUTCH": 0x500,
            "UART1_serial_PUTCH": 0x520,
            "UART_serial_NE": 0x119,
            "_emosVduBackend": 0x700,
            "_putch": 0x200,
            "_getch": 0x220,
            "_rst_10_handler": 0x300,
            "_rst_18_handler": 0x320,
            "__rst_38_handler": 0x380,
        }
        image[0x100:0x119] = (
            b"\xf5\x3a\x00\x07\x00\xb7\x28\x0c\xfe\x02\x28\x03"
            b"\xf1\xb7\xc9\xf1\xc3\x60\x01\x00\xf1\xc3\x00\x05\x00"
        )
        image[0x120:0x160] = b"\x00" * 0x40
        image[0x128:0x12C] = b"\xcd\x00\x05\x00"
        image[0x148:0x14C] = b"\xcd\x80\x01\x00"
        putch = b"\xcd\x00\x01\x00"
        write = b"\xcd\x20\x01\x00"
        image[0x208:0x20C] = putch
        image[0x308:0x30C] = putch
        image[0x338:0x33C] = write
        image[0x360:0x364] = putch
        return image, linked

    def test_accepts_fixed_dispatch_paths(self) -> None:
        image, linked = self.fixture()
        vdu.verify(image, linked)

    def test_rejects_route_and_callsite_drift(self) -> None:
        for offset in (0x105, 0x10A, 0x208, 0x338, 0x148):
            image, linked = self.fixture()
            image[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(vdu.VduError):
                vdu.verify(image, linked)

    def test_raw_uart_putch_blocks_match_official_base(self) -> None:
        current = (MOS_SOURCE / "src" / "serial.asm").read_text(
            encoding="utf-8"
        )
        base = subprocess.run(
            ["git", "show", f"{OFFICIAL_MOS_BASE}:src/serial.asm"],
            cwd=MOS_SOURCE,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
        ).stdout
        current_block = current[
            current.index("UART0_serial_PUTCH:") : current.index(
                "; Core EMOS semantic VDU dispatcher"
            )
        ].rstrip()
        base_block = base[
            base.index("UART0_serial_PUTCH:") : base.index(
                "; Called by UART0 and UART1 PUTCH"
            )
        ].rstrip()
        self.assertEqual(current_block, base_block)


if __name__ == "__main__":
    unittest.main()
