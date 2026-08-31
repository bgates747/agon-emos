#!/usr/bin/env python3
"""Verify the fixed-purpose PORT-008 source boundary and fixture artifact."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import subprocess

from build_fixture import FixtureError, load_manifest


class Port008Error(ValueError):
    pass


def require(source: str, expression: str, subject: str) -> None:
    if expression not in source:
        raise Port008Error(f"{subject} is missing {expression!r}")


def ordered(source: str, expressions: tuple[str, ...], subject: str) -> None:
    cursor = 0
    for expression in expressions:
        found = source.find(expression, cursor)
        if found < 0:
            raise Port008Error(f"{subject} lacks ordered expression {expression!r}")
        cursor = found + len(expression)


def validate_source(root: Path) -> None:
    serial = (root / "src/serial.asm").read_text(encoding="utf-8")
    vectors = (root / "src_startup/vectors16.asm").read_text(encoding="utf-8")
    core = (root / "src/emos.c").read_text(encoding="utf-8")
    normal_profile = (root / "port/mos-agondev.mk").read_text(encoding="utf-8")
    prototype_profile = (root / "port/port008-forward.mk").read_text(encoding="utf-8")

    if "EMOS_PORT008_FORWARD" in normal_profile:
        raise Port008Error("ordinary EMOS profile enables PORT-008")
    require(prototype_profile, "CPPFLAGS_EXTRA := -DEMOS_PORT008_FORWARD=1", "prototype profile")
    for expression in (
        "PORT008_READY_BIT\tEQU\t10h",
        "PORT008_CLOCK_BIT\tEQU\t20h",
        "PORT008_VALID_BIT\tEQU\t80h",
        "PORT008_OUTPUT_DDR\tEQU\t5Fh",
        "CP\t10h",
        "_port008_general_poll:\tDB\t23, 0, 80h, 1",
    ):
        require(serial, expression, "serial adapter")

    for register in ("pc_dr", "pc_ddr", "pc_alt1", "pc_alt2", "pd_dr", "pd_ddr", "pd_alt1", "pd_alt2"):
        require(serial, f"_port008_saved_{register}", "GPIO snapshot")

    ordered(
        serial[serial.index("PORT008_wait_ready:") : serial.index("PORT008_admitted:")],
        ("(_port008_idle_high)", "OUT0\t(PD_DR), A", "(_port008_idle_low)", "OUT0\t(PD_DR), A", "(PD_DR)", "PORT008_READY_BIT"),
        "READY admission loop",
    )
    ordered(
        serial[serial.index("PORT008_byte_loop:") : serial.index("PORT008_wait_release:")],
        ("OUT0\t(PC_DR), A", "(_port008_active_high)", "OUT0\t(PD_DR), A", "(_port008_active_low)", "OUT0\t(PD_DR), A"),
        "falling-edge byte loop",
    )
    require(vectors, "_rst_18_handler_0:\tCALL\tEMOS_vdu_WRITE", "RST 18 block path")
    require(vectors, "CALL\tEMOS_vdu_PUTCH", "RST 18 delimiter path")
    require(core, "if (mode != EMOS_MODE_EXCLUSIVE_EXTENDED) return EMOS_UNAVAILABLE;", "mode gate")
    require(core, "return EMOS_ADAPTER_PORT008_FORWARD;", "profile-selected adapter")
    if "UART1_serial_TX" in serial[serial.index("PORT008_send:") : serial.index("UART_serial_NE:")]:
        raise Port008Error("forward sender contains a reverse UART call")


def linked_symbols(nm: Path, elf: Path) -> dict[str, int]:
    output = subprocess.run(
        [nm, "-an", elf], check=True, text=True, stdout=subprocess.PIPE
    ).stdout
    result: dict[str, int] = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 3:
            try:
                result[fields[2]] = int(fields[0], 16)
            except ValueError:
                pass
    return result


def disassembly_block(disassembly: str, symbol: str) -> str:
    lines = disassembly.splitlines()
    marker = f"<{symbol}>:"
    start = next((index for index, line in enumerate(lines) if marker in line), None)
    if start is None:
        raise Port008Error(f"linked EMOS image lacks disassembly for {symbol}")
    body: list[str] = []
    for line in lines[start + 1 :]:
        if re.match(r"^\s*[0-9a-fA-F]+ <[^>]+>:$", line):
            break
        body.append(line)
    if not body:
        raise Port008Error(f"linked EMOS disassembly for {symbol} is empty")
    return "\n".join(body)


def validate_sender_disassembly(disassembly: str) -> None:
    admission = disassembly_block(disassembly, "PORT008_wait_ready")
    ordered(
        admission,
        (
            "out0 (0xa2),a",
            "out0 (0xa2),a",
            "in0 a,(0xa2)",
            "and a,0x10",
        ),
        "linked READY-admission loop",
    )

    byte_loop = disassembly_block(disassembly, "PORT008_byte_loop")
    ordered(
        byte_loop,
        (
            "ld a,(hl)",
            "out0 (0x9e),a",
            "out0 (0xa2),a",
            "out0 (0xa2),a",
            "inc hl",
            "dec bc",
        ),
        "linked falling-edge byte loop",
    )
    per_byte = byte_loop[: byte_loop.index("inc hl")]
    if per_byte.count("out0 (0x9e),a") != 1:
        raise Port008Error("linked byte loop must write Port C exactly once")
    if per_byte.count("out0 (0xa2),a") != 2:
        raise Port008Error("linked byte loop must make exactly two Port D writes")

    release = disassembly_block(disassembly, "PORT008_wait_release")
    ordered(
        release,
        (
            "out0 (0xa2),a",
            "out0 (0xa2),a",
            "in0 a,(0xa2)",
            "and a,0x10",
        ),
        "linked READY-release loop",
    )

    sender = disassembly_block(disassembly, "PORT008_send")
    ordered(
        sender,
        ("ld a,b", "cp a,0x10", "ld a,c"),
        "linked physical-record bound",
    )


def validate_linked_sender(
    binary: Path, elf: Path, nm: Path, objdump: Path
) -> None:
    image = binary.read_bytes()
    symbols = linked_symbols(nm, elf)
    required = (
        "_emos_port008_prepare",
        "_emos_port008_recover",
        "PORT008_send",
        "PORT008_wait_ready",
        "PORT008_byte_loop",
        "PORT008_wait_release",
        "_port008_general_poll",
    )
    missing = [name for name in required if name not in symbols]
    if missing:
        raise Port008Error("linked EMOS image lacks symbols: " + ", ".join(missing))
    poll = symbols["_port008_general_poll"]
    if poll + 4 > len(image) or image[poll : poll + 4] != bytes((23, 0, 0x80, 1)):
        raise Port008Error("linked EMOS General Poll bytes differ from 23,0,0x80,1")
    disassembly = subprocess.run(
        [objdump, "-d", elf], check=True, text=True, stdout=subprocess.PIPE
    ).stdout
    validate_sender_disassembly(disassembly)


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
    parser.add_argument("--emos-binary", type=Path)
    parser.add_argument("--emos-elf", type=Path)
    parser.add_argument("--emos-nm", type=Path)
    parser.add_argument("--emos-objdump", type=Path)
    args = parser.parse_args()
    try:
        validate_source(args.source)
        load_manifest(args.manifest)
        artifacts = (args.binary, args.elf, args.nm)
        if any(artifacts) and not all(artifacts):
            raise Port008Error("binary, ELF, and nm must be supplied together")
        if all(artifacts):
            validate_binary(args.manifest, args.binary, args.elf, args.nm)
        sender_artifacts = (
            args.emos_binary,
            args.emos_elf,
            args.emos_nm,
            args.emos_objdump,
        )
        if any(sender_artifacts) and not all(sender_artifacts):
            raise Port008Error(
                "EMOS binary, ELF, nm, and objdump must be supplied together"
            )
        if all(sender_artifacts):
            validate_linked_sender(
                args.emos_binary,
                args.emos_elf,
                args.emos_nm,
                args.emos_objdump,
            )
    except (FixtureError, Port008Error, OSError, subprocess.CalledProcessError) as exc:
        print(f"PORT-008 verification failed: {exc}")
        return 2
    result = "PORT-008 source and 106-byte ordinary-VDU fixture verified"
    if all(sender_artifacts):
        result += "; linked GPIO sender order and General Poll verified"
    print(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
