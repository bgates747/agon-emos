#!/usr/bin/env python3
"""Build and validate deterministic EMOS v1 transient-module containers."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
from typing import Any, Iterable, Sequence
import zlib


MAGIC = b"EMOD"
FORMAT_MAJOR = 1
FORMAT_MINOR = 0
CORE_ABI = 1
HEADER_SIZE = 128
MODULE_LIMIT = 32 * 1024
MODULE_BASE = 0x0B0000
PAYLOAD_ADDRESS = MODULE_BASE + HEADER_SIZE
PROVIDER_REQUEST_SIZE = 24
CLASS_STAR = 1
CLASS_SERVICE = 2
CLASS_NAMES = {"star": CLASS_STAR, "service": CLASS_SERVICE}
CLASS_VALUES = {value: key for key, value in CLASS_NAMES.items()}
NAMESPACE_SIZE = 16
NAME_SIZE = 24
IDENTITY_RE = re.compile(r"[a-z][a-z0-9._-]*\Z")
HEADER_CRC_OFFSET = 0x4C
POLICY_CORE = 0
POLICY_SAFE = 1
POLICY_COMPATIBLE = 2
POLICY_UNSAFE = 3
POLICY_MOSLET = 4
STATUS_RECOVERY_FAILED = 34
STATUS_UNAVAILABLE = 35


class ModuleError(ValueError):
    """The manifest, container, or registry violates the EMOS v1 contract."""


def _u16(value: int) -> bytes:
    return value.to_bytes(2, "little")


def _u24(value: int) -> bytes:
    return value.to_bytes(3, "little")


def _u32(value: int) -> bytes:
    return value.to_bytes(4, "little")


def _read_u16(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 2], "little")


def _read_u24(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 3], "little")


def _read_u32(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 4], "little")


def _integer(value: Any, field: str, low: int, high: int) -> int:
    if type(value) is not int or not low <= value <= high:
        raise ModuleError(f"{field} must be an integer from {low} through {high}")
    return value


def _identity(value: Any, field: str, maximum: int, allow_empty: bool) -> str:
    if not isinstance(value, str):
        raise ModuleError(f"{field} must be a string")
    if value == "" and allow_empty:
        return value
    if not IDENTITY_RE.fullmatch(value):
        raise ModuleError(f"{field} is not a canonical lowercase EMOS identity")
    if len(value.encode("ascii")) > maximum:
        raise ModuleError(f"{field} exceeds {maximum} bytes")
    return value


def normalize_manifest(document: Any) -> dict[str, Any]:
    if not isinstance(document, dict):
        raise ModuleError("manifest root must be an object")
    expected = {
        "schema",
        "class",
        "namespace",
        "name",
        "version",
        "core_abi",
        "entry_offset",
        "request_size",
        "flags",
        "capabilities",
    }
    extra = sorted(set(document) - expected)
    missing = sorted(expected - set(document))
    if missing:
        raise ModuleError("manifest is missing fields: " + ", ".join(missing))
    if extra:
        raise ModuleError("manifest has unknown fields: " + ", ".join(extra))
    if document["schema"] != 1:
        raise ModuleError("manifest schema must be 1")
    provider_class = CLASS_NAMES.get(document["class"])
    if provider_class is None:
        raise ModuleError("class must be 'star' or 'service'")
    namespace = _identity(
        document["namespace"], "namespace", NAMESPACE_SIZE, allow_empty=True
    )
    name = _identity(document["name"], "name", NAME_SIZE, allow_empty=False)
    if provider_class == CLASS_STAR and namespace:
        raise ModuleError("star providers require an empty namespace")
    if provider_class == CLASS_SERVICE and not namespace:
        raise ModuleError("service providers require a namespace")
    version = document["version"]
    if not isinstance(version, list) or len(version) != 3:
        raise ModuleError("version must contain three integers")
    version = [_integer(value, "version", 0, 255) for value in version]
    core_abi = document["core_abi"]
    if not isinstance(core_abi, list) or len(core_abi) != 2:
        raise ModuleError("core_abi must contain minimum and maximum")
    core_abi = [_integer(value, "core_abi", 1, 255) for value in core_abi]
    if core_abi[0] > core_abi[1]:
        raise ModuleError("core_abi minimum exceeds maximum")
    entry_offset = _integer(document["entry_offset"], "entry_offset", HEADER_SIZE, MODULE_LIMIT - 1)
    request_size = document["request_size"]
    if not isinstance(request_size, list) or len(request_size) != 2:
        raise ModuleError("request_size must contain minimum and maximum")
    request_size = [_integer(value, "request_size", PROVIDER_REQUEST_SIZE, 65535) for value in request_size]
    if request_size[0] > request_size[1]:
        raise ModuleError("request_size minimum exceeds maximum")
    flags = _integer(document["flags"], "flags", 0, 255)
    capabilities = _integer(document["capabilities"], "capabilities", 0, 0xFFFFFFFF)
    if flags != 0 or capabilities != 0:
        raise ModuleError("v1 external providers must use zero flags and capabilities")
    return {
        "schema": 1,
        "class": CLASS_VALUES[provider_class],
        "namespace": namespace,
        "name": name,
        "version": version,
        "core_abi": core_abi,
        "entry_offset": entry_offset,
        "request_size": request_size,
        "flags": flags,
        "capabilities": capabilities,
    }


def build_container(manifest: Any, payload: bytes) -> bytes:
    manifest = normalize_manifest(manifest)
    image_size = HEADER_SIZE + len(payload)
    if not payload:
        raise ModuleError("provider payload is empty")
    if image_size > MODULE_LIMIT:
        raise ModuleError(f"module image exceeds {MODULE_LIMIT} bytes")
    if manifest["entry_offset"] >= image_size:
        raise ModuleError("entry_offset lies outside the image")
    namespace = manifest["namespace"].encode("ascii")
    name = manifest["name"].encode("ascii")
    header = bytearray(HEADER_SIZE)
    header[0:4] = MAGIC
    header[4] = FORMAT_MAJOR
    header[5] = FORMAT_MINOR
    header[6:8] = _u16(HEADER_SIZE)
    header[8:11] = _u24(image_size)
    header[11:14] = _u24(manifest["entry_offset"])
    header[14:16] = bytes(manifest["core_abi"])
    header[16] = CLASS_NAMES[manifest["class"]]
    header[17] = manifest["flags"]
    header[18] = len(namespace)
    header[19] = len(name)
    header[20 : 20 + len(namespace)] = namespace
    header[36 : 36 + len(name)] = name
    header[60:63] = bytes(manifest["version"])
    header[64:68] = _u32(manifest["capabilities"])
    header[68:70] = _u16(manifest["request_size"][0])
    header[70:72] = _u16(manifest["request_size"][1])
    header[72:76] = _u32(zlib.crc32(payload) & 0xFFFFFFFF)
    header[HEADER_CRC_OFFSET : HEADER_CRC_OFFSET + 4] = _u32(zlib.crc32(header) & 0xFFFFFFFF)
    image = bytes(header) + payload
    validate_container(image)
    return image


def validate_container(image: bytes) -> dict[str, Any]:
    if len(image) < HEADER_SIZE:
        raise ModuleError("image is shorter than the fixed header")
    header = bytearray(image[:HEADER_SIZE])
    if header[0:4] != MAGIC:
        raise ModuleError("bad EMOD magic")
    if header[4] != FORMAT_MAJOR:
        raise ModuleError("unsupported format major")
    if header[5] > FORMAT_MINOR:
        raise ModuleError("unsupported format minor")
    if _read_u16(header, 6) != HEADER_SIZE:
        raise ModuleError("invalid header size")
    if _read_u24(header, 8) != len(image) or len(image) > MODULE_LIMIT:
        raise ModuleError("declared image size is invalid")
    entry_offset = _read_u24(header, 11)
    if not HEADER_SIZE <= entry_offset < len(image):
        raise ModuleError("entry offset lies outside payload")
    core_min, core_max = header[14], header[15]
    if not core_min <= CORE_ABI <= core_max:
        raise ModuleError("module is incompatible with this Core ABI")
    provider_class = header[16]
    if provider_class not in CLASS_VALUES:
        raise ModuleError("unknown provider class")
    if header[17] != 0 or _read_u32(header, 64) != 0:
        raise ModuleError("unsupported flags or capabilities")
    namespace_length, name_length = header[18], header[19]
    if namespace_length > NAMESPACE_SIZE or not 1 <= name_length <= NAME_SIZE:
        raise ModuleError("invalid identity length")
    namespace_field = bytes(header[20:36])
    name_field = bytes(header[36:60])
    if any(namespace_field[namespace_length:]) or any(name_field[name_length:]):
        raise ModuleError("identity padding is not zero")
    try:
        namespace = namespace_field[:namespace_length].decode("ascii")
        name = name_field[:name_length].decode("ascii")
    except UnicodeDecodeError as exc:
        raise ModuleError("identity is not ASCII") from exc
    _identity(namespace, "namespace", NAMESPACE_SIZE, allow_empty=True)
    _identity(name, "name", NAME_SIZE, allow_empty=False)
    if provider_class == CLASS_STAR and namespace:
        raise ModuleError("star provider has a namespace")
    if provider_class == CLASS_SERVICE and not namespace:
        raise ModuleError("service provider has no namespace")
    request_min = _read_u16(header, 68)
    request_max = _read_u16(header, 70)
    if request_min > PROVIDER_REQUEST_SIZE or request_max < PROVIDER_REQUEST_SIZE:
        raise ModuleError("invalid request size range")
    if any(header[63:64]) or any(header[80:128]):
        raise ModuleError("reserved header bytes are not zero")
    payload = image[HEADER_SIZE:]
    if _read_u32(header, 72) != (zlib.crc32(payload) & 0xFFFFFFFF):
        raise ModuleError("payload CRC mismatch")
    declared_header_crc = _read_u32(header, HEADER_CRC_OFFSET)
    header[HEADER_CRC_OFFSET : HEADER_CRC_OFFSET + 4] = b"\0\0\0\0"
    if declared_header_crc != (zlib.crc32(header) & 0xFFFFFFFF):
        raise ModuleError("header CRC mismatch")
    return {
        "schema": 1,
        "class": CLASS_VALUES[provider_class],
        "namespace": namespace,
        "name": name,
        "version": list(image[60:63]),
        "core_abi": [core_min, core_max],
        "entry_offset": entry_offset,
        "request_size": [request_min, request_max],
        "flags": header[17],
        "capabilities": _read_u32(header, 64),
        "image_size": len(image),
        "payload_crc32": f"{_read_u32(header, 72):08x}",
        "header_crc32": f"{declared_header_crc:08x}",
    }


def validate_registry(images: Iterable[tuple[str, bytes]]) -> list[dict[str, Any]]:
    entries = []
    for path, image in images:
        entry = validate_container(image)
        entry["path"] = path
        entries.append(entry)
    entries.sort(key=lambda entry: (CLASS_NAMES[entry["class"]], entry["namespace"], entry["name"], entry["path"]))
    previous: tuple[int, str, str] | None = None
    for entry in entries:
        key = (CLASS_NAMES[entry["class"]], entry["namespace"], entry["name"])
        if key == previous:
            raise ModuleError("duplicate provider claim: " + ".".join(part for part in key[1:] if part))
        previous = key
    return entries


def discover_registry(
    previous: dict[str, Any],
    images: Iterable[tuple[str, bytes]],
    *,
    maximum: int = 16,
    allocation_available: bool = True,
) -> dict[str, Any]:
    """Pure host oracle for Core's prepare-then-publish transaction.

    The caller retains ``previous`` if this function raises. The target uses
    the same bounded staging/validation/sort/collision/commit sequence and is
    structurally checked by the unit suite.
    """
    if not allocation_available:
        raise ModuleError("registry staging allocation failed")
    staged: list[tuple[str, bytes]] = []
    for item in images:
        if len(staged) >= maximum:
            raise ModuleError("registry is full")
        staged.append(item)
    entries = validate_registry(staged)
    generation = previous.get("generation", 0)
    if type(generation) is not int or not 0 <= generation <= 255:
        raise ModuleError("previous registry generation is invalid")
    return {"generation": (generation + 1) & 0xFF, "entries": entries}


def range_overlaps_module(address: int, length: int) -> bool:
    """Mirror the target's 24-bit request-buffer exclusion check."""
    _integer(address, "address", 0, 0xFFFFFF)
    _integer(length, "length", 0, 0xFFFFFF)
    if length == 0:
        return False
    end = (address + length) & 0xFFFFFF
    if end < address:
        return True
    return address < MODULE_BASE + MODULE_LIMIT and end > MODULE_BASE


