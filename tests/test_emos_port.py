"""Continuing-port provenance checks for the maintained EMOS sources."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
MOS_AGONDEV_ROOT = Path(
    os.environ.get("MOS_AGONDEV_ROOT", ROOT.parent / "mos-agondev")
).expanduser().resolve()
WORKTREE = Path(
    os.environ.get(
        "MOS_AGONDEV_WORKTREE",
        MOS_AGONDEV_ROOT / "projects" / "mos-port" / "worktree",
    )
).expanduser().resolve()
PORT = MOS_AGONDEV_ROOT / "projects" / "mos-port"
PROFILE = ROOT / "port" / "mos-agondev.mk"


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _make_variable(path: Path, name: str) -> list[str]:
    lines = path.read_text(encoding="utf-8").splitlines()
    start = re.compile(rf"^{re.escape(name)}\s*:=\s*(.*)$")
    for index, line in enumerate(lines):
        match = start.match(line)
        if match is None:
            continue
        logical = match.group(1)
        while logical.rstrip().endswith("\\"):
            logical = logical.rstrip()[:-1] + " "
            index += 1
            if index >= len(lines):
                raise AssertionError(f"unterminated Make variable {name} in {path}")
            logical += lines[index].split("#", 1)[0]
        return logical.split()
    raise AssertionError(f"missing Make variable {name} in {path}")


class EmosPortTests(unittest.TestCase):
    def test_zds_project_crlf_policy_is_explicit(self) -> None:
        attributes = (ROOT / ".gitattributes").read_text(encoding="utf-8")
        self.assertIn("MOS.zdsproj -text whitespace=cr-at-eol", attributes)
        project = (ROOT / "MOS.zdsproj").read_bytes()
        self.assertIn(b"\r\n", project)
        self.assertEqual(project.replace(b"\r\n", b"").count(b"\n"), 0)

    def test_zds_project_owns_the_new_maintained_source_once(self) -> None:
        project = (WORKTREE / "MOS.zdsproj").read_text(encoding="utf-8")
        for relative in (
            r".\\src\\emos.c",
            r".\\src\\emos_uart_probe.c",
            r".\\src\\emos_uart_flow.c",
            r".\\src\\emos_parallel.c",
            r".\\src\\emos_parallel_engine.c",
            r".\\src\\emos_parallel_io.asm",
        ):
            with self.subTest(relative=relative):
                self.assertEqual(project.count(relative), 1)

    def test_uart1_parallel_guard_is_not_profile_optional(self) -> None:
        uart = (ROOT / "src" / "uart.c").read_text(encoding="utf-8")
        project = (ROOT / "MOS.zdsproj").read_text(encoding="utf-8")
        self.assertNotIn("EMOS_PARALLEL_DATA_PLANE", uart)
        self.assertEqual(uart.count("emos_parallel_uart1_guard_acquire()"), 1)
        self.assertEqual(uart.count("emos_parallel_uart1_guard_release()"), 1)
        for relative in (
            r".\\src\\emos_parallel.c",
            r".\\src\\emos_parallel_engine.c",
            r".\\src\\emos_parallel_io.asm",
            r".\\src\\uart.c",
        ):
            with self.subTest(relative=relative):
                self.assertEqual(project.count(relative), 1)

    def test_prepared_snapshot_contains_exact_unpatched_emos_sources(self) -> None:
        metadata = json.loads(
            (WORKTREE / ".mos-agondev-worktree.json").read_text(encoding="utf-8")
        )
        files = {entry["path"]: entry for entry in metadata["files"]}
        self.assertIn("src/emos.c", files)
        self.assertIn("src/emos.h", files)
        for relative in (
            "src/emos.c",
            "src/emos_keyboard.c",
            "src/emos_keyboard.h",
            "src/emos_keyboard_io.asm",
            "src/emos.h",
            "src/emos_uart_probe.c",
            "src/emos_uart_probe.h",
            "src/emos_uart_flow.c",
            "src/emos_uart_flow.h",
            "src/emos_parallel.c",
            "src/emos_parallel_engine.c",
            "src/emos_parallel.h",
            "src/emos_parallel_io.asm",
        ):
            self.assertIn(relative, files)
            self.assertEqual(files[relative]["sha256"], _sha256(WORKTREE / relative))

    def test_product_profile_declares_emos_source_and_object_once(self) -> None:
        sources = _make_variable(PROFILE, "C_SOURCES_EXTRA")
        objects = _make_variable(PROFILE, "C_OBJECT_RELATIVE_EXTRA")
        commands = _make_variable(PROFILE, "PARITY_EXPECTED_COMMANDS")
        linked_checks = _make_variable(PROFILE, "FIRMWARE_LINK_CHECKS")
        self.assertEqual(
            sources,
            ["src/emos_sdlink.c", "src/emos_console.c", "src/emos_keyboard.c", "src/emos_uart_flow.c", "src/emos_uart_probe.c", "src/emos.c", "src/emos_parallel.c", "src/emos_parallel_engine.c"],
        )
        self.assertEqual(
            objects,
            ["src/emos_sdlink.o", "src/emos_console.o", "src/emos_keyboard.o", "src/emos_uart_flow.o", "src/emos_uart_probe.o", "src/emos.o", "src/emos_parallel.o", "src/emos_parallel_engine.o"],
        )
        self.assertEqual(
            _make_variable(PROFILE, "ASM_SOURCES_EXTRA"),
            ["src/emos_console_io.asm", "src/emos_keyboard_io.asm", "src/emos_parallel_io.asm"],
        )
        self.assertEqual(
            _make_variable(PROFILE, "ASM_OBJECT_RELATIVE_EXTRA"),
            ["src/emos_console_io.o", "src/emos_keyboard_io.o", "src/emos_parallel_io.o"],
        )
        self.assertEqual(commands, ["EMOS"])
        profile_text = PROFILE.read_text(encoding="utf-8")
        self.assertNotRegex(profile_text, r"(?m)^CPPFLAGS_EXTRA\s*:=")
        self.assertEqual(
            _make_variable(PROFILE, "C_SOURCE_CPPFLAGS_RELATIVE"),
            ["src/emos.c"],
        )
        scoped_flags = _make_variable(PROFILE, "C_SOURCE_CPPFLAGS_EXTRA")
        self.assertTrue(scoped_flags)
        self.assertNotIn("EMOS_PARALLEL_DATA_PLANE", " ".join(scoped_flags))
        self.assertEqual(
            linked_checks,
            [
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_uart_baud.py",
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_parallel.py",
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_keyboard.py",
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_console.py",
            ],
        )

        port008 = ROOT / "port" / "port008-forward.mk"
        predecessor = port008.read_text(encoding="utf-8")
        self.assertIn("SUPERSEDED", predecessor)
        self.assertIn("$(error", predecessor)
        self.assertNotIn("EMOS_PORT008_FORWARD", predecessor)

        fixed = ROOT / "port" / "parallel-fixed-qualification.mk"
        self.assertEqual(
            _make_variable(fixed, "C_SOURCES_EXTRA"),
            [
                "src/emos_sdlink.c",
                "src/emos_console.c",
                "src/emos_keyboard.c",
                "src/emos_uart_flow.c",
                "src/emos_uart_probe.c",
                "src/emos.c",
                "src/emos_parallel.c",
                "src/emos_parallel_engine.c",
                "src/emos_parallel_fixed_backend.c",
            ],
        )
        self.assertEqual(
            _make_variable(fixed, "ASM_SOURCES_EXTRA"),
            ["src/emos_console_io.asm", "src/emos_keyboard_io.asm", "src/emos_parallel_io.asm"],
        )
        fixed_text = fixed.read_text(encoding="utf-8")
        self.assertNotRegex(fixed_text, r"(?m)^CPPFLAGS_EXTRA\s*:=")
        self.assertIn(
            "include $(dir $(lastword $(MAKEFILE_LIST)))identity.mk",
            fixed_text,
        )
        self.assertIn("EMOS_IDENTITY_CPPFLAGS", fixed_text)
        self.assertIn("EMOS_QUALIFICATION_COMPOSITION_IDENTITY", fixed_text)
        self.assertNotIn("EMOS_PARALLEL_DATA_PLANE", fixed_text)
        self.assertIn("EMOS_PARALLEL_FIXED_QUALIFICATION=1", fixed_text)
        self.assertEqual(
            _make_variable(fixed, "C_SOURCE_CPPFLAGS_RELATIVE"),
            ["src/emos.c"],
        )
        self.assertTrue(
            _make_variable(fixed, "C_SOURCE_CPPFLAGS_EXTRA")
        )
        self.assertEqual(
            _make_variable(fixed, "FIRMWARE_LINK_CHECKS"),
            [
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_uart_baud.py",
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_parallel_fixed.py",
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_keyboard.py",
                "$(EMOS_PROFILE_ROOT)/projects/emos/verify_console.py",
            ],
        )

        product_make = (ROOT / "Makefile").read_text(encoding="utf-8")
        self.assertIn(
            'MOS_AGONDEV_WORKTREE="$(abspath $(MOS_WORKTREE))"', product_make
        )
        for target_name in (
            "firmware-check",
            "parallel-fixed-firmware-check",
            "qualify",
        ):
            target = product_make.split(f"{target_name}:", 1)[1].split(
                "\n\n", 1
            )[0]
            self.assertIn(
                'MOS_WORKTREE="$(abspath $(MOS_WORKTREE))"',
                target,
                target_name,
            )
            self.assertIn(
                'MOS_MAINTAINED_SOURCE="$(abspath $(MOS_SOURCE))"',
                target,
                target_name,
            )
            self.assertIn(
                'PROVENANCE_DIR="$(PROVENANCE_DIR_ABS)"',
                target,
                target_name,
            )
            self.assertIn(
                'TOOLCHAIN="$(abspath $(AGONDEV_TOOLCHAIN))"',
                target,
                target_name,
            )
        fixed_target = product_make.split(
            "parallel-fixed-firmware-check:", 1
        )[1].split("\n\n", 1)[0]
        self.assertIn("$(MAKE) contract-linked-check", fixed_target)
        self.assertIn("non-release fixed data-plane composition", product_make)
        self.assertNotIn("port008-firmware-check", product_make)
        self.assertNotIn("port008-linked-check", product_make)

        generic_build = (PORT / "Makefile").read_text(encoding="utf-8")
        generic_runtime = (PORT / "runtime" / "Makefile").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("src/emos.c", generic_build)
        self.assertNotIn("src/emos.o", generic_runtime)
        self.assertNotIn("emos_parallel", generic_build + generic_runtime)

    def test_generated_assembly_is_manifest_owned_and_unmodified(self) -> None:
        prepared = json.loads(
            (WORKTREE / ".mos-agondev-worktree.json").read_text(encoding="utf-8")
        )
        with tempfile.TemporaryDirectory() as directory:
            generated = Path(directory) / "generated"
            subprocess.run(
                [
                    sys.executable,
                    "-B",
                    str(PORT / "tools" / "zds2gas.py"),
                    "tree",
                    str(WORKTREE),
                    str(generated),
                ],
                cwd=ROOT,
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            manifest = json.loads(
                (generated / "manifest.json").read_text(encoding="utf-8")
            )
            provenance = manifest["input_provenance"]
            self.assertEqual(provenance["source_head"], prepared["source"]["head"])
            self.assertEqual(
                provenance["tracked_dirty"], prepared["source"]["tracked_dirty"]
            )
            self.assertEqual(provenance["prepared_file_count"], len(prepared["files"]))
            outputs = set()
            for entry in manifest["files"]:
                output = entry["output"]
                outputs.add(output)
                self.assertEqual(entry["output_sha256"], _sha256(generated / output))
        expected = set(
            _make_variable(PORT / "assembly-profile.mk", "ASM_SOURCES_BASE")
        )
        expected.update(_make_variable(PROFILE, "ASM_SOURCES_EXTRA"))
        self.assertEqual(outputs, expected)


if __name__ == "__main__":
    unittest.main()
