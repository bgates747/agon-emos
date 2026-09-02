#!/usr/bin/env python3
"""Verify the retained PORT-008 ordinary-VDU fixture as predecessor evidence.

The PORT-008 GPIO sender and its EMOS build profile are superseded by INTEG-002.
This checker deliberately makes no sender, activation, General Poll, mode, or
qualification claim; it preserves only the deterministic 106-byte application
fixture that still enters through the ordinary RST 18 surface.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import subprocess

from build_fixture import FixtureError, load_manifest


class Port008Error(ValueError):
    pass


def validate_source(root: Path) -> None:
    serial = (root / "src/serial.asm").read_text(encoding="utf-8")
    core = (root / "src/emos.c").read_text(encoding="utf-8")
    obsolete_profile = (root / "port/port008-forward.mk").read_text(
        encoding="utf-8"
    )
    fixed_profile = (root / "port/parallel-fixed-qualification.mk").read_text(
        encoding="utf-8"
    )
    for obsolete in (
        "EMOS_PORT008_FORWARD",
        "PORT008_send",
        "_emos_port008_prepare",
        "_emos_port008_recover",
        "_port008_general_poll",
    ):
        if obsolete in serial + core:
            raise Port008Error(
                f"maintained production source retains predecessor {obsolete}"
            )
    if "SUPERSEDED" not in obsolete_profile or "$(error" not in obsolete_profile:
        raise Port008Error("predecessor build profile is not visibly retired")
    for required in (
        "EMOS_PARALLEL_FIXED_QUALIFICATION=1",
        "EMOS_IDENTITY_CPPFLAGS",
        "EMOS_QUALIFICATION_COMPOSITION_IDENTITY",
    ):
        if required not in fixed_profile:
            raise Port008Error(f"replacement fixed profile lacks {required!r}")


def linked_symbols(nm: Path, elf: Path) -> dict[str, int]:
    output = subprocess.run(
        [str(nm), "-an", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout
    result: dict[str, int] = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 3 and re.fullmatch(r"[0-9A-Fa-f]+", fields[0]):
            result[fields[2]] = int(fields[0], 16)
    return result


def validate_binary(manifest: Path, binary: Path, elf: Path, nm: Path) -> None:
    _, expected = load_manifest(manifest)
    image = binary.read_bytes()
    symbols = linked_symbols(nm, elf)
    required = ("_start", "_port008_fixture_bytes", "_port008_fixture_end")
    missing = [name for name in required if name not in symbols]
    if missing:
        raise Port008Error("fixture image lacks symbols: " + ", ".join(missing))
    start = symbols["_start"]
    payload_start = symbols["_port008_fixture_bytes"] - start
    payload_end = symbols["_port008_fixture_end"] - start
    if payload_start <= 0 or payload_end != len(image):
        raise Port008Error("fixture layout is not one entry followed by payload")
    actual = image[payload_start:payload_end]
    if actual != expected:
        raise Port008Error("linked fixture payload differs from frozen bytes")
    if hashlib.sha256(actual).hexdigest() != hashlib.sha256(expected).hexdigest():
        raise Port008Error("linked fixture payload hash differs")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--nm", type=Path)
    args = parser.parse_args()
    try:
        validate_source(args.source)
        load_manifest(args.manifest)
        artifacts = (args.binary, args.elf, args.nm)
        if any(artifacts) and not all(artifacts):
            raise Port008Error("binary, ELF, and nm must be supplied together")
        if all(artifacts):
            validate_binary(args.manifest, args.binary, args.elf, args.nm)
    except (FixtureError, Port008Error, OSError, subprocess.CalledProcessError) as exc:
        print(f"PORT-008 predecessor-fixture verification failed: {exc}")
        return 2
    print(
        "PORT-008 predecessor fixture retained: 106 ordinary VDU bytes verified; "
        "the obsolete GPIO sender/profile and all qualification claims are excluded"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
