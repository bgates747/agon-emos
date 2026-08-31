from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "projects" / "port008-forward"
sys.path.insert(0, str(PROJECT))


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


fixture = load("port008_build_fixture", PROJECT / "build_fixture.py")
verify = load("port008_verify", PROJECT / "verify.py")


class Port008ForwardTests(unittest.TestCase):
    def test_source_boundary_is_fixed_and_forward_only(self) -> None:
        verify.validate_source(ROOT)

    def test_fixture_generation_is_exact_and_deterministic(self) -> None:
        document, payload = fixture.load_manifest(PROJECT / "fixture.json")
        self.assertEqual(len(payload), 106)
        self.assertEqual(hashlib.sha256(payload).hexdigest(), document["command_sha256"])
        first = fixture.assembly(payload)
        second = fixture.assembly(payload)
        self.assertEqual(first, second)
        self.assertIn("rst.lil 0x18", first)
        self.assertNotIn("out0", first.lower())
        with tempfile.TemporaryDirectory() as raw:
            output = Path(raw) / "fixture.asm"
            output.write_text(first, encoding="utf-8")
            self.assertEqual(output.read_text(encoding="utf-8"), second)

    def test_ordinary_profile_does_not_select_adapter(self) -> None:
        ordinary = (ROOT / "port/mos-agondev.mk").read_text(encoding="utf-8")
        prototype = (ROOT / "port/port008-forward.mk").read_text(encoding="utf-8")
        self.assertNotIn("EMOS_PORT008_FORWARD", ordinary)
        self.assertEqual(prototype.count("EMOS_PORT008_FORWARD"), 1)

    def test_linked_sender_requires_compiled_falling_edge_order(self) -> None:
        disassembly = """
00000100 <PORT008_send>:
 100: 78                 ld a,b
 101: fe 10              cp a,0x10
 103: 79                 ld a,c

00000110 <PORT008_wait_ready>:
 110: ed 39 a2           out0 (0xa2),a
 113: ed 39 a2           out0 (0xa2),a
 116: ed 38 a2           in0 a,(0xa2)
 119: e6 10              and a,0x10

00000120 <PORT008_byte_loop>:
 120: 7e                 ld a,(hl)
 121: ed 39 9e           out0 (0x9e),a
 124: ed 39 a2           out0 (0xa2),a
 127: ed 39 a2           out0 (0xa2),a
 12a: 23                 inc hl
 12b: 0b                 dec bc

00000130 <PORT008_wait_release>:
 130: ed 39 a2           out0 (0xa2),a
 133: ed 39 a2           out0 (0xa2),a
 136: ed 38 a2           in0 a,(0xa2)
 139: e6 10              and a,0x10
"""
        verify.validate_sender_disassembly(disassembly)
        with self.assertRaises(verify.Port008Error):
            verify.validate_sender_disassembly(
                disassembly.replace("out0 (0x9e),a", "out0 (0xa2),a", 1)
            )


if __name__ == "__main__":
    unittest.main()
