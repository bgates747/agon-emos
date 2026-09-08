from __future__ import annotations

import importlib.util
import hashlib
from pathlib import Path
import tempfile
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "projects/emos/verify_runtime.py"
SPEC = importlib.util.spec_from_file_location("verify_emos_runtime", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
runtime = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runtime)


IDENTITY = b"EMOS identity: agon-emos-v0.2.0, build agon-emos-v0.2.0-b2026-09-07-23-00-00Z, status draft"


def valid_output() -> bytes:
    return b"\r\n".join(IDENTITY if token == b"{identity}" else token
                          for token in runtime.EXPECTED_SEQUENCE) + b"\r\n/ *"


class EmosRuntimeTests(unittest.TestCase):
    def test_accepts_complete_ordered_runtime_transcript(self) -> None:
        runtime.validate_output(valid_output(), IDENTITY)

    def test_rejects_wrong_build_identity_at_boot_or_later_status(self) -> None:
        output = valid_output()
        with self.assertRaises(runtime.EmosRuntimeError):
            runtime.validate_output(output.replace(IDENTITY, b"wrong build", 1), IDENTITY)
        with self.assertRaises(runtime.EmosRuntimeError):
            runtime.validate_output(output.rsplit(IDENTITY, 1)[0], IDENTITY)

    def test_manifest_binds_identity_to_exact_firmware(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            firmware = Path(raw) / "MOS.bin"
            firmware.write_bytes(b"reviewed firmware")
            manifest = Path(raw) / "build-manifest.yaml"
            manifest.write_text(yaml.safe_dump({
                "build": {"source_identity": "agon-emos-v0.2.0",
                          "build_id": "agon-emos-v0.2.0-b2026-09-07-23-00-00Z", "status": "draft"},
                "outputs": [{"filename": "MOS.bin", "sha256": hashlib.sha256(firmware.read_bytes()).hexdigest()}],
            }))
            self.assertEqual(runtime.identity_from_manifest(manifest, firmware), IDENTITY)
            firmware.write_bytes(b"another firmware")
            with self.assertRaisesRegex(runtime.EmosRuntimeError, "manifest"):
                runtime.identity_from_manifest(manifest, firmware)

    def test_rejects_missing_reordered_and_failed_results(self) -> None:
        output = valid_output()
        with self.assertRaisesRegex(runtime.EmosRuntimeError, "missing or reorders"):
            runtime.validate_output(output.replace(b"Hello\r\n", b"", 1), IDENTITY)
        with self.assertRaisesRegex(runtime.EmosRuntimeError, "forbidden result"):
            runtime.validate_output(output + b"Invalid command\r\n", IDENTITY)
        with self.assertRaisesRegex(runtime.EmosRuntimeError, "command echo count"):
            runtime.validate_output(output.replace(b"/ *HELLO", b"HELLO", 1), IDENTITY)

    def test_media_snapshot_requires_exact_readonly_fixture(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            sdcard = Path(raw)
            modules = sdcard / "emos/modules"
            modules.mkdir(parents=True)
            for name in ("hello.emo", "echo.emo", "edu-probe.emo"):
                (modules / name).write_bytes(name.encode("ascii"))
            (sdcard / "emos.commands").write_bytes(runtime.COMMAND_FILE)
            snapshot = runtime.snapshot_media(sdcard)
            self.assertEqual(
                {name for name, (kind, _) in snapshot.items() if kind == "file"},
                runtime.MEDIA_FILES,
            )
            (modules / "extra.emo").write_bytes(b"extra")
            with self.assertRaisesRegex(runtime.EmosRuntimeError, "inventory"):
                runtime.snapshot_media(sdcard)

    def test_media_snapshot_rejects_command_or_symlink_drift(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            sdcard = Path(raw)
            modules = sdcard / "emos/modules"
            modules.mkdir(parents=True)
            for name in ("hello.emo", "echo.emo", "edu-probe.emo"):
                (modules / name).write_bytes(name.encode("ascii"))
            commands = sdcard / "emos.commands"
            commands.write_bytes(b"EMOS STATUS\r\n")
            with self.assertRaisesRegex(runtime.EmosRuntimeError, "command file"):
                runtime.snapshot_media(sdcard)
            commands.write_bytes(runtime.COMMAND_FILE)
            (modules / "hello.emo").unlink()
            (modules / "hello.emo").symlink_to(modules / "echo.emo")
            with self.assertRaisesRegex(runtime.EmosRuntimeError, "symlink"):
                runtime.snapshot_media(sdcard)


if __name__ == "__main__":
    unittest.main()