def application_policy(image: bytes, address: int) -> int:
    """Classify an executable using the maintained advanced-header contract."""
    _integer(address, "address", 0, 0xFFFFFF)
    if address == MODULE_BASE:
        return POLICY_MOSLET
    if len(image) < 0x4A or image[0x40:0x43] != b"MOS":
        return POLICY_UNSAFE
    if image[0x43] != 1 or image[0x44] != 1:
        return POLICY_UNSAFE
    flags = image[0x45]
    if image[0x46] != ((~flags) & 0xFF) or flags & 0xFC or flags & 3 == 3:
        return POLICY_UNSAFE
    declared_address = _read_u24(image, 0x47)
    if declared_address not in (0, address):
        return POLICY_UNSAFE
    if flags & 1:
        return POLICY_SAFE
    if flags & 2:
        return POLICY_COMPATIBLE
    return POLICY_UNSAFE


def preservation_transaction(
    *, save_status: int = 0, provider_status: int = 0, restore_status: int = 0
) -> dict[str, Any]:
    """Pure failure-order oracle for a compatible application's area swap."""
    for field, value in (
        ("save_status", save_status),
        ("provider_status", provider_status),
        ("restore_status", restore_status),
    ):
        _integer(value, field, 0, 255)
    if save_status:
        return {
            "events": ["save", "discard-partial"],
            "provider_called": False,
            "status": save_status,
            "recovery_required": False,
        }
    events = ["save", "provider", "restore"]
    if restore_status:
        return {
            "events": events,
            "provider_called": True,
            "status": STATUS_RECOVERY_FAILED,
            "recovery_required": True,
        }
    return {
        "events": events + ["discard-swap"],
        "provider_called": True,
        "status": provider_status,
        "recovery_required": False,
    }


