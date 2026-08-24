from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


emos = load("emos_module_media", ROOT / "projects" / "emos" / "emos_module.py")
media = load("build_emos_media", ROOT / "projects" / "emos" / "build_media.py")


def manifest(provider_class: str, namespace: str, name: str) -> dict:
    return {
        "schema": 1,
        "class": provider_class,
        "namespace": namespace,
        "name": name,
        "version": [1, 0, 0],
        "core_abi": [1, 1],
        "entry_offset": 128,
        "request_size": [24, 24],
        "flags": 0,
        "capabilities": 0,
    }


class EmosMediaTests(unittest.TestCase):
    def test_sample_service_provider_rejects_other_operations(self) -> None:
        source = (ROOT / "projects" / "emos" / "src" / "echo.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("request->operation[0] != 2", source)
        self.assertIn("request->operation[1] != 0", source)

    def test_case_tree_is_deterministic_and_semantically_bounded(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            valid = root / "providers"
            valid.mkdir()
            identities = (
                ("hello.emo", "star", "", "hello"),
                ("echo.emo", "service", "core", "echo"),
                ("edu-probe.emo", "service", "edu", "probe"),
            )
            for filename, provider_class, namespace, name in identities:
                (valid / filename).write_bytes(
                    emos.build_container(
                        manifest(provider_class, namespace, name), b"payload"
                    )
                )
            first = root / "first"
            second = root / "second"
            media.build(valid, first)
            media.build(valid, second)
            first_files = {
                path.relative_to(first): path.read_bytes()
                for path in first.rglob("*")
                if path.is_file()
            }
            second_files = {
                path.relative_to(second): path.read_bytes()
                for path in second.rglob("*")
                if path.is_file()
            }
            self.assertEqual(first_files, second_files)

            valid_modules = first / "valid" / "sdcard" / "emos" / "modules"
            self.assertEqual(len(emos.validate_registry(
                (path.name, path.read_bytes()) for path in valid_modules.iterdir()
            )), 3)
            collision = first / "collision" / "sdcard" / "emos" / "modules"
            with self.assertRaisesRegex(emos.ModuleError, "duplicate provider"):
                emos.validate_registry(
                    (path.name, path.read_bytes()) for path in collision.iterdir()
                )
            corrupt = first / "corruption" / "sdcard" / "emos" / "modules" / "hello.emo"
            with self.assertRaisesRegex(emos.ModuleError, "payload CRC"):
                emos.validate_container(corrupt.read_bytes())

            eligibility = first / "eligibility" / "headers"
            self.assertEqual(
                emos.application_policy((eligibility / "module-safe.bin").read_bytes(), 0x40000),
                emos.POLICY_SAFE,
            )
            self.assertEqual(
                emos.application_policy((eligibility / "module-compatible.bin").read_bytes(), 0x40000),
                emos.POLICY_COMPATIBLE,
            )
            for denied in (
                "unaware.bin",
                "version-zero.bin",
                "z80.bin",
                "reserved-flag.bin",
                "conflicting-flags.bin",
            ):
                with self.subTest(denied=denied):
                    self.assertEqual(
                        emos.application_policy((eligibility / denied).read_bytes(), 0x40000),
                        emos.POLICY_UNSAFE,
                    )
            index = json.loads((first / "index.json").read_text(encoding="utf-8"))
            self.assertFalse(index["physical_edp_claim"])


if __name__ == "__main__":
    unittest.main()
