#!/usr/bin/env python3
"""Private unbound native payload composition; preserves all ordinary checks.

PORT-008 ROM fit exposed the new assembly owner to the mandatory link inventory.
Admit only this profile's exact Port C writers, sole guarded raw caller and
atomic Port D helper. This checker does not authorize live activation or claim
electrical timing, and its no-live-caller gate must be reviewed before binding.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
from verify_parallel import (
    ParallelError, direct_transfer_count, disassemble_global_extent,
    instructions, linked_symbols, require_global_text_symbols, verify_iff_helper,
    verify_linked, verify_source,
)

def verify_native(source, elf, nm, objdump):
    verify_source(source)
    verify_linked(elf, nm, objdump, allow_native_payload=True)
    symbols = linked_symbols(nm, elf)
    addresses = require_global_text_symbols(symbols, (
        "_emos_parallel_native_payload", "_emos_parallel_native_bytes",
        "_emos_parallel_native_coordinate"))
    if "_parallel_boot_grant" not in symbols or "_emos_parallel_engine_read" in symbols:
        raise ParallelError("native composition lost private startup/reference exclusion")
    whole = subprocess.check_output([str(objdump), "-d", str(elf)], text=True).lower()
    guard = disassemble_global_extent(objdump, elf, "_emos_parallel_native_payload")
    raw = disassemble_global_extent(objdump, elf, "_emos_parallel_native_bytes")
    coordinator = disassemble_global_extent(objdump, elf, "_emos_parallel_native_coordinate")
    if direct_transfer_count(whole, addresses["_emos_parallel_native_coordinate"]) != 0:
        raise ParallelError("private coordinator acquired an unreviewed activation caller")
    native_address = addresses["_emos_parallel_native_payload"]
    if direct_transfer_count(whole, native_address) != 1 or direct_transfer_count(coordinator, native_address) != 1:
        raise ParallelError("native guard is not called solely by the coordinator")
    raw_address = addresses["_emos_parallel_native_bytes"]
    if direct_transfer_count(whole, raw_address) != 1 or direct_transfer_count(guard, raw_address) != 1:
        raise ParallelError("native raw leaf is not called solely by its guard")
    control = symbols.get("native_control", [])
    if len(control) != 1 or control[0][1] != "t":
        raise ParallelError("native Port D helper must remain local text")
    if direct_transfer_count(whole, control[0][0]) != 7 or direct_transfer_count(raw, control[0][0]) != 7:
        raise ParallelError("native Port D helper caller inventory changed")
    helper = disassemble_global_extent(objdump, elf, "native_control")
    verify_iff_helper("native_control", helper, 6)
    helper_ops = [row[1:] for row in instructions(helper)]
    expected = [("ld", "a,i"), ("push", "af"), ("di", ""),
                ("in0", "a,(0xa2)"), ("and", "a,0x4f"), ("or", "a,c"),
                ("out0", "(0xa2),a"), ("pop", "af")]
    if helper_ops[:8] != expected or len(helper_ops) != 11 or helper_ops[-1] != ("ret", ""):
        raise ParallelError("native Port D masked RMW instruction sequence changed")
    raw_ops = [row[1:] for row in instructions(raw)]
    if raw_ops.count(("out0", "(0xa2),a")) != 1 or raw_ops.count(("in0", "a,(0x9e)")) != 1:
        raise ParallelError("native control-write/data-read inventory changed")
    body = (source / "src/emos_parallel_native.c").read_text()
    # This source fingerprint supplements, rather than replaces, execution of
    # the compiled guard against modeled registers in parallel_native.rs.
    for required in ("length > 4096", "reverse > 1", "parked != 1",
                     "h->phase != EPH_BLOCK", "h->failed", "UART1_IER",
                     "UART1_MCTL != UART_MCTL_LOOP", "PC_DDR != 0xFF",
                     "PC_ALT1", "PC_ALT2", "(PD_DDR & 0xB0) != 0x10",
                     "(PD_ALT1 & 0xB0)", "(PD_ALT2 & 0xB0)",
                     "(PD_DR & 0xB0) != 0xA0", "h->failed = 1"):
        if required not in body:
            raise ParallelError("native guard lost required condition: " + required)
    for name in ("src/emos.h", "src/mos_api.asm"):
        if "emos_parallel_native" in (source / name).read_text():
            raise ParallelError("native leaf escaped to a public API")

def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "elf", "nm", "objdump"):
        p.add_argument("--" + name, type=Path, required=True)
    a = p.parse_args()
    verify_native(a.source.resolve(), a.elf.resolve(), a.nm.resolve(), a.objdump.resolve())
    print("EMOS private native linked checks passed: ordinary checks retained, "
          "exact Port C owners, guarded raw caller, private unactivated coordinator, atomic "
          "Port D helper; no electrical/performance claim")
if __name__ == "__main__":
    try:
        main()
    except (ParallelError, OSError, subprocess.CalledProcessError) as e:
        print("EMOS native verification failed: " + str(e), file=sys.stderr)
        raise SystemExit(1)
