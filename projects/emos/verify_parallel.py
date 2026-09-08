#!/usr/bin/env python3
"""Verify the linked production EMOS forward-parallel object boundary."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys


class ParallelError(RuntimeError):
    pass


REQUIRED_SYMBOLS = (
    "EMOS_vdu_PUTCH",
    "EMOS_vdu_WRITE",
    "EMOS_vdu_parallel_PUTCH",
    "EMOS_vdu_parallel_WRITE",
    "UART0_serial_PUTCH",
    "UART1_serial_PUTCH",
    "_putch",
    "_open_UART1",
    "_emos_parallel_engine_configure",
    "_emos_parallel_engine_open",
    "_emos_parallel_engine_close",
    "_emos_parallel_engine_write",
    "_emos_parallel_engine_fault",
    "_emos_parallel_generation_next",
    "_emos_parallel_init",
    "_emos_parallel_epoch_enter",
    "_emos_parallel_epoch_leave",
    "_emos_parallel_write_stream",
    "_emos_parallel_write_byte",
    "_emos_parallel_last_fault",
    "_emos_parallel_epoch_owned",
    "_emos_parallel_route_enter",
    "_emos_parallel_route_leave",
    "_emos_parallel_route_write_stream",
    "_emos_parallel_route_write_byte",
    "_emos_parallel_route_owned",
    "_emos_parallel_uart1_guard_acquire",
    "_emos_parallel_uart1_guard_release",
    "_emos_parallel_io_write_control",
    "_emos_parallel_io_configure",
    "_emos_parallel_io_try_lock",
    "_emos_parallel_io_try_begin_entry",
    "_emos_parallel_io_try_reserve_portc",
    "_emos_parallel_io_read_clock",
    "_emos_parallel_io_commit_epoch",
)

FORBIDDEN_NORMAL_SYMBOLS = (
    "_emos_parallel_fixed_enter",
    "_emos_parallel_fixed_ready",
    "_emos_parallel_fixed_leave",
)

PREDECESSOR_SYMBOL = re.compile(r"^(?:PORT008_|_port008_|_emos_port008_)")


def linked_symbols(nm: Path, elf: Path) -> dict[str, list[tuple[int, str]]]:
    result = subprocess.run(
        [str(nm), "-an", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    symbols: dict[str, list[tuple[int, str]]] = {}
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) == 3 and re.fullmatch(r"[0-9A-Fa-f]+", fields[0]):
            symbols.setdefault(fields[2], []).append(
                (int(fields[0], 16), fields[1])
            )
    return symbols


def require_global_text_symbols(
    symbols: dict[str, list[tuple[int, str]]], names: tuple[str, ...]
) -> dict[str, int]:
    result: dict[str, int] = {}
    for name in names:
        records = symbols.get(name, [])
        if len(records) != 1:
            raise ParallelError(
                f"linked image must contain exactly one {name}, got {len(records)}"
            )
        address, kind = records[0]
        if kind != "T":
            raise ParallelError(
                f"linked symbol {name} is not a defined global text symbol: {kind}"
            )
        result[name] = address
    return result


def require_text_symbols(
    symbols: dict[str, list[tuple[int, str]]], names: tuple[str, ...]
) -> dict[str, int]:
    result: dict[str, int] = {}
    for name in names:
        records = symbols.get(name, [])
        if len(records) != 1:
            raise ParallelError(
                f"linked image must contain exactly one {name}, got {len(records)}"
            )
        address, kind = records[0]
        if kind not in ("T", "t"):
            raise ParallelError(
                f"linked symbol {name} is not defined text: {kind}"
            )
        result[name] = address
    return result


def require_data_symbols(
    symbols: dict[str, list[tuple[int, str]]], names: tuple[str, ...]
) -> dict[str, int]:
    result: dict[str, int] = {}
    for name in names:
        records = symbols.get(name, [])
        if len(records) != 1:
            raise ParallelError(
                f"linked image must contain exactly one {name}, got {len(records)}"
            )
        address, kind = records[0]
        if kind not in ("B", "b"):
            raise ParallelError(f"linked symbol {name} is not BSS data: {kind}")
        result[name] = address
    return result


def disassemble(objdump: Path, elf: Path, symbol: str) -> str:
    return subprocess.run(
        [str(objdump), "-d", f"--disassemble={symbol}", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout.lower()


def disassemble_global_extent(objdump: Path, elf: Path, symbol: str) -> str:
    output = subprocess.run(
        [str(objdump), "-d", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout.lower()
    wanted = symbol.lower()
    selected: list[str] = []
    active = False
    for line in output.splitlines():
        label = re.match(r"^\s*[0-9a-f]+\s+<([^>]+)>:$", line)
        if label:
            name = label.group(1)
            if active and name.startswith("_") and name != wanted:
                break
            if name == wanted:
                active = True
        if active:
            selected.append(line)
    return "\n".join(selected)


def instruction_mnemonics(disassembly: str) -> list[str]:
    result = []
    for line in disassembly.splitlines():
        match = re.match(
            r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+([a-z][a-z0-9.]*)\b",
            line,
        )
        if match:
            result.append(match.group(1))
    return result


def instructions(disassembly: str) -> list[tuple[int, str, str]]:
    result = []
    for line in disassembly.splitlines():
        match = re.match(
            r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+"
            r"([a-z][a-z0-9.]*)\s*(.*)$",
            line,
        )
        if match:
            result.append(
                (int(match.group(1), 16), match.group(2), match.group(3).strip())
            )
    return result


def direct_transfer_count(
    disassembly: str, target: int, mnemonics: tuple[str, ...] = ("call", "jp")
) -> int:
    count = 0
    for _, mnemonic, operands in instructions(disassembly):
        if mnemonic not in mnemonics:
            continue
        addresses = [int(value, 16) for value in re.findall(r"0x([0-9a-f]+)", operands)]
        if target in addresses:
            count += 1
    return count


def address_reference_count(disassembly: str, target: int) -> int:
    return sum(
        target in [
            int(value, 16) for value in re.findall(r"0x([0-9a-f]+)", operands)
        ]
        for _, _, operands in instructions(disassembly)
    )


def direct_transfer_indexes(
    disassembly: str, target: int, mnemonics: tuple[str, ...] = ("call", "jp")
) -> list[int]:
    result = []
    for index, (_, mnemonic, operands) in enumerate(instructions(disassembly)):
        if mnemonic not in mnemonics:
            continue
        addresses = [
            int(value, 16) for value in re.findall(r"0x([0-9a-f]+)", operands)
        ]
        if target in addresses:
            result.append(index)
    return result


def verify_iff_helper(symbol: str, disassembly: str, masked_limit: int) -> None:
    rows = instructions(disassembly)

    def matching(mnemonic: str, operands: str | None = None) -> list[int]:
        return [
            index
            for index, (_, actual_mnemonic, actual_operands) in enumerate(rows)
            if actual_mnemonic == mnemonic
            and (operands is None or actual_operands == operands)
        ]

    ld_i = matching("ld", "a,i")
    push_af = matching("push", "af")
    disable = matching("di")
    pop_af = matching("pop", "af")
    conditional = [
        index
        for index, (_, mnemonic, operands) in enumerate(rows)
        if mnemonic == "jp" and operands.startswith("po,")
    ]
    enable = matching("ei")
    if not all(
        len(group) == 1
        for group in (ld_i, push_af, disable, pop_af, conditional, enable)
    ):
        raise ParallelError(
            f"{symbol} does not contain one exact IFF2 save/DI/restore sequence"
        )
    sequence = [
        ld_i[0], push_af[0], disable[0], pop_af[0], conditional[0], enable[0]
    ]
    if sequence != sorted(sequence):
        raise ParallelError(f"{symbol} IFF2 operations are out of order")
    if push_af[0] != ld_i[0] + 1 or conditional[0] != pop_af[0] + 1:
        raise ParallelError(f"{symbol} does not preserve IFF2 flags directly")
    if enable[0] != conditional[0] + 1 or enable[0] + 1 >= len(rows):
        raise ParallelError(f"{symbol} EI is not the conditional fall-through")
    target_match = re.fullmatch(r"po,0x([0-9a-f]+)", rows[conditional[0]][2])
    if target_match is None or int(target_match.group(1), 16) != rows[enable[0] + 1][0]:
        raise ParallelError(f"{symbol} conditional branch does not skip exactly EI")
    masked_instructions = pop_af[0] - disable[0] + 1
    if masked_instructions > masked_limit:
        raise ParallelError(
            f"{symbol} masked section grew to {masked_instructions} instructions "
            f"(limit {masked_limit})"
        )


def uncommented_c(source: str) -> str:
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    return re.sub(r"//.*$", "", source, flags=re.MULTILINE)


def verify_source(source: Path) -> None:
    header = (source / "src/emos_parallel.h").read_text(encoding="utf-8")
    engine = (source / "src/emos_parallel_engine.c").read_text(encoding="utf-8")
    binding = (source / "src/emos_parallel.c").read_text(encoding="utf-8")
    io = (source / "src/emos_parallel_io.asm").read_text(encoding="utf-8")
    serial = (source / "src/serial.asm").read_text(encoding="utf-8")
    uart = (source / "src/uart.c").read_text(encoding="utf-8")
    core = (source / "src/emos.c").read_text(encoding="utf-8")
    public_header = (source / "src/emos.h").read_text(encoding="utf-8")
    mos_api = (source / "src/mos_api.asm").read_text(encoding="utf-8")
    if "EMOS_PARALLEL_DATA_PLANE" in uart:
        raise ParallelError("UART1 parallel exclusion is profile-optional")
    if "((UINT16)4096)" not in header or "((UINT24)0xFFFFFF)" not in header:
        raise ParallelError("record limit or finite poll ceiling lost exact width")
    for status in ("0xE3", "0xE4", "0xE5", "0xE6", "0xE8"):
        if status not in header:
            raise ParallelError(f"missing lifecycle status {status}")
    # INTEG-007 admits one explicit Legacy-only UART diagnostic in Core.
    # Keep the predecessor-poll ban everywhere else, including every parallel
    # source and any additional call from Core. Do not rename the diagnostic
    # just to evade this ownership guard.
    poll_branch = re.search(
        r'if \(strcasecmp\(operation, "vdppoll"\) == 0\) \{(.*?)'
        r'(?=if \(strcasecmp\(operation, "uartflow"\))', core, re.DOTALL)
    checked_core = core
    if poll_branch:
        branch = poll_branch.group(0)
        call = "return emos_general_poll() ? FR_OK : FR_TIMEOUT;"
        if branch.count(call) != 1 or "emosModeState.mode != EMOS_MODE_LEGACY" not in branch:
            raise ParallelError("General Poll diagnostic lost its Core/Legacy guard")
        checked_core = core[:poll_branch.start()] + branch.replace(call, "") + core[poll_branch.end():]
    for forbidden in (
        "EMOS_PORT008_FORWARD",
        "general_poll",
        "PORT008_",
        "_port008_",
        "_emos_port008_",
        "UNIT_TEST",
    ):
        if forbidden in engine + binding + serial + checked_core:
            raise ParallelError(f"production data plane contains {forbidden}")
    if "volatile BYTE busy" not in header or "volatile BYTE firstFault" not in header:
        raise ParallelError("interrupt-visible engine state is not volatile")
    if "snapshot/restore policy" not in binding:
        raise ParallelError("deterministic release is not distinguished from prototype snapshots")
    if "emosParallelWriterLockStatus" in binding:
        raise ParallelError("writer-lock result is shared rather than per-call")
    if "BYTE lockStatus = EMOS_PARALLEL_BUSY" not in binding:
        raise ParallelError("writer-lock result is not stack-local")
    if "emosParallelDeferredFault" not in binding:
        raise ParallelError("rejected interrupting writers are not sticky")
    for required in (
        "static t_emosParallelEpochLease emosParallelRouteLease;",
        "BYTE emos_parallel_route_enter(",
        "BYTE emos_parallel_route_write_stream(",
        "BYTE emos_parallel_uart1_guard_acquire(void)",
        "emos_parallel_io_try_reserve_portc(",
    ):
        if required not in binding:
            raise ParallelError(f"production binding lacks {required!r}")
    for private_entry in (
        "emos_parallel_epoch_",
        "emos_parallel_route_",
        "emos_parallel_write_",
        "EMOS_vdu_parallel_",
    ):
        if private_entry in public_header or private_entry in mos_api:
            raise ParallelError(
                "core-private parallel entry is exposed by a public surface: "
                f"{private_entry}"
            )
    uart_code = uncommented_c(uart)
    open_uart1 = uart_code.split("BYTE open_UART1", 1)[1].split(
        "void close_UART1", 1
    )[0]
    if open_uart1.count("emos_parallel_uart1_guard_acquire()") != 1:
        raise ParallelError("UART1 open does not contain one real guard acquire")
    if open_uart1.count("emos_parallel_uart1_guard_release()") != 1:
        raise ParallelError("UART1 open does not contain one real guard release")
    acquire = open_uart1.find("emos_parallel_uart1_guard_acquire()")
    release = open_uart1.find("emos_parallel_uart1_guard_release()")
    if acquire < 0 or release < 0 or acquire >= release:
        raise ParallelError("UART1 open does not hold the parallel Port C guard")
    mutation_patterns = (
        r"\bserialFlags\s*[|&^]?=",
        r"\b(?:SETREG|RESETREG)\(\s*PC_(?:DR|DDR|ALT1|ALT2)\b",
        r"\bPC_(?:DR|DDR|ALT1|ALT2)\s*=",
        r"\bUART1_[A-Z0-9_]+\s*[|&^]?=",
        r"\bSETREG_LCR1\s*\(",
    )
    mutations = [
        (match.start(), match.group())
        for pattern in mutation_patterns
        for match in re.finditer(pattern, open_uart1)
    ]
    if len(mutations) != 17:
        raise ParallelError(
            f"UART1 mutation inventory changed from 17 to {len(mutations)}"
        )
    for position, mutation in mutations:
        if position < acquire or position > release:
            raise ParallelError(f"UART1 mutation escapes Port C guard: {mutation}")
    for required in (
        "if (emos_parallel_uart1_guard_acquire() != EMOS_PARALLEL_OK)",
        "return UART_ERR_FAILURE;",
        "serialFlags |= 0x10;",
        "SETREG_LCR1(",
    ):
        if required not in open_uart1:
            raise ParallelError(f"UART1 guarded transition lacks {required!r}")
    if mos_api.count("CALL\t_open_UART1") != 1:
        raise ParallelError("MOS UART-open API does not use the guarded choke point")
    if mos_api.count("DW24\t_open_UART1\t; 0x08") != 1:
        raise ParallelError("mos_getfunction slot 0x08 does not use guarded UART1 open")
    if io.count("AND\tEMOS_PARALLEL_CONTROL_BITS") < 4:
        raise ParallelError("Port D helper arguments are not masked to owned bits")
    for required in (
        "LD\tA, I",
        "DI",
        "EI",
        "IN0\tA, (PD_DR)",
        "OUT0\t(PD_DR), A",
        "EMOS_PARALLEL_KEEP_CONTROL",
        "OR\tEMOS_PARALLEL_READY_BIT",
    ):
        if required not in io:
            raise ParallelError(f"atomic Port D helper lacks {required!r}")


def verify_linked(
    elf: Path, nm: Path, objdump: Path, *, allow_fixed_qualification: bool = False
) -> None:
    symbols = linked_symbols(nm, elf)
    addresses = require_global_text_symbols(symbols, REQUIRED_SYMBOLS)
    addresses.update(
        require_text_symbols(
            symbols,
            (
                "EMOS_vdu_parallel",
                "EMOS_vdu_onboard",
                "EMOS_vdu_parallel_PUTCH_failed",
                "EMOS_vdu_WRITE_onboard",
                "_rst_10_handler",
                "_rst_18_handler_0",
                "_rst_18_handler_1",
                "__init",
                "_main",
                "_init_UART1",
                "mos_api_uopen",
            ),
        )
    )
    addresses.update(
        require_data_symbols(
            symbols,
            (
                "_emosParallelPinsOwned",
                "_emosParallelWriterLock",
                "_emosParallelDeferredFault",
                "_emosParallelRouteLease",
                "_serialFlags",
            ),
        )
    )
    backend_records = symbols.get("_emosVduBackend", [])
    if len(backend_records) != 1:
        raise ParallelError("linked image must contain exactly one _emosVduBackend")
    addresses["_emosVduBackend"] = backend_records[0][0]
    predecessor = sorted(name for name in symbols if PREDECESSOR_SYMBOL.match(name))
    if predecessor:
        raise ParallelError(
            "production image retains predecessor symbols: "
            + ", ".join(predecessor)
        )
    forbidden = (
        []
        if allow_fixed_qualification
        else [name for name in FORBIDDEN_NORMAL_SYMBOLS if name in symbols]
    )
    if forbidden:
        raise ParallelError(
            "normal/release image links qualification-only symbols: "
            + ", ".join(forbidden)
        )

    # INTEG-005: Core-owned RTS adds only PC_DR/PC_DDR writes. Keep the
    # whole-image owner inventory and bound each new writer to its exact
    # register sequence; real-driver host tests check PC2 masking/ownership.
    rts_portc_writes = {
        "_uart1_claim_rts": [0x9E, 0x9F],
        "_uart1_receive_ready": [0x9E],
        "_close_UART1": [0x9E, 0x9F],
    }
    observed_rts_writes = {name: [] for name in rts_portc_writes}
    allowed_portc_writers = {
        "__init",
        "_init_UART1",
        "_open_UART1",
        "_emos_parallel_epoch_enter",
        "_emos_parallel_release_local_pins",
        "_emos_parallel_ez80_write_data",
        *rts_portc_writes,
    }
    text_names_by_address: dict[int, set[str]] = {}
    for name, records in symbols.items():
        for address, kind in records:
            if kind in ("T", "t"):
                text_names_by_address.setdefault(address, set()).add(name)
    text_addresses = sorted(text_names_by_address)
    whole_image = subprocess.run(
        [str(objdump), "-d", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout.lower()
    observed_portc_writers: set[str] = set()
    for line in whole_image.splitlines():
        output = re.match(
            r"^\s*([0-9a-f]+):.*\bout0\s+\(0x([0-9a-f]+)\)", line
        )
        if output is None:
            continue
        instruction_address = int(output.group(1), 16)
        port = int(output.group(2), 16)
        if not 0x9E <= port <= 0xA1:
            continue
        owners = [address for address in text_addresses if address <= instruction_address]
        if not owners:
            raise ParallelError("linked Port C write has no owning text symbol")
        owner_names = text_names_by_address[owners[-1]]
        admitted = owner_names & allowed_portc_writers
        if not admitted:
            raise ParallelError(
                "linked Port C write escapes the maintained owner allowlist: "
                + ", ".join(sorted(owner_names))
            )
        observed_portc_writers.update(admitted)
        for name in admitted & rts_portc_writes.keys():
            observed_rts_writes[name].append(port)
    if observed_portc_writers != allowed_portc_writers:
        missing = allowed_portc_writers - observed_portc_writers
        unexpected = observed_portc_writers - allowed_portc_writers
        raise ParallelError(
            "linked Port C writer set changed; missing="
            + ",".join(sorted(missing))
            + " unexpected="
            + ",".join(sorted(unexpected))
        )
    if observed_rts_writes != rts_portc_writes:
        raise ParallelError("linked UART1 RTS register-write inventory changed")
    if addresses["__init"] >= addresses["_main"]:
        raise ParallelError("startup Port C defaults are not linked before main")

    def body(symbol: str) -> str:
        return disassemble(objdump, elf, symbol)

    def require_transfers(
        caller: str,
        callee: str,
        expected: int = 1,
        mnemonics: tuple[str, ...] = ("call", "jp"),
    ) -> None:
        actual = direct_transfer_count(
            body(caller), addresses[callee], mnemonics=mnemonics
        )
        if actual != expected:
            raise ParallelError(
                f"{caller} has {actual} direct transfers to {callee}, "
                f"expected {expected}"
            )

    for dispatcher in ("EMOS_vdu_PUTCH", "EMOS_vdu_WRITE"):
        references = address_reference_count(
            body(dispatcher), addresses["_emosVduBackend"]
        )
        if references != 1:
            raise ParallelError(
                f"{dispatcher} snapshots _emosVduBackend {references} times"
            )

    require_transfers("_putch", "EMOS_vdu_PUTCH", mnemonics=("call",))
    require_transfers("_rst_10_handler", "EMOS_vdu_PUTCH", mnemonics=("call",))
    require_transfers("_rst_18_handler_0", "EMOS_vdu_WRITE", mnemonics=("call",))
    require_transfers("_rst_18_handler_1", "EMOS_vdu_PUTCH", mnemonics=("call",))
    require_transfers("EMOS_vdu_parallel", "EMOS_vdu_parallel_PUTCH", mnemonics=("jp",))
    require_transfers("EMOS_vdu_onboard", "UART0_serial_PUTCH", mnemonics=("jp",))
    require_transfers(
        "EMOS_vdu_parallel_PUTCH",
        "_emos_parallel_route_write_byte",
        mnemonics=("call",),
    )
    whole_rows = instructions(whole_image)
    byte_bridge_rows = [
        (mnemonic, operands)
        for address, mnemonic, operands in whole_rows
        if addresses["EMOS_vdu_parallel_PUTCH"]
        <= address
        < addresses["EMOS_vdu_WRITE"]
    ]
    byte_failure = addresses["EMOS_vdu_parallel_PUTCH_failed"]
    expected_byte_bridge_rows = [
        ("push", "bc"),
        ("push", "de"),
        ("push", "hl"),
        ("push", "ix"),
        ("push", "iy"),
        ("push", "af"),
        ("ld", "hl,0x0000"),
        ("ld", "l,a"),
        ("push", "hl"),
        ("call", None),
        ("pop", "hl"),
        ("or", "a,a"),
        ("jr", f"nz,0x{byte_failure:x}"),
        ("pop", "af"),
        ("pop", "iy"),
        ("pop", "ix"),
        ("pop", "hl"),
        ("pop", "de"),
        ("pop", "bc"),
        ("scf", ""),
        ("ret", ""),
        ("pop", "af"),
        ("pop", "iy"),
        ("pop", "ix"),
        ("pop", "hl"),
        ("pop", "de"),
        ("pop", "bc"),
        ("or", "a,a"),
        ("ret", ""),
    ]
    if len(byte_bridge_rows) != len(expected_byte_bridge_rows):
        raise ParallelError("linked parallel byte bridge instruction count changed")
    for index, (actual, expected) in enumerate(
        zip(byte_bridge_rows, expected_byte_bridge_rows, strict=True)
    ):
        expected_mnemonic, expected_operands = expected
        if actual[0] != expected_mnemonic or (
            expected_operands is not None and actual[1] != expected_operands
        ):
            raise ParallelError(
                "linked parallel byte bridge ABI sequence changed "
                f"at instruction {index}: got {actual}, expected {expected}"
            )
    require_transfers(
        "EMOS_vdu_WRITE_onboard", "UART0_serial_PUTCH", mnemonics=("call",)
    )
    require_transfers(
        "EMOS_vdu_parallel_WRITE",
        "_emos_parallel_route_write_stream",
        mnemonics=("call",),
    )
    parallel_write_rows = [
        (mnemonic, operands)
        for _, mnemonic, operands in instructions(body("EMOS_vdu_parallel_WRITE"))
    ]
    expected_parallel_write_rows = [
        ("push", "de"),
        ("push", "ix"),
        ("push", "iy"),
        ("push", "hl"),
        ("push", "bc"),
        ("push", "bc"),
        ("push", "hl"),
        ("call", None),
        ("pop", "de"),
        ("pop", "de"),
        ("pop", "de"),
        ("pop", "hl"),
        ("ld", "bc,0x0000"),
        ("ld", "b,d"),
        ("ld", "c,e"),
        ("add", "hl,bc"),
        ("ld", "bc,0x0000"),
        ("pop", "iy"),
        ("pop", "ix"),
        ("pop", "de"),
        ("or", "a,a"),
        ("ret", "nz"),
        ("scf", ""),
        ("ret", ""),
    ]
    if len(parallel_write_rows) != len(expected_parallel_write_rows):
        raise ParallelError("linked parallel block bridge instruction count changed")
    for index, (actual, expected) in enumerate(
        zip(parallel_write_rows, expected_parallel_write_rows, strict=True)
    ):
        expected_mnemonic, expected_operands = expected
        if actual[0] != expected_mnemonic or (
            expected_operands is not None and actual[1] != expected_operands
        ):
            raise ParallelError(
                "linked parallel block bridge argument/epilogue sequence changed "
                f"at instruction {index}: got {actual}, expected {expected}"
            )
    require_transfers(
        "_emos_parallel_route_enter",
        "_emos_parallel_epoch_enter",
        mnemonics=("call",),
    )
    require_transfers(
        "_emos_parallel_route_leave",
        "_emos_parallel_epoch_leave",
    )
    require_transfers(
        "_emos_parallel_route_write_byte",
        "_emos_parallel_write_byte",
    )
    require_transfers(
        "_emos_parallel_route_write_stream",
        "_emos_parallel_write_stream",
    )
    for route_wrapper in (
        "_emos_parallel_route_enter",
        "_emos_parallel_route_leave",
        "_emos_parallel_route_write_stream",
        "_emos_parallel_route_write_byte",
        "_emos_parallel_route_owned",
    ):
        references = address_reference_count(
            body(route_wrapper), addresses["_emosParallelRouteLease"]
        )
        if references != 1:
            raise ParallelError(
                f"{route_wrapper} references the common route lease "
                f"{references} times, expected one"
            )
    require_transfers(
        "_emos_parallel_uart1_guard_acquire",
        "_emos_parallel_io_try_reserve_portc",
        mnemonics=("call",),
    )
    require_transfers(
        "_open_UART1",
        "_emos_parallel_uart1_guard_acquire",
        mnemonics=("call",),
    )
    require_transfers(
        "_open_UART1",
        "_emos_parallel_uart1_guard_release",
        mnemonics=("call",),
    )
    require_transfers("mos_api_uopen", "_open_UART1", mnemonics=("call",))
    require_transfers("_main", "_init_UART1", mnemonics=("call",))
    open_uart1_body = body("_open_UART1")
    open_uart1_rows = instructions(open_uart1_body)
    acquire_indexes = direct_transfer_indexes(
        open_uart1_body,
        addresses["_emos_parallel_uart1_guard_acquire"],
        mnemonics=("call",),
    )
    release_indexes = direct_transfer_indexes(
        open_uart1_body,
        addresses["_emos_parallel_uart1_guard_release"],
        mnemonics=("call",),
    )
    if len(acquire_indexes) != 1 or len(release_indexes) != 1:
        raise ParallelError("linked UART1 guard calls are not unique")
    acquire_index = acquire_indexes[0]
    release_index = release_indexes[0]
    if acquire_index >= release_index:
        raise ParallelError("linked UART1 guard release does not follow acquire")
    out_indexes = [
        index
        for index, (_, mnemonic, _) in enumerate(open_uart1_rows)
        if mnemonic == "out0"
    ]
    if not out_indexes:
        raise ParallelError("linked UART1 open contains no hardware mutation")
    if any(index <= acquire_index or index >= release_index for index in out_indexes):
        raise ParallelError("linked UART1 hardware mutation escapes the guard")
    serial_references = [
        index
        for index, (_, _, operands) in enumerate(open_uart1_rows)
        if addresses["_serialFlags"]
        in [
            int(value, 16)
            for value in re.findall(r"0x([0-9a-f]+)", operands)
        ]
    ]
    if not serial_references or any(
        index <= acquire_index or index >= release_index
        for index in serial_references
    ):
        raise ParallelError("linked UART1 serialFlags access escapes the guard")
    if acquire_index + 2 >= len(open_uart1_rows):
        raise ParallelError("linked UART1 acquire has no rejection branch")
    status_test = open_uart1_rows[acquire_index + 1]
    rejection = open_uart1_rows[acquire_index + 2]
    rejection_target = re.fullmatch(r"nz,0x([0-9a-f]+)", rejection[2])
    if status_test[1:] != ("or", "a,a") or rejection[1] not in ("jp", "jr"):
        raise ParallelError("linked UART1 acquire result is not tested immediately")
    if rejection_target is None:
        raise ParallelError("linked UART1 acquire failure does not branch on nonzero")
    target_address = int(rejection_target.group(1), 16)
    target_indexes = [
        index
        for index, (address, _, _) in enumerate(open_uart1_rows)
        if address == target_address
    ]
    if len(target_indexes) != 1 or target_indexes[0] <= release_index:
        raise ParallelError("linked UART1 rejection does not bypass mutation/release")
    target_index = target_indexes[0]
    if open_uart1_rows[target_index][1:] != ("ld", "a,0xff"):
        raise ParallelError("linked UART1 rejection does not return UART_ERR_FAILURE")
    if release_index + 1 >= len(open_uart1_rows) or open_uart1_rows[
        release_index + 1
    ][1:] != ("xor", "a,a"):
        raise ParallelError("linked UART1 success does not return UART_ERR_NONE")
    main_rows = instructions(body("_main"))
    init_calls = direct_transfer_indexes(
        body("_main"), addresses["_init_UART1"], mnemonics=("call",)
    )
    di_indexes = [
        index for index, (_, mnemonic, _) in enumerate(main_rows) if mnemonic == "di"
    ]
    ei_indexes = [
        index for index, (_, mnemonic, _) in enumerate(main_rows) if mnemonic == "ei"
    ]
    if (
        len(init_calls) != 1
        or len(di_indexes) != 1
        or len(ei_indexes) != 1
        or not di_indexes[0] < init_calls[0] < ei_indexes[0]
    ):
        raise ParallelError("linked init_UART1 is not inside main's boot DI window")
    if not any(
        mnemonic == "out0"
        for _, mnemonic, _ in open_uart1_rows[acquire_index + 1 : release_index]
    ):
        raise ParallelError("linked UART1 guard does not enclose hardware mutation")
    uart1_address = symbols.get("UART1_serial_PUTCH", [(None, "")])[0][0]
    if uart1_address is not None:
        for caller in (
            "EMOS_vdu_PUTCH",
            "EMOS_vdu_parallel",
            "EMOS_vdu_parallel_PUTCH",
            "EMOS_vdu_WRITE",
            "EMOS_vdu_parallel_WRITE",
        ):
            if direct_transfer_count(body(caller), uart1_address):
                raise ParallelError(f"{caller} falls through to forbidden UART1")

    for symbol in (
        "_emos_parallel_engine_configure",
        "_emos_parallel_engine_open",
        "_emos_parallel_engine_close",
        "_emos_parallel_engine_write",
    ):
        function_body = disassemble(objdump, elf, symbol)
        if not instructions(function_body):
            raise ParallelError(f"{symbol} has no linked instruction extent")
        mnemonics = instruction_mnemonics(function_body)
        if "di" in mnemonics or "ei" in mnemonics:
            raise ParallelError(f"{symbol} disables interrupts inside the engine")

    helper_limits = {
        "_emos_parallel_io_write_control": 8,
        "_emos_parallel_io_configure": 20,
        "_emos_parallel_io_try_lock": 16,
        "_emos_parallel_io_try_begin_entry": 22,
        "_emos_parallel_io_try_reserve_portc": 18,
        "_emos_parallel_io_read_clock": 10,
        "_emos_parallel_io_commit_epoch": 28,
    }
    helper_bodies = {
        symbol: disassemble_global_extent(objdump, elf, symbol)
        for symbol in helper_limits
    }
    for symbol, limit in helper_limits.items():
        verify_iff_helper(symbol, helper_bodies[symbol], limit)

    acquire_body = body("_emos_parallel_uart1_guard_acquire")
    for state_name in ("_emosParallelWriterLock", "_emosParallelPinsOwned"):
        if address_reference_count(acquire_body, addresses[state_name]) != 1:
            raise ParallelError(f"UART1 guard does not bind exactly once to {state_name}")
    if address_reference_count(
        acquire_body, addresses["_emosParallelDeferredFault"]
    ):
        raise ParallelError("UART1 guard mutates or inspects deferred parallel fault")
    release_rows = instructions(body("_emos_parallel_uart1_guard_release"))
    expected_release = [
        ("xor", "a,a"),
        ("ld", f"(0x{addresses['_emosParallelWriterLock']:x}),a"),
        ("ret", ""),
    ]
    if [(mnemonic, operands) for _, mnemonic, operands in release_rows] != expected_release:
        raise ParallelError("UART1 guard release is not one zero-store to the shared lock")

    reserve_rows = instructions(helper_bodies["_emos_parallel_io_try_reserve_portc"])

    def one_row(mnemonic: str, operands: str) -> int:
        matches = [
            index
            for index, (_, actual_mnemonic, actual_operands) in enumerate(reserve_rows)
            if actual_mnemonic == mnemonic and actual_operands == operands
        ]
        if len(matches) != 1:
            raise ParallelError(
                "atomic Port C reservation lacks one exact "
                f"{mnemonic} {operands} instruction"
            )
        return matches[0]

    lock_arg = one_row("ld", "hl,(iy+6)")
    pins_arg = one_row("ld", "de,(iy+9)")
    status_arg = one_row("ld", "de,(iy+12)")
    lock_read = one_row("ld", "a,(hl)")
    pins_read = one_row("ld", "a,(de)")
    lock_set = one_row("ld", "(hl),0x01")
    success = one_row("xor", "a,a")
    busy = one_row("ld", "a,0xe2")
    status_store = one_row("ld", "(de),a")
    branches = [
        (index, operands)
        for index, (_, mnemonic, operands) in enumerate(reserve_rows)
        if mnemonic == "jr" and operands.startswith("nz,")
    ]
    if len(branches) != 2 or branches[0][1] != branches[1][1]:
        raise ParallelError("atomic Port C reservation lacks a common busy branch")
    busy_target = re.fullmatch(r"nz,0x([0-9a-f]+)", branches[0][1])
    if busy_target is None or int(busy_target.group(1), 16) != reserve_rows[busy][0]:
        raise ParallelError("atomic Port C reservation busy branches miss E2")
    if not (
        lock_arg
        < lock_read
        < branches[0][0]
        < pins_arg
        < pins_read
        < branches[1][0]
        < lock_set
        < success
        < busy
        < status_arg
        < status_store
    ):
        raise ParallelError("atomic Port C reservation state operations are out of order")

    control = helper_bodies["_emos_parallel_io_write_control"]
    configure = helper_bodies["_emos_parallel_io_configure"]
    if control.count("and a,0xb0") != 1 or control.count("and a,0x4f") != 1:
        raise ParallelError("linked control RMW does not mask the exact owned bits")
    if configure.count("and a,0xb0") != 3 or configure.count("and a,0x4f") != 3:
        raise ParallelError("linked mux RMW does not mask the exact owned bits")
    clock_rows = instructions(helper_bodies["_emos_parallel_io_read_clock"])
    if not any(mnemonic == "ld" and operands == "b,0x04" for _, mnemonic, operands in clock_rows):
        raise ParallelError("linked atomic clock snapshot is not exactly four bytes")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--nm", required=True, type=Path)
    parser.add_argument("--objdump", required=True, type=Path)
    args = parser.parse_args()
    verify_source(args.source.resolve())
    verify_linked(
        args.elf.resolve(), args.nm.resolve(), args.objdump.resolve()
    )
    print(
        "EMOS production parallel linked-image entry points and instruction "
        "shapes verified, including semantic dispatch, bounded-block "
        "argument/epilogue, and UART1-guard reachability; object origin, target "
        "execution, and electrical behavior "
        "are not claimed"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ParallelError, OSError, subprocess.CalledProcessError) as error:
        print(f"EMOS parallel verification failed: {error}", file=sys.stderr)
        raise SystemExit(1) from None
