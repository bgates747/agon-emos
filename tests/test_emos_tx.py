from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
class TransmitTests(unittest.TestCase):
    def test_real_sender_boundaries(self):
        with tempfile.TemporaryDirectory() as temp:
            exe=Path(temp)/'tx'
            for flags in ([], ['-DEMOS_BENCH_TELEMETRY=1']):
                subprocess.run(['cc', *flags, '-DEMOS_TX_BLOCK_C_REFERENCE=1', '-std=c17', '-Wall', '-Wextra', '-Werror',
                    '-Wno-endif-labels', '-fsanitize=address,undefined',
                    '-DMOS_DEFINES_H', '-include', str(ROOT/'tests/host/defines.h'),
                    '-I'+str(ROOT/'tests/host'), '-I'+str(ROOT/'src'),
                    str(ROOT/'src/emos_keyboard.c'), str(ROOT/'tests/emos_tx_harness.c'),
                    '-o', str(exe)], check=True)
                subprocess.run([str(exe)], check=True, timeout=20)
