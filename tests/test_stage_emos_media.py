from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "stage_emos_media", ROOT / "scripts" / "stage_emos_media.py"
)
assert SPEC is not None and SPEC.loader is not None
stage_media = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(stage_media)


class StageEmosMediaTests(unittest.TestCase):
    def make_case(self, root: Path) -> tuple[Path, Path]:
        source = root / "source"
        modules = source / "emos/modules"
        modules.mkdir(parents=True)
        for name in ("hello.emo", "echo.emo", "edu-probe.emo"):
            (modules / name).write_bytes(name.encode("ascii"))
        (source / "emos.commands").write_bytes(b"*EMOS STATUS\r\n")
        profile = root / "profile"
        (profile / "sdcard").mkdir(parents=True)
        return source, profile

    def test_stage_is_complete_and_idempotent(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            source, profile = self.make_case(Path(raw))
            self.assertEqual(stage_media.stage(source, profile), 4)
            self.assertEqual(stage_media.stage(source, profile), 0)
            for path in source.rglob("*"):
                if path.is_file():
                    relative = path.relative_to(source)
                    self.assertEqual(path.read_bytes(), (profile / "sdcard" / relative).read_bytes())

    def test_refuses_differing_or_unreviewed_profile_files(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            source, profile = self.make_case(Path(raw))
            destination = profile / "sdcard/emos/modules"
            destination.mkdir(parents=True)
            (destination / "hello.emo").write_bytes(b"local")
            with self.assertRaisesRegex(stage_media.StageError, "overwrite differing"):
                stage_media.stage(source, profile)

            (destination / "hello.emo").write_bytes(b"hello.emo")
            (destination / "unreviewed.emo").write_bytes(b"local")
            with self.assertRaisesRegex(stage_media.StageError, "unreviewed existing"):
                stage_media.stage(source, profile)

    def test_refuses_symlinked_profile_paths(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            source, profile = self.make_case(root)
            target = root / "outside"
            target.mkdir()
            (profile / "sdcard/emos").symlink_to(target, target_is_directory=True)
            with self.assertRaisesRegex(stage_media.StageError, "symlinked"):
                stage_media.stage(source, profile)


if __name__ == "__main__":
    unittest.main()
