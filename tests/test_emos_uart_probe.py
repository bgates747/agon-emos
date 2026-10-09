from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class UartProbeTests(unittest.TestCase):
    def test_real_command_handles_reply_failures_deadlines_and_cleanup(self):
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp)/"ff.h").write_text("enum { FR_OK=0, FR_INVALID_PARAMETER=19 };\n")
            executable = Path(temp) / "uart-probe"
            subprocess.run(["cc", "-std=c17", "-Wall", "-Wextra", "-Werror", "-Wno-endif-labels",
                            "-fsanitize=address,undefined", "-DEMOS_UART_PROBE_HOST_TEST",
                            "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast",
                            "-DEMOS_BUSY=31", "-DMOS_DEFINES_H", "-include", str(ROOT/"tests/host/defines.h"),
                            "-I"+temp, "-I"+str(ROOT/"projects/uartflow/src"), "-I"+str(ROOT/"projects/uartprobe/src"),
                            "-I" + str(ROOT / "tests/host"), "-I" + str(ROOT / "src"),
                            str(ROOT / "projects/uartprobe/src/probe.c"), str(ROOT/"src/emos_uart_flow.c"),
                            str(ROOT / "tests/emos_uart_probe_harness.c"),
                            "-o", str(executable)], check=True)
            result = subprocess.run([str(executable)], check=True, text=True, capture_output=True)
            self.assertEqual(result.stdout.count("UART ROUND TRIP PASS"), 2)
            self.assertIn("no reply from P4", result.stdout)
            self.assertIn("MOS clock stalled", result.stdout)
            self.assertIn("extra reply bytes", result.stdout)


if __name__ == "__main__":
    unittest.main()