MODE_SHAPES = {
    0: (0, 0),
    1: (0, 1),
    2: (1, 1),
    3: (2, 1),
}


def mode_transaction(current: dict[str, int], target: int, adapter: str) -> dict[str, Any]:
    """Pure prepare/readiness/commit/recover oracle for the v1 adapters."""
    _integer(target, "target mode", 0, 3)
    if adapter not in ("unavailable", "fake"):
        raise ModuleError("unknown mode adapter")
    mode = current["mode"]
    generation = current["generation"]
    _integer(mode, "current mode", 0, 3)
    _integer(generation, "mode generation", 0, 255)
    if target == mode:
        return {"status": 0, "events": [], "state": dict(current)}
    if target != 0 and mode != 0:
        return {"status": STATUS_UNAVAILABLE, "events": [], "state": dict(current)}
    if target == 0:
        events = ["recover", "commit"]
    elif adapter == "fake" and target == 1:
        events = ["prepare", "ready", "adapter-commit", "commit"]
    else:
        return {
            "status": STATUS_UNAVAILABLE,
            "events": ["prepare", "recover"],
            "state": dict(current),
        }
    route, edu = MODE_SHAPES[target]
    return {
        "status": 0,
        "events": events,
        "state": {
            "mode": target,
            "vdu_route": route,
            "edu_active": edu,
            "generation": (generation + 1) & 0xFF,
        },
    }


