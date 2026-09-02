#!/usr/bin/env python3
"""Verify the linked resident EMOS gateway tables and wrapper ABI."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


class AbiError(ValueError):
    pass


def symbols(nm: Path, elf: Path) -> dict[str, int]:
    result = subprocess.run(
        [nm, "-an", elf], check=True, text=True, stdout=subprocess.PIPE
    )
    found: dict[str, int] = {}
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) == 3:
            try:
                found[fields[2]] = int(fields[0], 16)
            except ValueError:
                pass
    return found


def verify(image: bytes, linked: dict[str, int]) -> None:
    required = (
        "_emos_gateway",
        "mos_api_emos_gateway",
        "mos_api_block1_start",
        "mos_function_block_start",
        "mos_function_block_size",
        "_open_UART1",
    )
    missing = [name for name in required if name not in linked]
    if missing:
        raise AbiError("linked image lacks symbols: " + ", ".join(missing))
    if linked["mos_function_block_size"] != 0x21:
        raise AbiError("mos_getfunction table is not exactly 0x21 entries")

    api_slot = linked["mos_api_block1_start"] + 0x51 * 2
    api_target = int.from_bytes(image[api_slot : api_slot + 2], "little")
    if api_target != linked["mos_api_emos_gateway"]:
        raise AbiError("MOS API 0x51 does not target the EMOS wrapper")

    table = linked["mos_function_block_start"]
    uart_target = int.from_bytes(image[table + 0x08 * 3 : table + 0x09 * 3], "little")
    if uart_target != linked["_open_UART1"]:
        raise AbiError("mos_getfunction slot 0x08 does not target guarded UART1 open")
    reserved = image[table + 0x12 * 3 : table + 0x20 * 3]
    if any(reserved):
        raise AbiError("mos_getfunction slots 0x12 through 0x1f are not reserved")
    c_target = int.from_bytes(image[table + 0x20 * 3 : table + 0x21 * 3], "little")
    if c_target != linked["_emos_gateway"]:
        raise AbiError("mos_getfunction slot 0x20 is not the resident Core gateway")

    wrapper = image[
        linked["mos_api_emos_gateway"] : linked["mos_api_emos_gateway"] + 15
    ]
    expected_fixed = {
        0: 0xED,
        1: 0x6E,  # LD A,MB: reject non-ADL callers
        2: 0xB7,  # OR A,A
        3: 0xC2,  # JP NZ,not-implemented
        7: 0xE5,  # preserve caller HLU
        8: 0xCD,  # call resident C gateway
        12: 0x7D,  # status low byte to A
        13: 0xE1,
        14: 0xC9,
    }
    for offset, byte in expected_fixed.items():
        if wrapper[offset] != byte:
            raise AbiError(f"unexpected API wrapper byte at +0x{offset:x}")
    call_target = int.from_bytes(wrapper[9:12], "little")
    if call_target != linked["_emos_gateway"]:
        raise AbiError("MOS API wrapper does not call the resident Core gateway")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--nm", type=Path, required=True)
    args = parser.parse_args()
    try:
        verify(args.binary.read_bytes(), symbols(args.nm, args.elf))
    except (AbiError, OSError, subprocess.CalledProcessError) as exc:
        print(f"EMOS ABI verification failed: {exc}", file=sys.stderr)
        return 2
    print(
        "EMOS ABI verified: MOS API 0x51, guarded UART1 slot 0x08, and resident "
        "Core slot 0x20 are fixed"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
