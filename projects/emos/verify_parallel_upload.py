#!/usr/bin/env python3
"""Private upload extension: preserve native checks and bind new sole callers."""
import argparse
from pathlib import Path
import subprocess
from verify_parallel import (ParallelError, verify_source, verify_linked, linked_symbols, require_global_text_symbols,
                             direct_transfer_count, disassemble_global_extent, instructions)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "elf", "nm", "objdump"):
        parser.add_argument("--"+name, type=Path, required=True)
    args = parser.parse_args()
    verify_source(args.source)
    verify_linked(args.elf,args.nm,args.objdump,allow_upload_payload=True)
    symbols = linked_symbols(args.nm, args.elf)
    if any(name in symbols for name in ("_emos_parallel_native_bytes", "_emos_parallel_native_coordinate")):
        raise ParallelError("upload profile retained unused bounded-block payload code")
    addresses = require_global_text_symbols(symbols, (
        "_emos_parallel_upload_begin", "_emos_parallel_upload_read",
        "_emos_parallel_upload_end", "_emos_parallel_upload_bytes",
        "_emos_parallel_upload_abandon", "_emos_sdlink_gateway"))
    whole = subprocess.check_output([str(args.objdump), "-d", str(args.elf)], text=True).lower()
    for caller, callee in (
        ("_emos_sdlink_gateway", "_emos_parallel_upload_begin"),
        ("_emos_sdlink_gateway", "_emos_parallel_upload_read"),
        ("_emos_sdlink_gateway", "_emos_parallel_upload_end"),
        ("_emos_parallel_upload_read", "_emos_parallel_upload_bytes"),
    ):
        body = disassemble_global_extent(args.objdump, args.elf, caller)
        if (direct_transfer_count(whole, addresses[callee]) != 1 or
                direct_transfer_count(body, addresses[callee]) != 1):
            raise ParallelError("upload sole caller changed: "+callee)
    raw = disassemble_global_extent(args.objdump, args.elf, "_emos_parallel_upload_bytes")
    ops = [row[1:] for row in instructions(raw)]
    if (ops.count(("in0", "a,(0x9e)")) != 2 or
            ops.count(("out0", "(0xa2),a")) != 4 or
            any(op == "out0" and "0xa2" not in operand for op, operand in ops)):
        raise ParallelError("upload leaf changed its input-only Port C/clock writer inventory")
    if ops.count(("di", "")) != 1 or ops.count(("ei", "")) != 1:
        raise ParallelError("upload leaf lost specialized entry-IFF paths")
    for name in ("src/emos.h", "src/mos_api.asm"):
        if "emos_parallel_upload" in (args.source / name).read_text():
            raise ParallelError("private upload leaf escaped to public MOS ABI")
    if len(args.elf.with_suffix(".bin").read_bytes()) > 131072:
        raise ParallelError("upload candidate exceeds physical ROM")
    print("EMOS upload sole gateway/leaf, common ABI/pad/boot checks and ROM bound verified")


if __name__ == "__main__":
    main()
