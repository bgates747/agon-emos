from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class KeyboardTests(unittest.TestCase):
    def test_real_receiver_with_irq_and_uart_boundary(self):
        with tempfile.TemporaryDirectory() as temp:
            exe = Path(temp) / 'keyboard'
            subprocess.run(['cc', '-std=c17', '-Wall', '-Wextra', '-Werror',
                            '-Wno-endif-labels', '-fsanitize=address,undefined',
                            '-I'+str(ROOT/'tests/host'), '-I'+str(ROOT/'src'),
                            str(ROOT/'src/emos_keyboard.c'),
                            str(ROOT/'tests/emos_keyboard_harness.c'),
                            '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=20)
