#!/usr/bin/env python3
"""Build deterministic directory-backed EMOS integration case media."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil


VALID_NAMES = ("hello.emo", "echo.emo", "edu-probe.emo")


def _write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def _commands(path: Path, commands: list[str]) -> None:
    _write(path, ("\r\n".join(commands) + "\r\n").encode("ascii"))


def _header(*, version: int = 1, mode: int = 1, flags: int = 1) -> bytes:
    image = bytearray(0x4A)
    image[0x40:0x43] = b"MOS"
    image[0x43] = version
    image[0x44] = mode
    image[0x45] = flags
    image[0x46] = (~flags) & 0xFF
    return bytes(image)


def build(valid: Path, destination: Path) -> None:
    if destination.exists() or destination.is_symlink():
        raise ValueError(f"refusing to replace integration media: {destination}")
    modules = {name: (valid / name).read_bytes() for name in VALID_NAMES}

    valid_sd = destination / "valid" / "sdcard"
    for name, data in modules.items():
        _write(valid_sd / "emos" / "modules" / name, data)
    _commands(
        valid_sd / "emos.commands",
        [
            "EMOS DISCOVER",
            "EMOS STATUS",
            "HELLO",
            "EMOS CALL core.echo deterministic echo",
            "EMOS CALL edu.probe separate edu result",
            "EMOS FAKE ON",
            "EMOS MODE DUAL",
            "EMOS STATUS",
            "EMOS MODE LEGACY",
            "EMOS FAKE OFF",
            "EMOS STATUS",
        ],
    )

    collision_sd = destination / "collision" / "sdcard"
    _write(collision_sd / "emos" / "modules" / "echo-a.emo", modules["echo.emo"])
    _write(collision_sd / "emos" / "modules" / "echo-b.emo", modules["echo.emo"])
    _commands(collision_sd / "emos.commands", ["EMOS DISCOVER", "EMOS STATUS"])

    corrupt_sd = destination / "corruption" / "sdcard"
    corrupt = bytearray(modules["hello.emo"])
    corrupt[-1] ^= 1
    _write(corrupt_sd / "emos" / "modules" / "hello.emo", bytes(corrupt))
    _commands(corrupt_sd / "emos.commands", ["EMOS DISCOVER", "EMOS STATUS"])

    eligibility = destination / "eligibility" / "headers"
    _write(eligibility / "module-safe.bin", _header(flags=1))
    _write(eligibility / "module-compatible.bin", _header(flags=2))
    _write(eligibility / "unaware.bin", b"unheadered")
    _write(eligibility / "version-zero.bin", _header(version=0))
    _write(eligibility / "z80.bin", _header(mode=0))
    _write(eligibility / "reserved-flag.bin", _header(flags=4))
    _write(eligibility / "conflicting-flags.bin", _header(flags=3))

    index = {
        "schema": 1,
        "cases": {
            "valid": "three providers; command and two service calls; fake Dual round trip",
            "collision": "duplicate core.echo claim; prior registry retained",
            "corruption": "payload CRC mismatch; prior registry retained",
            "eligibility": "safe/compatible positives and bounded denial headers",
        },
        "physical_edp_claim": False,
    }
    _write(
        destination / "index.json",
        (json.dumps(index, indent=2, sort_keys=True) + "\n").encode("utf-8"),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("valid", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    build(args.valid, args.destination)
    print(f"built deterministic EMOS integration media at {args.destination}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
