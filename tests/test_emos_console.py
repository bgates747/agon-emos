from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ConsoleTests(unittest.TestCase):
    def test_real_adapter_with_scripted_uart_and_interrupts(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'console'
            subprocess.run(['cc', '-std=c17', '-Wall', '-Wextra', '-Werror',
                            '-Wno-endif-labels', '-fsanitize=address,undefined',
                            '-I'+str(ROOT/'tests/host'), '-I'+str(ROOT/'src'),
                            str(ROOT/'src/emos_console.c'),
                            str(ROOT/'tests/emos_console_harness.c'), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=20)
