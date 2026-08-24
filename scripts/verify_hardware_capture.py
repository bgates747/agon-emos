#!/usr/bin/env python3
"""Validate a real PORT-203 hardware capture and all referenced artifacts."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path


STAGES = (
    "boot",
    "storage",
    "rtc",
    "uart-vdp",
    "applications",
    "emos",
    "failure-recovery",
)
SHA256 = re.compile(r"[0-9a-f]{64}")


class CaptureError(RuntimeError):
    pass


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def exact_keys(value: object, keys: set[str], context: str) -> dict[str, object]:
    if not isinstance(value, dict) or set(value) != keys:
        raise CaptureError(f"{context} must contain exactly {sorted(keys)}")
    return value


def validate(capture_path: Path, firmware: Path) -> None:
    root = exact_keys(
        json.loads(capture_path.read_text(encoding="utf-8")),
        {"schema", "operator", "timestamp_utc", "hardware", "firmware_sha256",
         "vdp_sha256", "recovery_sha256", "stages", "artifacts", "approval"},
        "capture",
    )
    if root["schema"] != 1:
        raise CaptureError("capture schema must be 1")
    for name in ("operator", "timestamp_utc", "approval"):
        if not isinstance(root[name], str) or not root[name].strip():
            raise CaptureError(f"{name} must be a nonempty string")
    if root["approval"] != "author-approved-real-hardware":
        raise CaptureError("approval must be author-approved-real-hardware")
    hardware = exact_keys(
        root["hardware"],
        {"board", "revision", "clock", "storage", "vdp", "uart", "edp"},
        "hardware",
    )
    if any(not isinstance(value, str) or not value.strip() for value in hardware.values()):
        raise CaptureError("every hardware identity must be a nonempty string")
    for name in ("firmware_sha256", "vdp_sha256", "recovery_sha256"):
        if not isinstance(root[name], str) or SHA256.fullmatch(root[name]) is None:
            raise CaptureError(f"{name} must be a lowercase SHA-256")
    actual_firmware = digest(firmware)
    if root["firmware_sha256"] != actual_firmware:
        raise CaptureError(
            f"firmware hash mismatch: expected {root['firmware_sha256']}, got {actual_firmware}"
        )

    stages = root["stages"]
    if not isinstance(stages, list) or len(stages) != len(STAGES):
        raise CaptureError("capture must contain every hardware stage once")
    for expected, raw in zip(STAGES, stages, strict=True):
        stage = exact_keys(raw, {"id", "result", "notes"}, f"stage {expected}")
        if stage["id"] != expected or stage["result"] not in {"pass", "skipped"}:
            raise CaptureError(f"stage {expected} is absent, reordered, or failed")
        if not isinstance(stage["notes"], str):
            raise CaptureError(f"stage {expected} notes must be a string")
        if stage["result"] == "skipped" and not stage["notes"].strip():
            raise CaptureError(f"skipped stage {expected} requires a reason")

    artifacts = root["artifacts"]
    if not isinstance(artifacts, list) or not artifacts:
        raise CaptureError("capture must reference at least one raw artifact")
    seen: set[str] = set()
    for ordinal, raw in enumerate(artifacts):
        artifact = exact_keys(raw, {"path", "sha256"}, f"artifact {ordinal}")
        relative = artifact["path"]
        expected_hash = artifact["sha256"]
        if not isinstance(relative, str) or not relative or Path(relative).is_absolute():
            raise CaptureError(f"artifact {ordinal} path must be relative")
        if relative in seen or ".." in Path(relative).parts:
            raise CaptureError(f"artifact {ordinal} path is duplicate or escapes capture")
        seen.add(relative)
        if not isinstance(expected_hash, str) or SHA256.fullmatch(expected_hash) is None:
            raise CaptureError(f"artifact {ordinal} has an invalid SHA-256")
        path = capture_path.parent / relative
        if not path.is_file() or path.is_symlink():
            raise CaptureError(f"artifact is missing, non-regular, or symlinked: {relative}")
        if digest(path) != expected_hash:
            raise CaptureError(f"artifact hash mismatch: {relative}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware", type=Path, required=True)
    parser.add_argument("capture", type=Path)
    args = parser.parse_args()
    try:
        validate(args.capture.resolve(), args.firmware.resolve())
    except (CaptureError, OSError, UnicodeError, json.JSONDecodeError) as error:
        print(f"verify_hardware_capture.py: {error}", file=sys.stderr)
        return 1
    print("PORT-203 hardware capture is complete, hash-bound, and author-approved")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

