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


class Port008PredecessorEvidenceTests(unittest.TestCase):
    def test_predecessor_sender_and_profile_are_visibly_retired(self) -> None:
        verify.validate_source(ROOT)
        readme = (PROJECT / "README.md").read_text(encoding="utf-8")
        profile = (ROOT / "port" / "port008-forward.mk").read_text(
            encoding="utf-8"
        )
        self.assertIn("predecessor evidence only", readme)
        self.assertIn("SUPERSEDED", profile)
        self.assertIn("$(error", profile)

    def test_fixture_generation_is_exact_and_deterministic(self) -> None:
        document, payload = fixture.load_manifest(PROJECT / "fixture.json")
        self.assertEqual(len(payload), 106)
        self.assertEqual(
            hashlib.sha256(payload).hexdigest(), document["command_sha256"]
        )
        first = fixture.assembly(payload)
        second = fixture.assembly(payload)
        self.assertEqual(first, second)
        self.assertIn("rst.lil 0x18", first)
        self.assertNotIn("out0", first.lower())
        with tempfile.TemporaryDirectory() as raw:
            output = Path(raw) / "fixture.asm"
            output.write_text(first, encoding="utf-8")
            self.assertEqual(output.read_text(encoding="utf-8"), second)

    def test_retained_checker_makes_no_sender_claim(self) -> None:
        checker = (PROJECT / "verify.py").read_text(encoding="utf-8")
        self.assertIn("predecessor evidence", checker)
        self.assertNotIn("validate_linked_sender", checker)
        self.assertNotIn("validate_sender_disassembly", checker)


if __name__ == "__main__":
    unittest.main()
