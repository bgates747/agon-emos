from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class UartFlowTests(unittest.TestCase):
    def test_real_command_handles_reply_failures_deadlines_and_cleanup(self):
        with tempfile.TemporaryDirectory() as temp:
            executable = Path(temp) / "uart-flow"
            (Path(temp)/'ff.h').write_text('enum { FR_OK=0, FR_INVALID_PARAMETER=19 };\n')
            subprocess.run(["cc", "-std=c17", "-Wall", "-Wextra", "-Werror", "-Wno-endif-labels",
                            "-fsanitize=address,undefined", "-DEMOS_UART_FLOW_HOST_TEST",
                            "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast",
                            "-DEMOS_BUSY=31", "-DMOS_DEFINES_H", "-include", str(ROOT/'tests/host/defines.h'),
                            "-I"+temp,
                            "-I" + str(ROOT / "tests/host"), "-I" + str(ROOT / "src"),
                            str(ROOT / "projects/uartflow/src/flow.c"), str(ROOT / "src/emos_uart_flow.c"),
                            str(ROOT / "tests/emos_uart_flow_harness.c"),
                            "-o", str(executable)], check=True)
            result = subprocess.run([str(executable)], check=True, text=True, capture_output=True)
            self.assertEqual(result.stdout.count("UART FLOW PASS - returning to MOS"), 1)
            self.assertIn("15 UART flow command scenarios passed", result.stdout)
            self.assertIn("CTS did not release", result.stdout)
            self.assertIn("MOS clock stalled", result.stdout)


if __name__ == "__main__":
    unittest.main()
