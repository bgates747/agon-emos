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


def verify(image: bytes, linked: dict[str, int]) -> None:
    required = (
        "EMOS_vdu_PUTCH",
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

    dispatcher = image[linked["EMOS_vdu_PUTCH"] : linked["UART_serial_NE"]]
    expected = (
        b"\xf5\x3a"
        + linked["_emosVduBackend"].to_bytes(3, "little")
        + b"\xb7\x20\x05\xf1\xc3"
        + linked["UART0_serial_PUTCH"].to_bytes(3, "little")
        + b"\xf1\xb7\xc9"
    )
    if dispatcher != expected:
        raise VduError("dispatcher is not the fixed snapshot/fail-closed sequence")

    call = _call(linked["EMOS_vdu_PUTCH"])
    ranges = {
        "C putch": (linked["_putch"], linked["_getch"], 1),
        "RST 10": (linked["_rst_10_handler"], linked["_rst_18_handler"], 1),
        "RST 18": (linked["_rst_18_handler"], linked["__rst_38_handler"], 2),
    }
    for name, (start, end, count) in ranges.items():
        actual = image[start:end].count(call)
        if actual != count:
            raise VduError(f"{name} has {actual} dispatcher calls, expected {count}")


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
    print("EMOS VDU verified: RST 10, RST 18, and C putch use one fixed dispatcher")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
