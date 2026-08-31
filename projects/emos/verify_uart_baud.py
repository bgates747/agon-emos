#!/usr/bin/env python3
"""Verify EMOS preserves 32-bit UART baud arithmetic through the linked image.

Official ZDS-built MOS programs the intended divisor, but the inherited source
does not express the required intermediate width. Under AgonDev, ``UINT24`` is
``unsigned int`` and both operands therefore multiply at the eZ80 native
24-bit width before assignment to ``UINT32``. The resulting wrap is standard
unsigned arithmetic, not presently classified as a compiler defect. Physical
PORT-008 ZDI evidence found UART0 BRG divisor 11 instead of 1. This verifier
binds the source remedy to the actual helper calls and UART register writes in
the linked EMOS image; it does not infer physical baud or signal quality.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys


class UartBaudError(RuntimeError):
    pass


WIDENED_PRODUCT = (
    "(UINT32)CLOCK_DIVISOR_16 * (UINT32)pUART->baudRate"
)


def symbols(output: str) -> dict[str, int]:
    found: dict[str, int] = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 3 and re.fullmatch(r"[0-9A-Fa-f]+", fields[0]):
            found[fields[-1]] = int(fields[0], 16)
    return found


def require_call(disassembly: str, address: int, function: str) -> None:
    pattern = rf"\bcall\s+0x0*{address:x}\b"
    if re.search(pattern, disassembly, flags=re.IGNORECASE) is None:
        raise UartBaudError(
            f"{function} does not call required 32-bit helper at 0x{address:06X}"
        )


def verify_source(source_root: Path) -> tuple[int, int, int]:
    uart = (source_root / "src/uart.c").read_text(encoding="utf-8")
    if uart.count(WIDENED_PRODUCT) != 2:
        raise UartBaudError(
            "UART0 and UART1 must each widen both baud-product operands"
        )

    header = (source_root / "src/uart.h").read_text(encoding="utf-8")
    main = (source_root / "main.c").read_text(encoding="utf-8")
    clock_match = re.search(r"^#define\s+MASTERCLOCK\s+(\d+)", header, re.MULTILINE)
    divisor_match = re.search(
        r"^#define\s+CLOCK_DIVISOR_16\s+(\d+)", header, re.MULTILINE
    )
    baud_match = re.search(
        r"wait_ESP32\s*\(\s*&pUART0\s*,\s*(\d+)\s*\)", main
    )
    if not all((clock_match, divisor_match, baud_match)):
        raise UartBaudError("cannot derive UART0 clock, oversample, and baud inputs")
    assert clock_match is not None and divisor_match is not None and baud_match is not None
    clock = int(clock_match.group(1))
    oversample = int(divisor_match.group(1))
    baud = int(baud_match.group(1))
    expected = clock // (oversample * baud)
    wrapped_product = (oversample * baud) & 0xFFFFFF
    wrapped_result = clock // wrapped_product
    if (clock, oversample, baud, expected, wrapped_result) != (
        18_432_000,
        16,
        1_152_000,
        1,
        11,
    ):
        raise UartBaudError(
            "review UART constants or regression oracle before accepting changed values"
        )
    return clock, baud, expected


def verify_linked(elf: Path, nm: Path, objdump: Path) -> None:
    nm_output = subprocess.run(
        [str(nm), "-n", str(elf)],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    ).stdout
    linked = symbols(nm_output)
    required = ("_open_UART0", "_open_UART1", "__lshl", "__ldivu")
    missing = [name for name in required if name not in linked]
    if missing:
        raise UartBaudError(f"missing linked symbols: {', '.join(missing)}")

    for function, low_port, high_port in (
        ("_open_UART0", 0xC0, 0xC1),
        ("_open_UART1", 0xD0, 0xD1),
    ):
        disassembly = subprocess.run(
            [str(objdump), "-d", f"--disassemble={function}", str(elf)],
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        ).stdout
        require_call(disassembly, linked["__lshl"], function)
        require_call(disassembly, linked["__ldivu"], function)
        if "__ishl" in linked:
            old_call = rf"\bcall\s+0x0*{linked['__ishl']:x}\b"
            if re.search(old_call, disassembly, flags=re.IGNORECASE):
                raise UartBaudError(
                    f"{function} still uses the wrapping 24-bit shift"
                )
        for port in (low_port, high_port):
            if f"out0 (0x{port:02x}),a" not in disassembly.lower():
                raise UartBaudError(
                    f"{function} does not write expected BRG port 0x{port:02X}"
                )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--nm", required=True, type=Path)
    parser.add_argument("--objdump", required=True, type=Path)
    args = parser.parse_args()

    clock, baud, divisor = verify_source(args.source.resolve())
    verify_linked(
        args.elf.resolve(), args.nm.resolve(), args.objdump.resolve()
    )
    print(
        "EMOS UART baud contract: "
        f"clock={clock} baud={baud} divisor={divisor}; "
        "UART0/UART1 use linked 32-bit multiply and divide"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except UartBaudError as error:
        print(f"EMOS UART baud contract failed: {error}", file=sys.stderr)
        raise SystemExit(1) from None
