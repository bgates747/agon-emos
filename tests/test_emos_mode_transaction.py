"""Execute the exact EMOS mode coordinator against a scripted fixed adapter."""

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "emos.c"


PREAMBLE = r'''
#include <stdio.h>
#include <string.h>
#include <strings.h>

typedef unsigned char BYTE;
typedef int BOOL;

#define TRUE 1
#define FALSE 0
#define FR_OK 0
#define FR_INVALID_PARAMETER 19
#define EMOS_BUSY 31
#define EMOS_UNAVAILABLE 35
#define EMOS_REGISTRY_FULL 36
#define EMOS_MODE_LEGACY 0
#define EMOS_MODE_DUAL 1
#define EMOS_MODE_EXCLUSIVE_COMPAT 2
#define EMOS_MODE_EXCLUSIVE_EXTENDED 3
#define EMOS_PARALLEL_FIXED_QUALIFICATION 1

#define EMOS_POLICY_CORE 0
static BYTE emosPolicy;
static BYTE emos_console_keep;
static BYTE consoleAvailable;
static BYTE emos_console_owned;
static BYTE emos_console_prepare(void) { return consoleAvailable; }
static BYTE emos_console_commit(void) { return consoleAvailable; }
static BYTE emos_console_recover(void) { return 1; }
static void emos_console_publish(void) { }
static void emos_console_notice(BYTE mode) { (void)mode; }
static BOOL emosBusy = FALSE;
volatile BYTE emosVduBackend = 0;

BYTE emos_parallel_fixed_enter(BYTE qualificationAssertion);
BYTE emos_parallel_fixed_ready(void);
BYTE emos_parallel_fixed_leave(void);
'''


