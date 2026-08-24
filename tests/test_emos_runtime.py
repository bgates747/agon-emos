from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "projects/emos/verify_runtime.py"
SPEC = importlib.util.spec_from_file_location("verify_emos_runtime", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
runtime = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(runtime)


def valid_output() -> bytes:
    return b"\r\n".join(runtime.EXPECTED_SEQUENCE) + b"\r\n/ *"


class EmosRuntimeTests(unittest.TestCase):
    def test_accepts_complete_ordered_runtime_transcript(self) -> None:
        runtime.validate_output(valid_output())

    def test_rejects_missing_reordered_and_failed_results(self) -> None:
        output = valid_output()
        with self.assertRaisesRegex(runtime.EmosRuntimeError, "missing or reorders"):
            runtime.validate_output(output.replace(b"Hello\r\n", b"", 1))
        with self.assertRaisesRegex(runtime.EmosRuntimeError, "forbidden result"):
            runtime.validate_output(output + b"Invalid command\r\n")
        with self.assertRaisesRegex(runtime.EmosRuntimeError, "command echo count"):
            runtime.validate_output(output.replace(b"/ *HELLO", b"HELLO", 1))

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
