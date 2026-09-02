"""Execute and structurally audit the production forward-parallel engine."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "src" / "emos_parallel_engine.c"
BINDING = ROOT / "src" / "emos_parallel.c"
FIXED_BACKEND = ROOT / "src" / "emos_parallel_fixed_backend.c"
HEADER = ROOT / "src" / "emos_parallel.h"
SERIAL = ROOT / "src" / "serial.asm"
UART = ROOT / "src" / "uart.c"
HARNESS = ROOT / "tests" / "emos_parallel_engine_harness.c"
BINDING_HARNESS = ROOT / "tests" / "emos_parallel_binding_harness.c"
UART_GUARD_HARNESS = ROOT / "tests" / "emos_uart1_guard_harness.c"
HOST_INCLUDE = ROOT / "tests" / "host"


class EmosParallelTests(unittest.TestCase):
    def test_exact_production_engine_executes_against_deterministic_wire(self) -> None:
        compiler = shutil.which("cc")
        self.assertIsNotNone(compiler, "host C compiler is required")
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "emos-parallel-engine"
            subprocess.run(
                [
                    compiler,
                    "-std=c17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-pedantic",
                    f"-I{HOST_INCLUDE}",
                    f"-I{ROOT / 'src'}",
                    str(ENGINE),
                    str(HARNESS),
                    "-o",
                    str(executable),
                ],
                cwd=ROOT,
                check=True,
            )
            result = subprocess.run(
                [str(executable)],
                cwd=ROOT,
                check=True,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        self.assertEqual(result.stdout, "EMOS parallel engine host checks passed\n")
        self.assertEqual(result.stderr, "")

    def test_exact_production_binding_executes_against_host_registers(self) -> None:
        compiler = shutil.which("cc")
        self.assertIsNotNone(compiler, "host C compiler is required")
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "emos-parallel-binding"
            subprocess.run(
                [
                    compiler,
                    "-std=c17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-pedantic",
                    f"-I{HOST_INCLUDE}",
                    f"-I{ROOT / 'src'}",
                    str(ENGINE),
                    str(BINDING),
                    str(FIXED_BACKEND),
                    str(BINDING_HARNESS),
                    "-o",
                    str(executable),
                ],
                cwd=ROOT,
                check=True,
            )
            result = subprocess.run(
                [str(executable)],
                cwd=ROOT,
                check=True,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        self.assertEqual(result.stdout, "EMOS parallel binding host checks passed\n")
        self.assertEqual(result.stderr, "")

    def test_exact_uart1_open_obeys_parallel_guard(self) -> None:
        compiler = shutil.which("cc")
        self.assertIsNotNone(compiler, "host C compiler is required")
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "emos-uart1-guard"
            subprocess.run(
                [
                    compiler,
                    "-std=c17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-pedantic",
                    # Inherited ZDS headers use ``#endif NAME`` labels.  Keep
                    # every other host warning fatal while compiling them.
                    "-Wno-endif-labels",
                    f"-I{HOST_INCLUDE}",
                    f"-I{ROOT / 'src'}",
                    str(UART),
                    str(UART_GUARD_HARNESS),
                    "-o",
                    str(executable),
                ],
                cwd=ROOT,
                check=True,
            )
            result = subprocess.run(
                [str(executable)],
                cwd=ROOT,
                check=True,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        self.assertEqual(result.stdout, "EMOS UART1 guard host checks passed\n")
        self.assertEqual(result.stderr, "")

    def test_engine_has_no_test_or_circuit_specific_product_branch(self) -> None:
        source = ENGINE.read_text(encoding="utf-8")
        for forbidden in (
            "UNIT_TEST",
            "EMOS_PORT008_FORWARD",
            "light2-harness-r01",
            "General Poll",
            "general_poll",
            "DI()",
            "EI()",
            "__asm",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

    def test_target_binding_owns_exact_shared_ez80_endpoints(self) -> None:
        source = BINDING.read_text(encoding="utf-8")
        for required in (
            "EMOS_PARALLEL_READY_BIT   ((BYTE)0x10)",
            "EMOS_PARALLEL_CLOCK_BIT   ((BYTE)0x20)",
            "EMOS_PARALLEL_VALID_BIT   ((BYTE)0x80)",
            "PC_DR = value;",
            "emos_parallel_io_write_control(value);",
            "if (serialFlags & 0x10)",
            "emos_parallel_release_local_pins();",
            "BYTE lockStatus = EMOS_PARALLEL_BUSY;",
            "emos_parallel_generation_next(",
            "emosParallelDeferredFault",
            "emos_parallel_io_read_clock(sysvars, snapshot);",
            "emos_parallel_io_try_reserve_portc(",
            "static t_emosParallelEpochLease emosParallelRouteLease;",
            "BYTE emos_parallel_route_write_stream",
            "BYTE emos_parallel_uart1_guard_acquire",
        ):
            with self.subTest(required=required):
                self.assertIn(required, source)
        self.assertNotIn("general_poll", source)
        self.assertNotIn("EMOS_PORT008_FORWARD", source)
        self.assertNotIn("DI()", source)
        self.assertNotIn("EI()", source)
        self.assertNotIn("emosParallelWriterLockStatus", source)

    def test_semantic_dispatcher_has_only_production_parallel_route(self) -> None:
        source = SERIAL.read_text(encoding="utf-8")
        for required in (
            "EMOS_vdu_PUTCH:",
            "EMOS_vdu_WRITE:",
            "EMOS_vdu_parallel_PUTCH:",
            "EMOS_vdu_parallel_WRITE:",
            "CALL\t_emos_parallel_route_write_byte",
            "CALL\t_emos_parallel_route_write_stream",
            "LD\tBC, 0",
            "LD\tB, D",
            "LD\tC, E",
        ):
            with self.subTest(required=required):
                self.assertIn(required, source)
        for obsolete in (
            "PORT008_",
            "_port008_",
            "_emos_port008_",
            "EMOS_PORT008_FORWARD",
        ):
            with self.subTest(obsolete=obsolete):
                self.assertNotIn(obsolete, source)

    def test_uart1_open_reserves_portc_before_every_mutation(self) -> None:
        source = UART.read_text(encoding="utf-8")
        body = source.split("BYTE open_UART1", 1)[1].split("// Close UART1", 1)[0]
        acquire = body.index("emos_parallel_uart1_guard_acquire()")
        release = body.index("emos_parallel_uart1_guard_release()")
        for mutation in (
            "serialFlags &= 0x0F",
            "SETREG(PC_DDR",
            "UART1_LCTL |=",
            "serialFlags |= 0x10",
            "SETREG_LCR1(",
        ):
            with self.subTest(mutation=mutation):
                self.assertLess(acquire, body.index(mutation))
                self.assertLess(body.index(mutation), release)
        self.assertIn("return UART_ERR_FAILURE;", body[: body.index("serialFlags")])

    def test_rejected_writer_publishes_status_before_unlock(self) -> None:
        source = BINDING.read_text(encoding="utf-8")
        write = source.split("BYTE emos_parallel_write_stream", 1)[1]
        invalid = write.split("if (!emosParallelPinsOwned", 1)[1].split(
            "\n\t}", 1
        )[0]
        deferred = write.split(
            "if (emosParallelDeferredFault != EMOS_PARALLEL_OK)", 1
        )[1].split("\n\t}", 1)[0]
        self.assertLess(
            invalid.index("emosParallelBindingStatus = EMOS_PARALLEL_NOT_OWNED"),
            invalid.index("emosParallelWriterLock = 0"),
        )
        self.assertLess(
            deferred.index("emosParallelBindingStatus = status"),
            deferred.index("emosParallelWriterLock = 0"),
        )
        self.assertIn("return status;", deferred)

    def test_contract_constants_are_exact_width_and_internal(self) -> None:
        header = HEADER.read_text(encoding="utf-8")
        for required in (
            "((UINT16)4096)",
            "((UINT24)0xFFFFFF)",
            "EMOS_PARALLEL_ADMISSION_TIMEOUT",
            "EMOS_PARALLEL_COMPLETION_TIMEOUT",
            "t_emosParallelEpochLease",
        ):
            with self.subTest(required=required):
                self.assertIn(required, header)


if __name__ == "__main__":
    unittest.main()