HARNESS = r'''
static BYTE scriptedEnter;
static BYTE scriptedLeave;
static BYTE routeOwned;
static BYTE readyScript[8];
static unsigned readyLength;
static unsigned readyIndex;
static char events[32];
static unsigned eventLength;

static void event(char value) {
    events[eventLength++] = value;
    events[eventLength] = 0;
}

BYTE emos_parallel_fixed_enter(BYTE qualificationAssertion) {
    event('E');
    if (!qualificationAssertion) return 0xe4;
    if (scriptedEnter == FR_OK) routeOwned = TRUE;
    return scriptedEnter;
}

BYTE emos_parallel_fixed_ready(void) {
    BYTE ready = routeOwned;
    event('R');
    if (readyIndex < readyLength) ready = readyScript[readyIndex++];
    return ready;
}

BYTE emos_parallel_fixed_leave(void) {
    event('L');
    if (scriptedLeave == FR_OK) routeOwned = FALSE;
    return scriptedLeave;
}

static void resetCoordinator(void) {
    memset(&emosModeState, 0, sizeof(emosModeState));
    memset(&emosEduState, 0, sizeof(emosEduState));
    memset(readyScript, 0, sizeof(readyScript));
    memset(events, 0, sizeof(events));
    emosEduState.selectedAdapter = EMOS_ADAPTER_PARALLEL_FIXED;
    emosVduBackend = EMOS_VDU_ONBOARD;
    emosBusy = FALSE;
    emosPolicy = EMOS_POLICY_CORE;
    emos_console_keep = 0;
    scriptedEnter = FR_OK;
    scriptedLeave = FR_OK;
    routeOwned = FALSE;
    readyLength = 0;
    readyIndex = 0;
    eventLength = 0;
}

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int happyPath(void) {
    resetCoordinator();
    CHECK(emos_request_mode(EMOS_MODE_EXCLUSIVE_EXTENDED) == FR_OK);
    CHECK(strcmp(events, "ERR") == 0);
    CHECK(emosModeState.mode == EMOS_MODE_EXCLUSIVE_EXTENDED);
    CHECK(emosModeState.vduRoute == EMOS_VDU_EDP_EXTENDED);
    CHECK(emosModeState.generation == 1);
    CHECK(emosVduBackend == EMOS_VDU_EDP_EXTENDED);
    CHECK(emosEduState.active == TRUE);
    CHECK(emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED);
    CHECK(routeOwned == TRUE);

    eventLength = 0;
    events[0] = 0;
    CHECK(emos_request_mode(EMOS_MODE_LEGACY) == FR_OK);
    CHECK(strcmp(events, "RL") == 0);
    CHECK(emosModeState.mode == EMOS_MODE_LEGACY);
    CHECK(emosVduBackend == EMOS_VDU_ONBOARD);
    CHECK(routeOwned == FALSE);
    return 0;
}

static int failedPrepareCleansWithoutInventingAReleaseFailure(void) {
    resetCoordinator();
    scriptedEnter = 0xe4;
    CHECK(emos_request_mode(EMOS_MODE_EXCLUSIVE_EXTENDED) == EMOS_UNAVAILABLE);
    CHECK(strcmp(events, "ER") == 0);
    CHECK(emosModeState.mode == EMOS_MODE_LEGACY);
    CHECK(emosVduBackend == EMOS_VDU_ONBOARD);
    CHECK(emosEduState.lastStatus == 0xe4);
    CHECK(emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED);
    CHECK(routeOwned == FALSE);
    return 0;
}

static int failedEnterCleansPreownedRouteAndRetainsCause(void) {
    resetCoordinator();
    routeOwned = TRUE;
    scriptedEnter = 0xe2;
    CHECK(emos_request_mode(EMOS_MODE_EXCLUSIVE_EXTENDED) == EMOS_UNAVAILABLE);
    CHECK(strcmp(events, "ERL") == 0);
    CHECK(emosModeState.mode == EMOS_MODE_LEGACY);
    CHECK(emosVduBackend == EMOS_VDU_ONBOARD);
    CHECK(emosEduState.preparedMode == EMOS_MODE_LEGACY);
    CHECK(emosEduState.active == FALSE);
    CHECK(emosEduState.lastStatus == 0xe2);
    CHECK(emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED);
    CHECK(routeOwned == FALSE);
    return 0;
}

static int recoveryFailureRetainsOwnershipAndLegacyRetryReleases(void) {
    resetCoordinator();
    readyScript[0] = FALSE;
    readyScript[1] = TRUE;
    readyLength = 2;
    scriptedLeave = 0xe2;
    CHECK(emos_request_mode(EMOS_MODE_EXCLUSIVE_EXTENDED) == EMOS_UNAVAILABLE);
    CHECK(strcmp(events, "ERRL") == 0);
    CHECK(emosModeState.mode == EMOS_MODE_LEGACY);
    CHECK(emosVduBackend == EMOS_VDU_ONBOARD);
    CHECK(emosEduState.lastStatus == 0xe2);
    CHECK(emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED);
    CHECK(routeOwned == TRUE);

    eventLength = 0;
    events[0] = 0;
    readyIndex = 0;
    readyLength = 0;
    scriptedLeave = FR_OK;
    CHECK(emos_request_mode(EMOS_MODE_LEGACY) == FR_OK);
    CHECK(strcmp(events, "RRL") == 0);
    CHECK(routeOwned == FALSE);
    CHECK(emosModeState.mode == EMOS_MODE_LEGACY);
    CHECK(emosVduBackend == EMOS_VDU_ONBOARD);
    return 0;
}

static int successfulFakeSwitchRecoversFixedRouteAndRestoresDefault(void) {
    resetCoordinator();
    routeOwned = TRUE;
    CHECK(emos_select_fake(TRUE) == FR_OK);
    CHECK(strcmp(events, "RL") == 0);
    CHECK(emosEduState.selectedAdapter == EMOS_ADAPTER_FAKE);
    CHECK(emosEduState.preparedMode == EMOS_MODE_LEGACY);
    CHECK(emosEduState.active == FALSE);
    CHECK(emosEduState.lastStatus == FR_OK);
    CHECK(routeOwned == FALSE);

    eventLength = 0;
    events[0] = 0;
    CHECK(emos_select_fake(FALSE) == FR_OK);
    CHECK(strcmp(events, "") == 0);
    CHECK(emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED);
    CHECK(emosEduState.lastStatus == FR_OK);
    CHECK(routeOwned == FALSE);
    return 0;
}

static int failedRecoveryCannotSwitchAdapter(void) {
    resetCoordinator();
    routeOwned = TRUE;
    scriptedLeave = 0xe2;
    CHECK(emos_select_fake(TRUE) == EMOS_UNAVAILABLE);
    CHECK(strcmp(events, "RL") == 0);
    CHECK(emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED);
    CHECK(emosEduState.lastStatus == 0xe2);
    CHECK(routeOwned == TRUE);
    return 0;
}

static int publicResultMappingPreservesOnlyThePublicDomain(void) {
    int result;
    for (result = FR_OK; result <= EMOS_REGISTRY_FULL; result++)
        CHECK(emos_adapter_public_result(result) == result);
    CHECK(emos_adapter_public_result(-1) == EMOS_UNAVAILABLE);
    CHECK(emos_adapter_public_result(EMOS_REGISTRY_FULL + 1) == EMOS_UNAVAILABLE);
    CHECK(emos_adapter_public_result(0xe0) == EMOS_UNAVAILABLE);
    CHECK(emos_adapter_public_result(0xe8) == EMOS_UNAVAILABLE);
    return 0;
}

static int applicationRequests(void) {
    resetCoordinator();
    emosPolicy = 3; /* ordinary MOS application, no extension header */
    emos_console_owned = consoleAvailable = 1;
    CHECK(emos_review_command("excom", "") == EMOS_BUSY);
    CHECK(emos_review_command("excom", "--unknown") == FR_INVALID_PARAMETER);
    CHECK(!emos_console_keep && emosModeState.mode == EMOS_MODE_LEGACY);
    CHECK(emos_review_command("ExCoM", "--KeEp-DisPlay") == FR_OK);
    CHECK(emosModeState.mode == EMOS_MODE_EXCLUSIVE_COMPAT && !emos_console_keep);
    emosBusy = TRUE;
    CHECK(emos_review_command("legacy", "--keep-display") == EMOS_BUSY);
    CHECK(emosModeState.mode == EMOS_MODE_EXCLUSIVE_COMPAT && !emos_console_keep);
    emosBusy = FALSE;
    CHECK(emos_review_command("LEGACY", "--KEEP-DISPLAY") == FR_OK);
    CHECK(emosModeState.mode == EMOS_MODE_LEGACY && !emos_console_keep);
    consoleAvailable = 0;
    CHECK(emos_review_command("excom", "--keep-display") == EMOS_UNAVAILABLE);
    CHECK(emosModeState.mode == EMOS_MODE_LEGACY && !emos_console_keep);
    CHECK(emos_review_command("excom", "") == EMOS_BUSY);
    emos_console_keep = 1;
    CHECK(emos_request_mode(EMOS_MODE_EXCLUSIVE_EXTENDED) == EMOS_BUSY);
    CHECK(emos_request_mode(EMOS_MODE_DUAL) == EMOS_BUSY);
    resetCoordinator();
    return 0;
}

int main(void) {
    if (applicationRequests()) return 1;
    if (happyPath()) return 1;
    if (failedPrepareCleansWithoutInventingAReleaseFailure()) return 1;
    if (failedEnterCleansPreownedRouteAndRetainsCause()) return 1;
    if (recoveryFailureRetainsOwnershipAndLegacyRetryReleases()) return 1;
    if (successfulFakeSwitchRecoversFixedRouteAndRestoresDefault()) return 1;
    if (failedRecoveryCannotSwitchAdapter()) return 1;
    if (publicResultMappingPreservesOnlyThePublicDomain()) return 1;
    puts("EMOS mode coordinator host checks passed");
    return 0;
}
'''


