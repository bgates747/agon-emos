from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "scripts/verify_hardware_capture.py"
SPEC = importlib.util.spec_from_file_location("verify_hardware_capture", SCRIPT)
assert SPEC and SPEC.loader
capture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(capture)


class HardwareCaptureTests(unittest.TestCase):
    def fixture(self, root: Path) -> tuple[Path, Path]:
        firmware = root / "MOS.bin"
        firmware.write_bytes(b"candidate")
        log = root / "raw.log"
        log.write_bytes(b"real capture bytes")
        document = {
            "schema": 1,
            "operator": "Author",
            "timestamp_utc": "2026-08-24T12:00:00Z",
            "hardware": {name: "recorded" for name in (
                "board", "revision", "clock", "storage", "vdp", "uart", "edp"
            )},
            "firmware_sha256": hashlib.sha256(firmware.read_bytes()).hexdigest(),
            "vdp_sha256": "1" * 64,
            "recovery_sha256": "2" * 64,
            "stages": [
                {"id": name, "result": "pass", "notes": "observed"}
                for name in capture.STAGES
            ],
            "artifacts": [{
                "path": log.name,
                "sha256": hashlib.sha256(log.read_bytes()).hexdigest(),
            }],
            "approval": "author-approved-real-hardware",
        }
        manifest = root / "capture.json"
        manifest.write_text(json.dumps(document), encoding="utf-8")
        return manifest, firmware

    def test_accepts_complete_hash_bound_capture(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            manifest, firmware = self.fixture(Path(directory))
            capture.validate(manifest, firmware)

    def test_rejects_fake_approval_stage_failure_and_artifact_drift(self) -> None:
        for mutation in ("approval", "stage", "artifact"):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                manifest, firmware = self.fixture(root)
                data = json.loads(manifest.read_text(encoding="utf-8"))
                if mutation == "approval":
                    data["approval"] = "synthetic"
                elif mutation == "stage":
                    data["stages"][2]["result"] = "fail"
                else:
                    (root / "raw.log").write_bytes(b"changed")
                manifest.write_text(json.dumps(data), encoding="utf-8")
                with self.assertRaises(capture.CaptureError):
                    capture.validate(manifest, firmware)


if __name__ == "__main__":
    unittest.main()
