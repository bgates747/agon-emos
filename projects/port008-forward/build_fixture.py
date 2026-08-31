#!/usr/bin/env python3
"""Generate the ordinary ADL RST 18h fixture from its frozen byte manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


class FixtureError(ValueError):
    pass


def load_manifest(path: Path) -> tuple[dict, bytes]:
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != 1:
        raise FixtureError("unsupported fixture schema")
    try:
        payload = bytes.fromhex(document["command_hex"])
    except (KeyError, ValueError) as exc:
        raise FixtureError("command_hex is missing or malformed") from exc
    digest = hashlib.sha256(payload).hexdigest()
    if digest != document.get("command_sha256"):
        raise FixtureError("command byte hash does not match the manifest")
    if len(payload) != 106:
        raise FixtureError(f"fixture must contain 106 bytes, found {len(payload)}")
    if document.get("load_address") != 0x040000:
        raise FixtureError("fixture load address must remain 0x040000")
    if document.get("reverse_transport") != "disabled":
        raise FixtureError("PORT-008 fixture must not claim a return transport")
    return document, payload


def assembly(payload: bytes) -> str:
    rows = []
    for offset in range(0, len(payload), 16):
        values = ", ".join(f"0x{value:02x}" for value in payload[offset : offset + 16])
        rows.append(f"        .byte {values}")
    return "\n".join(
        (
            "/* Generated from fixture.json; do not edit this build artifact. */",
            "        .assume adl=1",
            "        .section .text",
            "        .global _start",
            "        .global _port008_fixture_bytes",
            "        .global _port008_fixture_end",
            "_start:",
            "        ld hl,_port008_fixture_bytes",
            "        ld bc,_port008_fixture_end-_port008_fixture_bytes",
            "        xor a",
            "        rst.lil 0x18",
            "        ret",
            "_port008_fixture_bytes:",
            *rows,
            "_port008_fixture_end:",
            "",
        )
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    _, payload = load_manifest(args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(assembly(payload), encoding="utf-8", newline="\n")
    print(f"generated {args.output} with {len(payload)} VDU bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
