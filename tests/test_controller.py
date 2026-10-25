from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

class ControllerTests(unittest.TestCase):
    def test_resident_cache_delivery_faults_and_lifecycle(self):
        with tempfile.TemporaryDirectory() as temp:
            out=Path(temp)/'controller'
            (Path(temp)/'ff.h').write_text('enum {FR_OK=0,FR_TIMEOUT=15,FR_INVALID_PARAMETER=19};\n')
            subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror','-Wno-endif-labels',
                            '-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast',
                            '-fsanitize=address,undefined','-DMOS_DEFINES_H',
                            '-DEMOS_BUSY=31','-DEMOS_UNSAFE_CALLER=32','-DEMOS_UNAVAILABLE=35',
                            '-include',str(ROOT/'tests/host/defines.h'),'-I'+temp,
                            '-I'+str(ROOT/'tests/host'),'-I'+str(ROOT/'src'),
                            str(ROOT/'src/emos_controller.c'),str(ROOT/'tests/emos_controller_harness.c'),
                            '-o',str(out)],check=True)
            result=subprocess.run([str(out)],check=True,text=True,capture_output=True)
            self.assertIn('checks pass',result.stdout)

if __name__=='__main__':unittest.main()
