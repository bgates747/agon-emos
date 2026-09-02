from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
MOS_SOURCE = ROOT
SCRIPT = ROOT / "projects" / "emos" / "verify_abi.py"
SPEC = importlib.util.spec_from_file_location("verify_emos_abi", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
abi = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(abi)


class EmosAbiTests(unittest.TestCase):
    def test_source_keeps_alias_builtin_module_file_precedence(self) -> None:
        mos = (MOS_SOURCE / "src" / "mos.c").read_text(
            encoding="utf-8"
        )
        positions = [
            mos.index("result = mos_execAlias"),
            mos.index("cmd = mos_getCommand(command, MATCH_COMMANDS);"),
            mos.index("result = emos_dispatch_command(command, ptr, &moduleMatched);"),
            mos.index("// Command not built-in, so see if it's a file"),
        ]
        self.assertEqual(positions, sorted(positions))
        core = (MOS_SOURCE / "src" / "emos.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("canonical[index] = tolower", core)

    def test_source_freezes_noncolliding_gateway_numbers(self) -> None:
        api = (MOS_SOURCE / "src" / "mos_api.inc").read_text(
            encoding="utf-8"
        )
        self.assertIn("mos_emos_gateway:\tEQU\t51h", api)
        self.assertIn("mos_function_emos_gateway:\tEQU\t20h", api)
        self.assertIn("0x12 is deliberately left available", api)

    def fixture(self) -> tuple[bytearray, dict[str, int]]:
        image = bytearray(0x800)
        linked = {
            "_emos_gateway": 0x600,
            "mos_api_emos_gateway": 0x400,
            "mos_api_block1_start": 0x100,
            "mos_function_block_start": 0x200,
            "mos_function_block_size": 0x21,
            "_open_UART1": 0x700,
        }
        image[0x100 + 0x51 * 2 : 0x100 + 0x51 * 2 + 2] = (0x400).to_bytes(
            2, "little"
        )
        image[0x200 + 0x20 * 3 : 0x200 + 0x21 * 3] = (0x600).to_bytes(
            3, "little"
        )
        image[0x200 + 0x08 * 3 : 0x200 + 0x09 * 3] = (0x700).to_bytes(
            3, "little"
        )
        image[0x400:0x40F] = bytes(
            [0xED, 0x6E, 0xB7, 0xC2, 0, 0, 0, 0xE5, 0xCD, 0, 6, 0, 0x7D, 0xE1, 0xC9]
        )
        return image, linked

    def test_accepts_exact_gateway_layout(self) -> None:
        image, linked = self.fixture()
        abi.verify(image, linked)

    def test_rejects_slot_wrapper_and_reserved_drift(self) -> None:
        for offset in (
            0x100 + 0x51 * 2,
            0x200 + 0x08 * 3,
            0x200 + 0x12 * 3,
            0x400 + 8,
        ):
            image, linked = self.fixture()
            image[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(abi.AbiError):
                abi.verify(image, linked)


if __name__ == "__main__":
    unittest.main()
