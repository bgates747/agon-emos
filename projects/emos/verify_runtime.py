#!/usr/bin/env python3
"""Exercise the reviewed EMOS v1 transcript inside the target MOS under Fab."""

from __future__ import annotations

import argparse
from collections import Counter
import os
from pathlib import Path
import subprocess
from typing import TypeAlias


PROJECT_ROOT = Path(__file__).resolve().parents[2]
MOS_AGONDEV_ROOT = Path(
    os.environ.get("MOS_AGONDEV_ROOT", PROJECT_ROOT.parent / "mos-agondev")
).expanduser().resolve()
DEFAULT_FAB_ROOT = MOS_AGONDEV_ROOT / "fab-agon-emulator"
DEFAULT_FIRMWARE = MOS_AGONDEV_ROOT / "projects/mos-port/bin/MOS.bin"
DEFAULT_SDCARD = PROJECT_ROOT / "projects/emos/build/media/cases/valid/sdcard"
MAX_OUTPUT_BYTES = 1024 * 1024
MEDIA_FILES = {
    "emos.commands",
    "emos/modules/echo.emo",
    "emos/modules/edu-probe.emo",
    "emos/modules/hello.emo",
}
COMMAND_LINES = (
    b"EMOS DISCOVER",
    b"EMOS STATUS",
    b"HELLO",
    b"EMOS CALL core.echo deterministic echo",
    b"EMOS CALL edu.probe separate edu result",
    b"EMOS FAKE ON",
    b"EMOS MODE DUAL",
    b"EMOS STATUS",
    b"EMOS MODE LEGACY",
    b"EMOS FAKE OFF",
    b"EMOS STATUS",
)
COMMANDS = b"\n".join(COMMAND_LINES) + b"\n"
COMMAND_FILE = b"\r\n".join(COMMAND_LINES) + b"\r\n"
EXPECTED_SEQUENCE = (
    b"Agon Platform MOS Version 3.0.2 Arthur",
    b"/ *EMOS DISCOVER",
    b"EMOS: discovered 3 provider(s)",
    b"/ *EMOS STATUS",
    b"EMOS v1: Legacy, registry 3, generation 1",
    b"VDU route 0, EDU inactive, adapter unavailable, mode generation 0",
    b"hello 1.0.0 /emos/modules/hello.emo",
    b"core.echo 1.0.0 /emos/modules/echo.emo",
    b"edu.probe 0.1.0 /emos/modules/edu-probe.emo",
    b"/ *HELLO",
    b"Hello",
    b"/ *EMOS CALL core.echo deterministic echo",
    b"EMOS service: deterministic echo",
    b"/ *EMOS CALL edu.probe separate edu result",
    b"EMOS service: separate edu result",
    b"/ *EMOS FAKE ON",
    b"/ *EMOS MODE DUAL",
    b"/ *EMOS STATUS",
    b"EMOS v1: Dual, registry 3, generation 1",
    b"VDU route 0, EDU active, adapter fake, mode generation 1",
    b"/ *EMOS MODE LEGACY",
    b"/ *EMOS FAKE OFF",
    b"/ *EMOS STATUS",
    b"EMOS v1: Legacy, registry 3, generation 1",
    b"VDU route 0, EDU inactive, adapter unavailable, mode generation 2",
)
FORBIDDEN_OUTPUT = (
    b"Invalid command",
    b"No SD card present",
    b"EMOS provider failed",
    b"EMOS backend unavailable",
    b"Error:",
)
SnapshotEntry: TypeAlias = tuple[str, bytes | None]


class EmosRuntimeError(RuntimeError):
    """Target EMOS did not satisfy the reviewed integration contract."""


def find_cli(fab_root: Path) -> Path:
    candidates = (
        fab_root / "target/release/agon-cli-emulator",
        fab_root / "agon-cli-emulator",
    )
    for candidate in candidates:
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate
    raise EmosRuntimeError(
        "Fab CLI emulator is missing or not executable; checked:\n"
        + "\n".join(f"  {candidate}" for candidate in candidates)
    )


