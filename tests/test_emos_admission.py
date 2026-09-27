"""Execute the maintained resident admission engine with a controlled wire peer."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class AdmissionTests(unittest.TestCase):
    def test_resident_state_and_fault_boundaries(self):
        with tempfile.TemporaryDirectory() as temp:
            exe = Path(temp)/'admission'
            subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror',
                '-Wno-endif-labels','-fsanitize=address,undefined',
                '-DEMOS_BUSY=31','-DEMOS_UNAVAILABLE=35','-DMOS_DEFINES_H',
                '-include',str(ROOT/'tests/host/defines.h'),
                '-I'+str(ROOT/'tests/host'),'-I'+str(ROOT/'src'),
                str(ROOT/'tests/emos_admission_harness.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True,timeout=10)

if __name__ == '__main__': unittest.main()
