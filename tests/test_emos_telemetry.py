from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class TelemetryTests(unittest.TestCase):
    def test_real_gateway_in_emulated_application_address_space(self):
        with tempfile.TemporaryDirectory() as temp:
            tmp = Path(temp)
            # Only result constants are needed from FatFS; no filesystem is
            # simulated by this transport-boundary test.
            (tmp/'ff.h').write_text('enum { FR_OK=0, FR_TIMEOUT=15, FR_INVALID_PARAMETER=19 };\n')
            exe = tmp/'gateway'
            subprocess.run(['cc', '-std=c17', '-Wall', '-Wextra', '-Werror',
                            '-Wno-endif-labels', '-Wno-pointer-to-int-cast',
                            '-Wno-int-to-pointer-cast', '-fsanitize=address,undefined',
                            '-DEMOS_BENCH_TELEMETRY=1', '-DEMOS_BUSY=31', '-DEMOS_UNAVAILABLE=35',
                            '-DMOS_DEFINES_H', '-include',str(ROOT/'tests/host/defines.h'),
                            '-I'+str(tmp), '-I'+str(ROOT/'tests/host'),
                            '-I'+str(ROOT/'src'), str(ROOT/'src/emos_telemetry.c'),
                            str(ROOT/'tests/emos_telemetry_harness.c'), '-o', str(exe)],check=True)
            subprocess.run([str(exe)], check=True, timeout=10)


if __name__ == '__main__':
    unittest.main()