def snapshot_media(sdcard: Path) -> dict[str, SnapshotEntry]:
    if sdcard.is_symlink() or not sdcard.is_dir():
        raise EmosRuntimeError(f"EMOS SD-card root is not a real directory: {sdcard}")
    snapshot: dict[str, SnapshotEntry] = {}
    for path in sorted(sdcard.rglob("*")):
        relative = str(path.relative_to(sdcard))
        if path.is_symlink():
            raise EmosRuntimeError(f"EMOS media contains a symlink: {relative}")
        if path.is_dir():
            snapshot[relative] = ("directory", None)
        elif path.is_file():
            snapshot[relative] = ("file", path.read_bytes())
        else:
            raise EmosRuntimeError(f"EMOS media contains a special path: {relative}")
    actual_files = {name for name, (kind, _) in snapshot.items() if kind == "file"}
    if actual_files != MEDIA_FILES:
        raise EmosRuntimeError(
            "EMOS valid media file inventory differs from the reviewed four files"
        )
    command_file = snapshot["emos.commands"][1]
    if command_file != COMMAND_FILE:
        raise EmosRuntimeError("EMOS command file differs from the reviewed transcript")
    return snapshot


def validate_output(output: bytes) -> None:
    if not output:
        raise EmosRuntimeError("Fab produced no EMOS runtime output")
    if len(output) > MAX_OUTPUT_BYTES:
        raise EmosRuntimeError(f"Fab output exceeded {MAX_OUTPUT_BYTES} bytes")
    normalized = output.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    banner = normalized.find(b"Agon Platform MOS Version")
    if banner < 0:
        raise EmosRuntimeError("MOS banner is absent from EMOS runtime output")
    normalized = normalized[banner:]
    lines = [line.strip(b"\x00\x80 ") for line in normalized.splitlines()]

    forbidden = [token for token in FORBIDDEN_OUTPUT if token in normalized]
    if forbidden:
        raise EmosRuntimeError(
            "EMOS runtime output contains forbidden result: "
            + ", ".join(repr(token) for token in forbidden)
        )
    expected_commands = Counter(COMMAND_LINES)
    for command, expected_count in expected_commands.items():
        actual_count = lines.count(b"/ *" + command)
        if actual_count != expected_count:
            raise EmosRuntimeError(
                f"command echo count for {command!r} is {actual_count}, "
                f"expected {expected_count}"
            )

    cursor = 0
    for token in EXPECTED_SEQUENCE:
        try:
            cursor = lines.index(token, cursor) + 1
        except ValueError as error:
            raise EmosRuntimeError(
                f"EMOS runtime sequence is missing or reorders {token!r}"
            ) from error


def verify(cli: Path, firmware: Path, sdcard: Path, timeout: float) -> bytes:
    if timeout <= 0:
        raise EmosRuntimeError("timeout must be positive")
    if not cli.is_file() or not os.access(cli, os.X_OK):
        raise EmosRuntimeError(f"Fab CLI emulator is not executable: {cli}")
    if not firmware.is_file():
        raise EmosRuntimeError(f"candidate firmware is missing: {firmware}")
    before = snapshot_media(sdcard)
    try:
        completed = subprocess.run(
            [
                os.fspath(cli),
                "--mos",
                os.fspath(firmware),
                "--sdcard",
                os.fspath(sdcard),
                "--unlimited-cpu",
                "--zero",
            ],
            input=COMMANDS,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as error:
        raise EmosRuntimeError(f"EMOS runtime exceeded {timeout:g} seconds") from error
    if completed.returncode != 0:
        raise EmosRuntimeError(f"Fab exited with status {completed.returncode}")
    validate_output(completed.stdout)
    after = snapshot_media(sdcard)
    if after != before:
        raise EmosRuntimeError("EMOS valid media changed during the runtime check")
    return completed.stdout


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fab-root", type=Path, default=DEFAULT_FAB_ROOT)
    parser.add_argument("--firmware", type=Path, default=DEFAULT_FIRMWARE)
    parser.add_argument("--sdcard", type=Path, default=DEFAULT_SDCARD)
    parser.add_argument("--timeout", type=float, default=30.0)
    arguments = parser.parse_args()
    try:
        cli = find_cli(arguments.fab_root.expanduser().resolve())
        output = verify(
            cli,
            arguments.firmware.expanduser().resolve(),
            arguments.sdcard.expanduser().resolve(),
            arguments.timeout,
        )
    except (OSError, EmosRuntimeError) as error:
        print(f"verify_runtime.py: {error}", file=os.sys.stderr)
        return 1
    print(
        "EMOS target runtime verified: discovery, star/service providers, "
        f"fake Dual, and Legacy recovery ({len(output)} output bytes)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