def _load_manifest(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ModuleError(f"could not read manifest {path}: {exc}") from exc


def _write_output(path: Path, data: bytes) -> None:
    if path.exists() or path.is_symlink():
        raise ModuleError(f"refusing to replace existing output: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    build = commands.add_parser("build")
    build.add_argument("manifest", type=Path)
    build.add_argument("payload", type=Path)
    build.add_argument("output", type=Path)
    inspect = commands.add_parser("inspect")
    inspect.add_argument("image", type=Path)
    directory = commands.add_parser("validate-dir")
    directory.add_argument("directory", type=Path)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    try:
        if args.command == "build":
            image = build_container(_load_manifest(args.manifest), args.payload.read_bytes())
            _write_output(args.output, image)
            print(json.dumps(validate_container(image), indent=2, sort_keys=True))
        elif args.command == "inspect":
            print(json.dumps(validate_container(args.image.read_bytes()), indent=2, sort_keys=True))
        else:
            if not args.directory.is_dir():
                raise ModuleError(f"module directory does not exist: {args.directory}")
            paths = sorted(
                (
                    path
                    for path in args.directory.iterdir()
                    if path.is_file() and path.suffix.casefold() == ".emo"
                ),
                key=lambda path: (path.name.casefold(), path.name),
            )
            entries = validate_registry((path.name, path.read_bytes()) for path in paths)
            print(json.dumps(entries, indent=2, sort_keys=True))
    except (ModuleError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
