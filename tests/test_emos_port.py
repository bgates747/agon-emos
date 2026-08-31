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
    text = path.read_text(encoding="utf-8")
    match = re.search(
        rf"^{re.escape(name)}\s*:=\s*(.*?)(?=^[A-Z][A-Z0-9_]*\s*:?=|^\S[^\n]*:|\Z)",
        text,
        flags=re.MULTILINE | re.DOTALL,
    )
    if match is None:
        raise AssertionError(f"missing Make variable {name} in {path}")
    return re.findall(r"(?:^|\s)([^\s\\]+)", match.group(1))


class EmosPortTests(unittest.TestCase):
    def test_zds_project_owns_the_new_maintained_source_once(self) -> None:
        project = (WORKTREE / "MOS.zdsproj").read_text(encoding="utf-8")
        self.assertEqual(project.count(r".\\src\\emos.c"), 1)

    def test_prepared_snapshot_contains_exact_unpatched_emos_sources(self) -> None:
        metadata = json.loads(
            (WORKTREE / ".mos-agondev-worktree.json").read_text(encoding="utf-8")
        )
        files = {entry["path"]: entry for entry in metadata["files"]}
        self.assertIn("src/emos.c", files)
        self.assertIn("src/emos.h", files)
        for relative in ("src/emos.c", "src/emos.h"):
            self.assertEqual(files[relative]["sha256"], _sha256(WORKTREE / relative))

    def test_product_profile_declares_emos_source_and_object_once(self) -> None:
        sources = _make_variable(PROFILE, "C_SOURCES_EXTRA")
        objects = _make_variable(PROFILE, "C_OBJECT_RELATIVE_EXTRA")
        commands = _make_variable(PROFILE, "PARITY_EXPECTED_COMMANDS")
        self.assertEqual(sources, ["src/emos.c"])
        self.assertEqual(objects, ["src/emos.o"])
        self.assertEqual(commands, ["EMOS"])

        generic_build = (PORT / "Makefile").read_text(encoding="utf-8")
        generic_runtime = (PORT / "runtime" / "Makefile").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("src/emos.c", generic_build)
        self.assertNotIn("src/emos.o", generic_runtime)

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
        expected = set(_make_variable(PORT / "Makefile", "ASM_SOURCES"))
        self.assertEqual(outputs, expected)


if __name__ == "__main__":
    unittest.main()
