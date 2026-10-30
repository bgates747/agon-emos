from pathlib import Path
import runpy
import shutil
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
M=runpy.run_path(str(ROOT/'projects/emos/verify_parallel.py'))
class TextGuardTests(unittest.TestCase):
    def test_only_core_diagnostic_call_is_admitted(self):
        M['verify_source'](ROOT)
        for filename in ('emos.c','emos_parallel.c','emos_parallel_engine.c','serial.asm'):
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as td:
                target=Path(td);shutil.copytree(ROOT/'src',target/'src')
                with (target/'src'/filename).open('a') as f:f.write('\nvoid forbidden(void) { emos_visible_text(); }\n')
                with self.assertRaises(M['ParallelError']):M['verify_source'](target)
    def test_fake_adapter_cannot_reenter_ordinary_source(self):
        with tempfile.TemporaryDirectory() as td:
            target=Path(td);shutil.copytree(ROOT/'src',target/'src')
            with (target/'src/emos.c').open('a') as f:
                f.write('\nstatic int emos_select_fake(int enabled) { return enabled; }\n')
            with self.assertRaises(M['ParallelError']):M['verify_source'](target)
