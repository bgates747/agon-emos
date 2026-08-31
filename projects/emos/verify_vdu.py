#!/usr/bin/env python3
"""Verify every linked ordinary-VDU entry reaches one Core dispatcher."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


class VduError(ValueError):
    pass


def symbols(nm: Path, elf: Path) -> dict[str, int]:
    result = subprocess.run(
        [nm, "-an", elf], check=True, text=True, stdout=subprocess.PIPE
    )
    found: dict[str, int] = {}
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) != 3:
            continue
        try:
            found[fields[2]] = int(fields[0], 16)
        except ValueError:
            pass
    return found


def _call(address: int) -> bytes:
    return b"\xcd" + address.to_bytes(3, "little")


def _jump(address: int) -> bytes:
    return b"\xc3" + address.to_bytes(3, "little")


def _jr(opcode: int, instruction: int, target: int) -> bytes:
    displacement = target - (instruction + 2)
    if not -128 <= displacement <= 127:
        raise VduError("dispatcher relative branch is out of range")
    return bytes((opcode, displacement & 0xFF))


def verify(image: bytes, linked: dict[str, int]) -> None:
    required = (
        "EMOS_vdu_PUTCH",
        "EMOS_vdu_WRITE",
        "EMOS_vdu_port008",
        "EMOS_vdu_onboard",
        "PORT008_vdu_PUTCH",
        "PORT008_send",
        "UART0_serial_PUTCH",
        "UART1_serial_PUTCH",
        "UART_serial_NE",
        "_emosVduBackend",
        "_putch",
        "_getch",
        "_rst_10_handler",
        "_rst_18_handler",
        "__rst_38_handler",
    )
    missing = [name for name in required if name not in linked]
    if missing:
        raise VduError("linked image lacks symbols: " + ", ".join(missing))

    start = linked["EMOS_vdu_PUTCH"]
    dispatcher = image[start : linked["UART_serial_NE"]]
    expected = (
        b"\xf5\x3a"
        + linked["_emosVduBackend"].to_bytes(3, "little")
        + b"\xb7"
        + _jr(0x28, start + 6, linked["EMOS_vdu_onboard"])
        + b"\xfe\x02"
        + _jr(0x28, start + 10, linked["EMOS_vdu_port008"])
        + b"\xf1\xb7\xc9\xf1"
        + _jump(linked["PORT008_vdu_PUTCH"])
        + b"\xf1"
        + _jump(linked["UART0_serial_PUTCH"])
    )
    if dispatcher != expected:
        raise VduError("dispatcher is not the fixed snapshot/route/fail-closed sequence")

    putch_call = _call(linked["EMOS_vdu_PUTCH"])
    write_call = _call(linked["EMOS_vdu_WRITE"])
    ranges = {
        "C putch": (linked["_putch"], linked["_getch"], putch_call, 1),
        "RST 10": (linked["_rst_10_handler"], linked["_rst_18_handler"], putch_call, 1),
        "RST 18 block": (linked["_rst_18_handler"], linked["__rst_38_handler"], write_call, 1),
        "RST 18 delimiter": (linked["_rst_18_handler"], linked["__rst_38_handler"], putch_call, 1),
    }
    for name, (start, end, call, count) in ranges.items():
        actual = image[start:end].count(call)
        if actual != count:
            raise VduError(f"{name} has {actual} dispatcher calls, expected {count}")

    write = image[linked["EMOS_vdu_WRITE"] : linked["PORT008_vdu_PUTCH"]]
    if write.count(_call(linked["UART0_serial_PUTCH"])) != 1:
        raise VduError("block dispatcher does not retain one onboard UART call")
    if write.count(_call(linked["PORT008_send"])) != 1:
        raise VduError("block dispatcher does not map the EDP block to one record")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--nm", type=Path, required=True)
    args = parser.parse_args()
    try:
        verify(args.binary.read_bytes(), symbols(args.nm, args.elf))
    except (VduError, OSError, subprocess.CalledProcessError) as exc:
        print(f"EMOS VDU verification failed: {exc}", file=sys.stderr)
        return 2
    print("EMOS VDU verified: RST 10/C putch use byte dispatch and RST 18 uses bounded block dispatch")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