def exact_mode_translation_unit() -> str:
    source = SOURCE.read_text(encoding="utf-8")
    state_start = source.index("#define EMOS_VDU_ONBOARD")
    state_end = source.index("static UINT16 emos_read16", state_start)
    mode_start = source.index("BYTE emos_get_mode(void)")
    mode_end = source.index("static const char *emos_mode_name", mode_start)
    command_start = source.index('    if (strcasecmp(operation, "excom") == 0')
    command_end = source.index('\n\tif (strcasecmp(operation, "mode")', command_start)
    command = ('int emos_review_command(char *operation, char *args) { int result;\n' +
               source[command_start:command_end] + '\nreturn FR_INVALID_PARAMETER; }\n')
    return PREAMBLE + source[state_start:state_end] + source[mode_start:mode_end] + command + HARNESS


class EmosModeTransactionTests(unittest.TestCase):
    def test_exact_mode_coordinator_transaction_and_recovery(self) -> None:
        compiler = shutil.which("cc")
        self.assertIsNotNone(compiler, "host C compiler is required")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            translation_unit = root / "emos-mode-transaction.c"
            executable = root / "emos-mode-transaction"
            translation_unit.write_text(
                exact_mode_translation_unit(), encoding="utf-8", newline="\n"
            )
            subprocess.run(
                [
                    compiler,
                    "-std=c17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-pedantic",
                    str(translation_unit),
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
        self.assertTrue(result.stdout.endswith("EMOS mode coordinator host checks passed\n"))
        self.assertEqual(result.stderr, "")


if __name__ == "__main__":
    unittest.main()
