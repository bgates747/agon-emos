from pathlib import Path
import runpy
import shutil
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
M=runpy.run_path(str(ROOT/'projects/emos/verify_parallel.py'))
class PollGuardTests(unittest.TestCase):
    def test_only_core_diagnostic_call_is_admitted(self):
        M['verify_source'](ROOT)
        for filename in ('emos.c','emos_parallel.c','emos_parallel_engine.c','serial.asm'):
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as td:
                target=Path(td);shutil.copytree(ROOT/'src',target/'src')
                with (target/'src'/filename).open('a') as f:f.write('\nvoid forbidden(void) { emos_general_poll(); }\n')
                with self.assertRaises(M['ParallelError']):M['verify_source'](target)
    def test_legacy_guard_cannot_be_removed(self):
        with tempfile.TemporaryDirectory() as td:
            target=Path(td);shutil.copytree(ROOT/'src',target/'src');p=target/'src/emos.c'
            head,branch=p.read_text().split('if (strcasecmp(operation, "vdppoll") == 0)',1)
            p.write_text(head+'if (strcasecmp(operation, "vdppoll") == 0)'+branch.replace('emosModeState.mode != EMOS_MODE_LEGACY','0',1))
            with self.assertRaises(M['ParallelError']):M['verify_source'](target)
