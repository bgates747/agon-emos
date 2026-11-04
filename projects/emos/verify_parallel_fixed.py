#!/usr/bin/env python3
"""Verify structural linkage of the non-release fixed-backend composition.

Pinned direct-call counts are compiler/linker reachability fingerprints. They
detect composition drift but do not prove which runtime branch executes.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys

from verify_parallel import (
    ParallelError,
    direct_transfer_count,
    disassemble_global_extent,
    instructions,
    linked_symbols,
    require_data_symbols,
    require_global_text_symbols,
    require_text_symbols,
    verify_linked,
    verify_source,
)


class FixedParallelError(RuntimeError):
    pass


def uncommented_c(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//.*$", "", source, flags=re.MULTILINE)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--nm", required=True, type=Path)
    parser.add_argument("--objdump", required=True, type=Path)
    args = parser.parse_args()
    source = args.source.resolve()
    elf = args.elf.resolve()
    nm = args.nm.resolve()
    objdump = args.objdump.resolve()
    verify_source(source)
    verify_linked(elf, nm, objdump, allow_fixed_qualification=True)
    fixed = uncommented_c(
        (source / "src/emos_parallel_fixed_backend.c").read_text(
            encoding="utf-8"
        )
    )
    core = uncommented_c(
        (source / "src/emos.c").read_text(encoding="utf-8")
    )
    for required in (
        "BYTE emos_parallel_fixed_enter(BYTE qualificationAssertion)",
        "&qualificationAssertion",
        "emos_parallel_route_enter(",
        "BYTE emos_parallel_fixed_ready(void)",
        "emos_parallel_route_owned();",
        "emos_parallel_route_leave();",
    ):
        if required not in fixed:
            raise FixedParallelError(f"fixed backend lacks {required!r}")
    if re.search(r"peer_ready\([^)]*\)\s*\{[^}]*return\s+1", fixed, re.DOTALL):
        raise FixedParallelError("peer readiness is an unconditional fake success")
    for forbidden in (
        "emos_parallel_write_stream",
        "emos_parallel_write_byte",
        "fixed_write",
    ):
        if forbidden in fixed:
            raise FixedParallelError(
                f"fixed backend source directly references {forbidden}"
            )
    for required in (
        "EMOS_PARALLEL_FIXED_QUALIFICATION",
        "return EMOS_ADAPTER_PARALLEL_FIXED;",
        "emos_parallel_fixed_enter(TRUE)",
        "emos_parallel_fixed_ready()",
        "emos_parallel_fixed_leave()",
        "static int emos_adapter_public_result(int result)",
        "return result >= FR_OK && result <= EMOS_REGISTRY_FULL ?",
        "if (recoveryResult != FR_OK)",
        "return emos_adapter_public_result(recoveryResult);",
        'return "parallel-fixed-qualification";',
    ):
        if required not in core:
            raise FixedParallelError(
                f"mode coordinator lacks fixed composition binding {required!r}"
            )
    if re.search(r"(?m)^\s*emos_adapter_recover\(\);", core):
        raise FixedParallelError("mode coordinator discards a recovery result")

    required_symbols = (
        "_emos_parallel_fixed_enter",
        "_emos_parallel_fixed_ready",
        "_emos_parallel_fixed_leave",
    )
    symbols = linked_symbols(nm, elf)
    addresses = require_global_text_symbols(
        symbols,
        required_symbols
        + (
            "_emos_parallel_route_enter",
            "_emos_parallel_route_owned",
            "_emos_parallel_route_leave",
        ),
    )
    addresses.update(
        require_text_symbols(
            symbols,
            (
                "_emos_init",
                "_emos_request_mode",
                "_emos_adapter_ready",
                "_emos_adapter_recover",
            ),
        )
    )
    addresses.update(require_data_symbols(symbols, ("_emosEduState",)))
    enter_body = disassemble_global_extent(
        objdump, elf, "_emos_parallel_fixed_enter"
    )
    leave_body = disassemble_global_extent(
        objdump, elf, "_emos_parallel_fixed_leave"
    )
    ready_body = disassemble_global_extent(
        objdump, elf, "_emos_parallel_fixed_ready"
    )

    def requires_direct_target(body: str, symbol: str) -> None:
        address = addresses[symbol]
        count = direct_transfer_count(body, address)
        if count != 1:
            raise FixedParallelError(
                f"linked fixed entry has {count} direct transfers to {symbol}, "
                "expected one"
            )

    requires_direct_target(enter_body, "_emos_parallel_route_enter")
    requires_direct_target(ready_body, "_emos_parallel_route_owned")
    requires_direct_target(leave_body, "_emos_parallel_route_leave")

    coordinator_bodies = {
        name: disassemble_global_extent(objdump, elf, name)
        for name in (
            "_emos_init",
            "_emos_request_mode",
            "_emos_adapter_ready",
            "_emos_adapter_recover",
        )
    }

    # These counts pin the reviewed compiler/linker shape.  Exact coordinator
    # branch outcomes are exercised separately by the source-extraction host
    # transaction test; an address/count check cannot establish path semantics.
    def require_coordinator_target(
        caller: str, callee: str, expected: int
    ) -> None:
        actual = direct_transfer_count(
            coordinator_bodies[caller], addresses[callee]
        )
        if actual != expected:
            raise FixedParallelError(
                f"linked {caller} has {actual} direct transfers to {callee}, "
                f"expected {expected}"
            )

    require_coordinator_target(
        "_emos_request_mode", "_emos_parallel_fixed_enter", 1
    )
    require_coordinator_target("_emos_request_mode", "_emos_adapter_ready", 2)
    require_coordinator_target("_emos_request_mode", "_emos_adapter_recover", 3)
    require_coordinator_target(
        "_emos_request_mode", "_emos_parallel_fixed_ready", 1
    )
    require_coordinator_target(
        "_emos_adapter_ready", "_emos_parallel_fixed_ready", 1
    )
    require_coordinator_target(
        "_emos_adapter_recover", "_emos_parallel_fixed_leave", 1
    )
    require_coordinator_target(
        "_emos_adapter_recover", "_emos_parallel_fixed_ready", 1
    )

    init_rows = [
        (mnemonic, operands)
        for _, mnemonic, operands in instructions(coordinator_bodies["_emos_init"])
    ]
    fixed_selection = (
        ("ld", "a,0x02"),
        ("ld", f"(0x{addresses['_emosEduState']:x}),a"),
    )
    if not any(
        tuple(init_rows[index : index + 2]) == fixed_selection
        for index in range(len(init_rows) - 1)
    ):
        raise FixedParallelError(
            "linked EMOS initialization does not select the fixed adapter"
        )
    for forbidden in (
        "_emos_parallel_write_stream",
        "_emos_parallel_write_byte",
    ):
        if any(
            direct_transfer_count(body, address)
            for address, _ in symbols.get(forbidden, [])
            for body in (enter_body, ready_body, leave_body)
        ):
            raise FixedParallelError(
                f"linked fixed entry directly references {forbidden}"
            )
    image = elf.read_bytes()
    if b"EMOS qualification composition: %s (non-release)\r\n\0" not in image:
        raise FixedParallelError(
            "linked image lacks the explicit non-release composition diagnostic"
        )
    composition_identities = set(
        re.findall(
            rb"(?:UNVERSIONED-PORT008-FORWARD-QUALIFICATION-DO-NOT-DEPLOY|"
            rb"port-008-forward-qualification-r[0-9]+)\0",
            image,
        )
    )
    if len(composition_identities) != 1:
        raise FixedParallelError(
            "linked image lacks one unambiguous qualification-composition identity"
        )
    print(
        "EMOS non-release fixed-composition linked-image checks passed: "
        "the fixed-adapter initialization store and pinned structural coordinator "
        "call-edge counts match; common route calls and the separately identified "
        "nonrelease composition diagnostic "
        "are present; ordinary VDU routing reaches the common production route; "
        "and the adapter has no direct production-write reference; coordinator "
        "branch/path execution, object origin, runtime peer preparation, target "
        "execution, release identity, and electrical behavior are not claimed"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (
        FixedParallelError,
        ParallelError,
        OSError,
        subprocess.CalledProcessError,
    ) as error:
        print(f"EMOS fixed-backend verification failed: {error}", file=sys.stderr)
        raise SystemExit(1) from None
