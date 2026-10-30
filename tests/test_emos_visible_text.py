from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class VisibleTextTests(unittest.TestCase):
    def test_real_command_handles_reply_failures_deadlines_and_cleanup(self):
        with tempfile.TemporaryDirectory() as temp:
            executable = Path(temp) / "visible-text"
            (Path(temp)/"ff.h").write_text("enum { FR_OK=0, FR_INVALID_PARAMETER=19 };\n")
            subprocess.run(["cc", "-std=c17", "-Wall", "-Wextra", "-Werror", "-Wno-endif-labels",
                            "-fsanitize=address,undefined", "-DMOS_DEFINES_H", "-DEMOS_BUSY=31",
                            "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast",
                            "-include", str(ROOT/"tests/host/defines.h"),
                            "-I"+temp, "-I"+str(ROOT/"projects/uartflow/src"),
                            "-I"+str(ROOT/"projects/uartprobe/src"),
                            "-I"+str(ROOT/"tests/host"), "-I"+str(ROOT/"src"),
                            str(ROOT/"projects/uartprobe/src/visible_text.c"),
                            str(ROOT/"src/emos_uart_flow.c"),
                            str(ROOT/"tests/emos_visible_text_harness.c"),
                            "-o", str(executable)], check=True)
            result = subprocess.run([str(executable)], check=True, text=True, capture_output=True)
            self.assertEqual(result.stdout.count("VDP TEXT PASS: parser ACK - confirm browser text; returning to MOS"), 2)
            self.assertIn("42 visible text transport scenarios and invalid-input checks passed", result.stdout)
            self.assertIn("transmit timeout", result.stdout)
            self.assertIn("MOS clock stalled", result.stdout)


if __name__ == "__main__":
    unittest.main()
