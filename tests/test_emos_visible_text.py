from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class VisibleTextTests(unittest.TestCase):
    def test_real_command_handles_reply_failures_deadlines_and_cleanup(self):
        with tempfile.TemporaryDirectory() as temp:
            executable = Path(temp) / "visible-text"
            subprocess.run(["cc", "-std=c17", "-Wall", "-Wextra", "-Werror", "-Wno-endif-labels",
                            "-fsanitize=address,undefined", "-DEMOS_UART_PROBE_HOST_TEST",
                            "-I" + str(ROOT / "tests/host"), "-I" + str(ROOT / "src"),
                            str(ROOT / "src/emos_uart_probe.c"),
                            str(ROOT / "tests/emos_visible_text_harness.c"),
                            "-o", str(executable)], check=True)
            result = subprocess.run([str(executable)], check=True, text=True, capture_output=True)
            self.assertEqual(result.stdout.count("VDP TEXT PASS: parser ACK - confirm browser text; returning to MOS"), 2)
            self.assertIn("42 visible text transport scenarios and invalid-input checks passed", result.stdout)
            self.assertIn("transmit timeout", result.stdout)
            self.assertIn("MOS clock stalled", result.stdout)


if __name__ == "__main__":
    unittest.main()
