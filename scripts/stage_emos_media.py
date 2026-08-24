#!/usr/bin/env python3
"""Stage one generated EMOS SD-card case without overwriting local state."""

from __future__ import annotations

import argparse
import os
import shutil
from pathlib import Path


PROJECT_ROOT = Path(__file__).absolute().parent.parent
DEFAULT_SOURCE = PROJECT_ROOT / "projects/emos/build/media/cases/valid/sdcard"
DEFAULT_PROFILE = PROJECT_ROOT.parent / "mos-agondev/emulator"


class StageError(RuntimeError):
    """Raised when staging would obscure or replace local profile state."""


def absolute(path: Path) -> Path:
    return Path(os.path.abspath(path.expanduser()))


def source_files(source: Path) -> dict[Path, Path]:
    source = absolute(source)
    required = (source / "emos/modules", source / "emos.commands")
    if not required[0].is_dir() or not required[1].is_file():
        raise StageError(f"EMOS media is incomplete; run make emos-media-check: {source}")
    files = {
        path.relative_to(source): path
        for path in sorted(source.rglob("*"))
        if path.is_file() and not path.is_symlink()
    }
    expected = {
        Path("emos.commands"),
        Path("emos/modules/hello.emo"),
        Path("emos/modules/echo.emo"),
        Path("emos/modules/edu-probe.emo"),
    }
    if set(files) != expected:
        raise StageError(
            "EMOS valid media must contain exactly the reviewed integration files"
        )
    return files


def _require_real_directory(path: Path, label: str) -> None:
    if path.is_symlink() or not path.is_dir():
        raise StageError(f"Expected real {label} directory: {path}")


def stage(source: Path, profile: Path) -> int:
    files = source_files(source)
    profile = absolute(profile)
    sdcard = profile / "sdcard"
    _require_real_directory(profile, "emulator profile")
    _require_real_directory(sdcard, "SD-card root")

    managed_roots = (sdcard / "emos", sdcard / "emos.commands")
    for root in managed_roots:
        if root.is_symlink():
            raise StageError(f"Refusing symlinked EMOS destination: {root}")

    expected_destinations = {sdcard / relative for relative in files}
    existing = {
        path
        for root in managed_roots
        if root.exists()
        for path in ([root] if root.is_file() else root.rglob("*"))
        if path.is_file() or path.is_symlink()
    }
    unexpected = existing - expected_destinations
    if unexpected:
        names = ", ".join(str(path) for path in sorted(unexpected))
        raise StageError(f"Refusing unreviewed existing EMOS profile files: {names}")

    for relative, source_path in files.items():
        destination = sdcard / relative
        if destination.is_symlink():
            raise StageError(f"Refusing symlinked EMOS destination: {destination}")
        if destination.exists():
            if not destination.is_file():
                raise StageError(f"Refusing non-file EMOS destination: {destination}")
            if destination.read_bytes() != source_path.read_bytes():
                raise StageError(f"Refusing to overwrite differing profile file: {destination}")

    copied = 0
    for relative, source_path in files.items():
        destination = sdcard / relative
        if destination.exists():
            continue
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source_path, destination)
        copied += 1
    return copied


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--profile", type=Path, default=DEFAULT_PROFILE)
    arguments = parser.parse_args()
    try:
        copied = stage(arguments.source, arguments.profile)
    except StageError as error:
        raise SystemExit(f"EMOS media staging error: {error}") from error
    print(f"EMOS valid media staged: {copied} new files")


if __name__ == "__main__":
    main()
