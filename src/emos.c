/*
 * Resident EMOS services, mode ownership and foreground MOSlet dispatch.
 * AUDIT-008 retires the cancelled external .emo registry/loader. The public
 * gateway remains resident; disk utilities use the ordinary stock MOSlet ABI.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "emos.h"
#include "emos_uart_probe.h"
#include "emos_keyboard.h"
#include "emos_sdlink.h"
#include "emos_telemetry.h"
#include "emos_console.h"
#include "uart.h"
#include "emos_uart_flow.h"
#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
#include "emos_parallel.h"
#endif

/*
 * Product/build identity is supplied by the selected maintained-source build
 * profile. Keep fail-closed defaults so an exploratory or misconfigured build
 * announces that it is not deployment authority instead of impersonating a
 * reviewed candidate.
 */
#ifndef EMOS_SOURCE_IDENTITY
#define EMOS_SOURCE_IDENTITY "UNVERSIONED-DO-NOT-DEPLOY"
#endif
#ifndef EMOS_BUILD_ID
#define EMOS_BUILD_ID "UNVERSIONED-DO-NOT-DEPLOY"
#endif
#ifndef EMOS_ARTIFACT_STATUS
#define EMOS_ARTIFACT_STATUS "UNVERSIONED-DO-NOT-DEPLOY"
#endif
#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
#ifndef EMOS_QUALIFICATION_COMPOSITION_IDENTITY
#error "The fixed qualification profile requires its separate composition identity"
#endif
#endif
#include "ff.h"
#include "mos.h"
#include "mos_sysvars.h"

static BOOL emosBusy = FALSE;
static BYTE emosPolicy = EMOS_POLICY_CORE;
volatile BYTE emosVduBackend = 0;

#define EMOS_VDU_ONBOARD 0
#define EMOS_VDU_EDP_COMPAT 1
#define EMOS_VDU_EDP_EXTENDED 2
#define EMOS_EDU_INACTIVE 0
#define EMOS_EDU_ACTIVE 1
#define EMOS_ADAPTER_UNAVAILABLE 0
#define EMOS_ADAPTER_FAKE 1
#define EMOS_ADAPTER_PARALLEL_FIXED 2
#define EMOS_ADAPTER_UART_CONSOLE 3

typedef struct {
	BYTE mode;
	BYTE vduRoute;
	BYTE eduState;
	BYTE generation;
} t_emosModeState;

/* This result domain is Core-owned but deliberately not a stock VDP sysvar.
 * A later qualified resident EDP adapter replaces these narrow seams. */
typedef struct {
	BYTE selectedAdapter;
	BYTE preparedMode;
	BYTE active;
	BYTE generation;
	BYTE lastStatus;
} t_emosEduState;

static t_emosModeState emosModeState;
static t_emosEduState emosEduState;

static BYTE emos_default_adapter(void) {
#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
	return EMOS_ADAPTER_PARALLEL_FIXED;
#else
	return EMOS_ADAPTER_UNAVAILABLE;
#endif
}

static UINT16 emos_read16(const BYTE *ptr) {
	return (UINT16)ptr[0] | ((UINT16)ptr[1] << 8);
}

UINT24 emos_read24(const BYTE *ptr) {
#if defined(__ez80__)
    /* eZ80 ADL is little endian and supports unaligned 24-bit loads. memcpy
     * states the aliasing contract without three shift/or sequences. Keep the
     * byte-wise path for ZDS/host compilers without this target guarantee. */
    UINT24 value;
    __builtin_memcpy(&value,ptr,3);
    return value;
#else
	return (UINT24)ptr[0] | ((UINT24)ptr[1] << 8) | ((UINT24)ptr[2] << 16);
#endif
}

static void emos_write16(BYTE *ptr, UINT16 value) {
	ptr[0] = value & 0xFF;
	ptr[1] = value >> 8;
}

static void emos_write24(BYTE *ptr, UINT24 value) {
	ptr[0] = value & 0xFF;
	ptr[1] = (value >> 8) & 0xFF;
	ptr[2] = (value >> 16) & 0xFF;
}

static BOOL emos_zero(const BYTE *data, UINT24 length) {
	while (length--) {
		if (*data++) return FALSE;
	}
	return TRUE;
}

static BOOL emos_identity_valid(const BYTE *value, BYTE length, BYTE maximum, BOOL emptyAllowed) {
	BYTE i;
	if (length == 0) return emptyAllowed;
	if (length > maximum || value[0] < 'a' || value[0] > 'z') return FALSE;
	for (i = 1; i < length; i++) {
		BYTE ch = value[i];
		if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
			ch == '.' || ch == '_' || ch == '-')) return FALSE;
	}
	return TRUE;
}

static void emos_print_identity(void) {
	printf("EMOS identity: %s, build %s, status %s\r\n",
		EMOS_SOURCE_IDENTITY, EMOS_BUILD_ID, EMOS_ARTIFACT_STATUS);
	#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
	printf("EMOS qualification composition: %s (non-release)\r\n",
		EMOS_QUALIFICATION_COMPOSITION_IDENTITY);
	#endif
}

void emos_init(void) {
	emosBusy = FALSE;
	emosPolicy = EMOS_POLICY_CORE;
	emosModeState.mode = EMOS_MODE_LEGACY;
	emosModeState.vduRoute = EMOS_VDU_ONBOARD;
	emosModeState.eduState = EMOS_EDU_INACTIVE;
	emosModeState.generation = 0;
	memset(&emosEduState, 0, sizeof(emosEduState));
	emosEduState.selectedAdapter = emos_default_adapter();
	emosVduBackend = EMOS_VDU_ONBOARD;
	/* main calls this after onboard VDP startup and SD mount. Expose the same
	 * product/build identity as EMOS STATUS without requiring a boot script. */
	emos_print_identity();
}

static BOOL emos_range_overlaps_module(UINT24 address, UINT24 length) {
	UINT24 end;
	if (length == 0) return FALSE;
	end = address + length;
	if (end < address) return TRUE;
	return address < EMOS_MODULE_BASE + EMOS_MODULE_SIZE && end > EMOS_MODULE_BASE;
}

UINT24 emos_gateway(t_emosGatewayRequest *request) {
	char namespaceName[EMOS_NAMESPACE_SIZE + 1];
	char providerName[EMOS_NAME_SIZE + 1];
	UINT24 result;

    /* REMOTE-005: resident sdlink never loads/scrubs MOSlet memory. Other
     * resident services retain their MOSlet-space exclusion after validation. */
    BYTE mosletRequest = emos_range_overlaps_module((UINT24)request, sizeof(*request));
    if (!request || (mosletRequest && ((UINT24)request < EMOS_MODULE_BASE ||
        (UINT24)request > EMOS_MODULE_BASE + EMOS_MODULE_SIZE - sizeof(*request))))
        return EMOS_UNSAFE_CALLER;
	if (emos_read16(request->size) != EMOS_GATEWAY_REQUEST_SIZE ||
		request->abiMajor != EMOS_CORE_ABI || request->abiMinor != 0 ||
		emos_read16(request->operation) != EMOS_OPERATION_SERVICE ||
		request->namespaceLength == 0 || request->namespaceLength > EMOS_NAMESPACE_SIZE ||
		request->nameLength == 0 || request->nameLength > EMOS_NAME_SIZE ||
		request->reserved != 0 || emos_read16(request->flags) != 0)
		return FR_INVALID_PARAMETER;
	memcpy(namespaceName, request->namespaceName, request->namespaceLength);
	namespaceName[request->namespaceLength] = 0;
	memcpy(providerName, request->providerName, request->nameLength);
	providerName[request->nameLength] = 0;
	if (!emos_identity_valid((BYTE *)namespaceName, request->namespaceLength, EMOS_NAMESPACE_SIZE, TRUE) ||
		!emos_identity_valid((BYTE *)providerName, request->nameLength, EMOS_NAME_SIZE, FALSE) ||
		!emos_zero(request->namespaceName + request->namespaceLength, EMOS_NAMESPACE_SIZE - request->namespaceLength) ||
		!emos_zero(request->providerName + request->nameLength, EMOS_NAME_SIZE - request->nameLength))
		return FR_INVALID_PARAMETER;
    /* Qualification-only resident service: no module load, public VDU route
     * change or application-owned transport. Reserved resident identity.
     * The request and input must be wholly in ordinary application RAM;
     * static application buffers meet this even when MOS owns the stack. */
    if (mosletRequest && (strcmp(namespaceName,"ext") || strcmp(providerName,"sdlink")))
        return EMOS_UNSAFE_CALLER;
    if (strcmp(namespaceName, "edu") == 0 && strcmp(providerName, "text-probe") == 0) {
        UINT24 address = emos_read24(request->input);
        UINT24 length = emos_read24(request->inputLength);
        if ((UINT24)request < 0x040000 || (UINT24)request > 0x0B0000 - sizeof(*request) ||
            !length || length > EMOS_TEXT_LIMIT || address < 0x040000 ||
            address >= 0x0B0000 || length > 0x0B0000 - address ||
            !emos_zero(request->output, 9)) return FR_INVALID_PARAMETER;
        if (emosBusy) return EMOS_BUSY;
        if (emosModeState.mode != EMOS_MODE_LEGACY) return EMOS_UNAVAILABLE;
        if (!emos_text_valid((const BYTE *)address, (UINT16)length)) return FR_INVALID_PARAMETER;
        emosBusy = TRUE;
        result = emos_text_probe((const BYTE *)address, (UINT16)length) ? FR_OK : FR_TIMEOUT;
        emosBusy = FALSE;
        return result;
    }
    if (strcmp(namespaceName,"ext") == 0) {
#ifdef EMOS_BENCH_TELEMETRY
        BYTE telemetry = strcmp(providerName,"telemetry") == 0;
#else
        BYTE telemetry = 0;
#endif
        if (telemetry || strcmp(providerName,"sdlink") == 0) {
            if (emosBusy) return EMOS_BUSY;
            emosBusy = TRUE;
#ifdef EMOS_BENCH_TELEMETRY
            result = telemetry ? emos_telemetry_gateway(request) : emos_sdlink_gateway(request);
#else
            result = emos_sdlink_gateway(request);
#endif
            emosBusy = FALSE;
            return result;
        }
    }
	/* No external providers: preserve the gateway error domain. */
	return EMOS_NOT_FOUND;
}

BYTE emos_application_enter(UINT8 *image, UINT24 address) {
	BYTE previous = emosPolicy;
	BYTE flags;
    emos_sdlink_reset();
#ifdef EMOS_BENCH_TELEMETRY
    emos_telemetry_reset();
#endif
	if (address == EMOS_MODULE_BASE) {
		emosPolicy = EMOS_POLICY_MOSLET;
		return previous;
	}
	emosPolicy = EMOS_POLICY_UNSAFE;
	if (image[0x40] != 'M' || image[0x41] != 'O' || image[0x42] != 'S' ||
		image[0x43] != 1 || image[0x44] != 1) return previous;
	flags = image[0x45];
	if ((BYTE)~flags != image[0x46] || (flags & 0xFC) || (flags & 3) == 3)
		return previous;
	if (emos_read24(image + 0x47) != 0 && emos_read24(image + 0x47) != address)
		return previous;
	if (flags & 1) emosPolicy = EMOS_POLICY_SAFE;
	else if (flags & 2) emosPolicy = EMOS_POLICY_COMPATIBLE;
	return previous;
}

void emos_application_leave(BYTE previousPolicy) {
    emos_sdlink_reset();
#ifdef EMOS_BENCH_TELEMETRY
    emos_telemetry_reset();
#endif
	emosPolicy = previousPolicy;
}

BYTE emos_get_mode(void) {
	return emosModeState.mode;
}

static void emos_mode_shape(BYTE mode, t_emosModeState *state) {
	state->mode = mode;
	state->eduState = mode == EMOS_MODE_LEGACY ? EMOS_EDU_INACTIVE : EMOS_EDU_ACTIVE;
	if (mode == EMOS_MODE_EXCLUSIVE_COMPAT) state->vduRoute = EMOS_VDU_EDP_COMPAT;
	else if (mode == EMOS_MODE_EXCLUSIVE_EXTENDED) state->vduRoute = EMOS_VDU_EDP_EXTENDED;
	else state->vduRoute = EMOS_VDU_ONBOARD;
}

static int emos_adapter_prepare(BYTE mode) {
	emosEduState.lastStatus = EMOS_UNAVAILABLE;
    if (mode == EMOS_MODE_EXCLUSIVE_COMPAT &&
        emosEduState.selectedAdapter != EMOS_ADAPTER_FAKE) {
        #ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
        /* A failed parallel cleanup can still own its lease in logical Legacy.
         * Never hide that owner by replacing its adapter selection. */
        if (emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED &&
            emos_parallel_fixed_ready()) return EMOS_BUSY;
        #endif
        emosEduState.selectedAdapter = EMOS_ADAPTER_UART_CONSOLE;
        if (!emos_console_prepare()) return EMOS_UNAVAILABLE;
    }
	else if (emosEduState.selectedAdapter == EMOS_ADAPTER_FAKE) {
		if (mode != EMOS_MODE_DUAL) return EMOS_UNAVAILABLE;
	}
	#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
	else if (emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED) {
		/* The qualification procedure requires its operator/top-level to prepare
		 * the peer's active parallel epoch first.  This explicit mode request is
		 * the assertion that preparation is complete; EMOS does not receive a
		 * separate attestation signal.  The request still owns route commitment,
		 * and no activation or precommit General Poll is emitted here. */
		if (mode != EMOS_MODE_EXCLUSIVE_EXTENDED) return EMOS_UNAVAILABLE;
		emosEduState.lastStatus = emos_parallel_fixed_enter(TRUE);
		if (emosEduState.lastStatus != FR_OK) return emosEduState.lastStatus;
	}
#endif
	else return EMOS_UNAVAILABLE;
	emosEduState.preparedMode = mode;
	emosEduState.lastStatus = FR_OK;
	return FR_OK;
}

static int emos_adapter_ready(BYTE mode) {
	if (emosEduState.preparedMode != mode) return EMOS_UNAVAILABLE;
    if (emosEduState.selectedAdapter == EMOS_ADAPTER_UART_CONSOLE) {
        return mode == EMOS_MODE_EXCLUSIVE_COMPAT && emos_console_owned ? FR_OK : EMOS_UNAVAILABLE;
    }
	if (emosEduState.selectedAdapter == EMOS_ADAPTER_FAKE)
		return mode == EMOS_MODE_DUAL ? FR_OK : EMOS_UNAVAILABLE;
	#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
	if (emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED)
		return mode == EMOS_MODE_EXCLUSIVE_EXTENDED &&
			emos_parallel_fixed_ready() ? FR_OK : EMOS_UNAVAILABLE;
#endif
	return EMOS_UNAVAILABLE;
}

static int emos_adapter_commit(BYTE mode) {
	if (emos_adapter_ready(mode) != FR_OK) return EMOS_UNAVAILABLE;
    if (emosEduState.selectedAdapter == EMOS_ADAPTER_UART_CONSOLE && !emos_console_commit()) {
        return EMOS_UNAVAILABLE;
    }
	emosEduState.active = TRUE;
	emosEduState.generation++;
	emosEduState.lastStatus = FR_OK;
	return FR_OK;
}

static int emos_adapter_recover(void) {
    if (emosEduState.selectedAdapter == EMOS_ADAPTER_UART_CONSOLE && !emos_console_recover()) {
        return EMOS_UNAVAILABLE;
    }
	/* A failed fixed-profile leave retains its live route lease and adapter
	 * state.  The mode coordinator must not publish Legacy while a writer owns
	 * the epoch or while deterministic electrical release has not completed. */
	#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
	if (emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED &&
		emos_parallel_fixed_ready()) {
		int result = emos_parallel_fixed_leave();
		if (result != FR_OK) {
			emosEduState.lastStatus = result;
			return result;
		}
	}
#endif
	emosEduState.preparedMode = EMOS_MODE_LEGACY;
	emosEduState.active = FALSE;
	emosEduState.lastStatus = FR_OK;
	return FR_OK;
}

static int emos_adapter_public_result(int result) {
	/* Parallel lifecycle statuses are private 0xE0..0xE8 diagnostics.  MOS
	 * commands accept only the FatFS/MOS 0..36 result domain; lastStatus retains
	 * the exact private failure while the public caller gets a printable error. */
	return result >= FR_OK && result <= EMOS_REGISTRY_FULL ?
		result : EMOS_UNAVAILABLE;
}

static int emos_select_fake(BOOL enabled) {
	int result;
	if (emosBusy) return EMOS_BUSY;
	if (emosModeState.mode != EMOS_MODE_LEGACY) return EMOS_UNAVAILABLE;
	result = emos_adapter_recover();
	if (result != FR_OK) return emos_adapter_public_result(result);
	emosEduState.selectedAdapter = enabled ? EMOS_ADAPTER_FAKE : emos_default_adapter();
	return FR_OK;
}

int emos_request_mode(BYTE mode) {
	t_emosModeState prepared = emosModeState;
	int result;
	if (mode > EMOS_MODE_EXCLUSIVE_EXTENDED) return FR_INVALID_PARAMETER;
	/* INTEG-011: an application may explicitly retain the two displays at a
     * complete VDU boundary. Default/mixed-mode transitions remain CLI-only. */
    if (emosBusy || (emosPolicy != EMOS_POLICY_CORE && (!emos_console_keep ||
        (mode != EMOS_MODE_LEGACY && mode != EMOS_MODE_EXCLUSIVE_COMPAT)))) return EMOS_BUSY;
	if (mode == emosModeState.mode) {
		#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
		/* A failed entry cleanup can retain the fixed route while the published
		 * logical mode is still Legacy.  A repeated Legacy request is the
		 * deterministic recovery retry instead of a false no-op success. */
		if (mode == EMOS_MODE_LEGACY &&
			emosEduState.selectedAdapter == EMOS_ADAPTER_PARALLEL_FIXED &&
			emos_parallel_fixed_ready()) {
			result = emos_adapter_recover();
			return emos_adapter_public_result(result);
		}
		#endif
		return FR_OK;
	}
	if (mode != EMOS_MODE_LEGACY && emosModeState.mode != EMOS_MODE_LEGACY)
		return EMOS_UNAVAILABLE;
	if (mode == EMOS_MODE_LEGACY) {
		result = emos_adapter_recover();
		if (result != FR_OK) return emos_adapter_public_result(result);
		emos_mode_shape(EMOS_MODE_LEGACY, &prepared);
	} else {
		int recoveryResult;
		emos_mode_shape(mode, &prepared);
		result = emos_adapter_prepare(mode);
		if (result == FR_OK) result = emos_adapter_ready(mode);
		if (result == FR_OK) result = emos_adapter_commit(mode);
		if (result != FR_OK) {
			recoveryResult = emos_adapter_recover();
			if (recoveryResult != FR_OK)
				return emos_adapter_public_result(recoveryResult);
			emosEduState.lastStatus = result;
			return emos_adapter_public_result(result);
		}
	}
	prepared.generation = emosModeState.generation + 1;
	/* Publish the complete logical state before the one-byte route commit.
	 * Each dispatcher already snapshots that byte exactly once. */
	emosModeState = prepared;
	emosVduBackend = prepared.vduRoute;
    if (emosEduState.selectedAdapter == EMOS_ADAPTER_UART_CONSOLE) {
        if (mode == EMOS_MODE_EXCLUSIVE_COMPAT) emos_console_publish();
        emos_console_notice(mode == EMOS_MODE_EXCLUSIVE_COMPAT);
    }
	return FR_OK;
}

static const char *emos_mode_name(BYTE mode) {
	switch (mode) {
		case EMOS_MODE_DUAL: return "Dual";
		case EMOS_MODE_EXCLUSIVE_COMPAT: return "Exclusive Compatible";
		case EMOS_MODE_EXCLUSIVE_EXTENDED: return "Exclusive Extended";
		default: return "Legacy";
	}
}

static const char *emos_adapter_name(BYTE adapter) {
    if (adapter == EMOS_ADAPTER_UART_CONSOLE) return "uart-console";
	if (adapter == EMOS_ADAPTER_FAKE) return "fake";
	#ifdef EMOS_PARALLEL_FIXED_QUALIFICATION
	if (adapter == EMOS_ADAPTER_PARALLEL_FIXED)
		return "parallel-fixed-qualification";
#endif
	return "unavailable";
}

static int emos_call_service(char *args) {
	t_emosGatewayRequest request;
	char output[128];
	char namespaceName[EMOS_NAMESPACE_SIZE + 1];
	char providerName[EMOS_NAME_SIZE + 1];
	char *identity;
	char *separator;
	UINT24 namespaceLength;
	UINT24 nameLength;
	UINT24 outputLength;
	UINT24 index;
	int result = extractString(args, &args, NULL, &identity, EXTRACT_FLAG_AUTO_TERMINATE);
	if (result != FR_OK) return result;
	separator = strrchr(identity, '.');
	if (!separator || separator == identity || separator[1] == 0) return FR_INVALID_PARAMETER;
	*separator = 0;
	namespaceLength = strlen(identity);
	nameLength = strlen(separator + 1);
	if (namespaceLength > EMOS_NAMESPACE_SIZE || nameLength > EMOS_NAME_SIZE)
		return FR_INVALID_PARAMETER;
	for (index = 0; index < namespaceLength; index++)
		namespaceName[index] = tolower((unsigned char)identity[index]);
	namespaceName[namespaceLength] = 0;
	for (index = 0; index < nameLength; index++)
		providerName[index] = tolower((unsigned char)separator[1 + index]);
	providerName[nameLength] = 0;
	memset(&request, 0, sizeof(request));
	memset(output, 0, sizeof(output));
	emos_write16(request.size, EMOS_GATEWAY_REQUEST_SIZE);
	request.abiMajor = EMOS_CORE_ABI;
	request.namespaceLength = namespaceLength;
	request.nameLength = nameLength;
	emos_write16(request.operation, EMOS_OPERATION_SERVICE);
	emos_write24(request.input, (UINT24)args);
	emos_write24(request.inputLength, args && args[0] ? strlen(args) + 1 : 0);
	emos_write24(request.output, (UINT24)output);
	emos_write24(request.outputCapacity, sizeof(output));
	memcpy(request.namespaceName, namespaceName, namespaceLength);
	memcpy(request.providerName, providerName, nameLength);
	result = emos_gateway(&request);
	if (result != FR_OK) return result;
	outputLength = emos_read24(request.outputLength);
	if (outputLength >= sizeof(output)) outputLength = sizeof(output) - 1;
	output[outputLength] = 0;
	printf("EMOS service: %s\r\n", output);
	return FR_OK;
}

static int emos_keyinput_command(char *args) {
    char *source;
    int result, status = FR_OK;
    BYTE selected;
    result = extractString(args, &args, NULL, &source, EXTRACT_FLAG_AUTO_TERMINATE);
    if (result == FR_OK) {
        while (args && isspace((unsigned char)*args)) ++args;
        if (args && *args) goto usage;
        if (!strcasecmp(source, "extender")) selected = EMOS_KEY_EXTENDER;
        else if (!strcasecmp(source, "browser")) selected = EMOS_KEY_BROWSER;
        else if (!strcasecmp(source, "mainboard")) selected = EMOS_KEY_MAINBOARD;
        else goto usage;
        result = emos_keyboard_select(selected);
        if (result != EMOS_KEY_OK) {
            status = result == EMOS_KEY_BUSY ? EMOS_BUSY : FR_TIMEOUT;
            printf("KEYINPUT FAIL: %s\r\n", result == EMOS_KEY_BUSY ?
                "UART or keyboard service busy" : "receiver readiness timeout");
        }
    } else if (result != FR_INVALID_PARAMETER) return result;
    printf("Keyboard input: %s%s\r\n",
        emos_key_source == EMOS_KEY_EXTENDER ? "extender" :
        emos_key_source == EMOS_KEY_BROWSER ? "browser" : "mainboard",
        emos_key_faulted ? " (fault)" : "");
    return status;
usage:
    printf("Usage: EMOS KEYINPUT [mainboard|browser|extender]\r\n");
    return FR_INVALID_PARAMETER;
}

/* AUDIT-008: stock mos_runBinFile chooses the load region using Moslet$Path.
 * /emos must not change that global policy or load at the application address.
 * Reuse stock loading/execution with an explicit MOSlet address instead.
 * These are trusted executable files, not a sandbox. An ordinary MOS header
 * cannot prove where a program was linked; install only MOSlets built for B0000.
 */
static int emos_run_utility(const char *name, char *args) {
    char path[6 + EMOS_NAME_SIZE + 4 + 1];
    UINT24 length = strlen(name);
    UINT24 index;
    FILINFO info;
    int result;
    if (emosBusy || emosPolicy != EMOS_POLICY_CORE) return EMOS_BUSY;
    if (!length || length > EMOS_NAME_SIZE) return FR_INVALID_PARAMETER;
    memcpy(path, "/emos/", 6);
    for (index = 0; index < length; ++index) {
        unsigned char ch = name[index];
        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
        if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '-')) return FR_INVALID_PARAMETER;
        path[6 + index] = ch;
    }
    memcpy(path + 6 + length, ".bin", 5);
    result = f_stat(path, &info);
    if (result != FR_OK) return result;
    if ((info.fattrib & AM_DIR) || info.fsize < 0x45)
        return MOS_INVALID_EXECUTABLE;
    if (info.fsize > EMOS_MODULE_SIZE) return MOS_OVERLAPPING_SYSTEM;
    /* Never run an old header after a failed or empty load. No live MOSlet
     * exists at this idle/Core boundary; application memory is left alone.
     * Size preflight prevents stock mos_LOAD's bounded-read truncation from
     * accepting an oversized image. Reuse stock file-I/O error semantics. */
    memset((void *)(EMOS_MODULE_BASE + 0x40), 0, 5);
    result = mos_LOAD(path, EMOS_MODULE_BASE, EMOS_MODULE_SIZE);
    if (result != FR_OK) return result;
    return mos_runBin(EMOS_MODULE_BASE, args);
}

int emos_cmd(char *args) {
	char *operation;
	int result = extractString(args, &args, NULL, &operation, EXTRACT_FLAG_AUTO_TERMINATE);
	if (result == FR_INVALID_PARAMETER) {
		emos_print_identity();
		printf("EMOS: %s\r\n", emos_mode_name(emosModeState.mode));
		return FR_OK;
	}
	if (result != FR_OK) return result;
    if (!strcasecmp(operation, "keyinput")) return emos_keyinput_command(args);
    if (uart1_keyboard_owned && (!strcasecmp(operation, "vdptext") ||
        !strcasecmp(operation, "vdppoll") || !strcasecmp(operation, "uartflow") ||
        !strcasecmp(operation, "uarttest"))) {
        printf("EMOS: UART1 is owned by keyboard input\r\n");
        return EMOS_BUSY;
    }
    if (strcasecmp(operation, "vdptext") == 0) {
        if (args && *args) return FR_INVALID_PARAMETER;
        if (emosModeState.mode != EMOS_MODE_LEGACY) {
            printf("VDP TEXT FAIL: EMOS must be in Legacy\r\n");
            return FR_INVALID_PARAMETER;
        }
        emos_print_identity();
        return emos_visible_text() ? FR_OK : FR_TIMEOUT;
    }
    if (strcasecmp(operation, "vdppoll") == 0) {
        if (args && *args) return FR_INVALID_PARAMETER;
        if (emosModeState.mode != EMOS_MODE_LEGACY) {
            printf("VDP POLL FAIL: EMOS must be in Legacy\r\n");
            return FR_INVALID_PARAMETER;
        }
        emos_print_identity();
        return emos_general_poll() ? FR_OK : FR_TIMEOUT;
    }
    if (strcasecmp(operation, "uartflow") == 0) {
        if (args && *args) return FR_INVALID_PARAMETER;
        if (emosModeState.mode != EMOS_MODE_LEGACY) {
            printf("UART FLOW FAIL: EMOS must be in Legacy\r\n");
            return FR_INVALID_PARAMETER;
        }
        emos_print_identity();
#ifdef EMOS_BENCH_TELEMETRY
        /* BENCH-001 composition trades this old standalone diagnostic for the
         * resident telemetry experiment; full/default EMOS retains UARTFLOW. */
        printf("UARTFLOW is absent from the telemetry bench build\r\n");
        return EMOS_UNAVAILABLE;
#else
        return emos_uart_flow() ? FR_OK : FR_TIMEOUT;
#endif
    }
	if (strcasecmp(operation, "uarttest") == 0) {
        if (args && *args) return FR_INVALID_PARAMETER;
        if (emosModeState.mode != EMOS_MODE_LEGACY) {
            printf("UART ROUND TRIP FAIL: EMOS must be in Legacy\r\n");
            return FR_INVALID_PARAMETER;
        }
        emos_print_identity();
        return emos_uart_probe() ? FR_OK : FR_TIMEOUT;
    }
	/* Retired names stay reserved: never reinterpret an old provider command
     * as an on-disk utility. Historical .emo fixtures are not current gates. */
	if (!strcasecmp(operation, "discover") || !strcasecmp(operation, "clear"))
		return EMOS_UNAVAILABLE;
	if (strcasecmp(operation, "status") == 0) {
		emos_print_identity();
		printf("EMOS: %s\r\n", emos_mode_name(emosModeState.mode));
		printf("  VDU route %d, EDU %s, adapter %s, mode generation %d\r\n",
			emosModeState.vduRoute, emosModeState.eduState ? "active" : "inactive",
			emos_adapter_name(emosEduState.selectedAdapter),
			emosModeState.generation);
		return FR_OK;
	}
    if (strcasecmp(operation, "excom") == 0 || strcasecmp(operation, "legacy") == 0) {
        /* INTEG-011: mos_oscli uses this same coordinator; no application
         * transport bypass. The option belongs to one request, even on error. */
        if (args && *args && strcasecmp(args,"--keep-display")) return FR_INVALID_PARAMETER;
        emos_console_keep = args && *args;
        result = emos_request_mode(strcasecmp(operation,"excom") == 0 ?
            EMOS_MODE_EXCLUSIVE_COMPAT : EMOS_MODE_LEGACY);
        emos_console_keep = 0;
        if (result != FR_OK) printf("EMOS: display switch failed; current route retained\r\n");
        return result;
    }
	if (strcasecmp(operation, "mode") == 0) {
		char *modeName;
		result = extractString(args, &args, NULL, &modeName, EXTRACT_FLAG_AUTO_TERMINATE);
		if (result != FR_OK) return result;
		if (strcasecmp(modeName, "legacy") == 0) return emos_request_mode(EMOS_MODE_LEGACY);
		if (strcasecmp(modeName, "dual") == 0) return emos_request_mode(EMOS_MODE_DUAL);
		if (strcasecmp(modeName, "compatible") == 0) return emos_request_mode(EMOS_MODE_EXCLUSIVE_COMPAT);
		if (strcasecmp(modeName, "extended") == 0) return emos_request_mode(EMOS_MODE_EXCLUSIVE_EXTENDED);
		return FR_INVALID_PARAMETER;
	}
	if (strcasecmp(operation, "fake") == 0) {
		char *setting;
		result = extractString(args, &args, NULL, &setting, EXTRACT_FLAG_AUTO_TERMINATE);
		if (result != FR_OK) return result;
		if (strcasecmp(setting, "on") == 0) return emos_select_fake(TRUE);
		if (strcasecmp(setting, "off") == 0) return emos_select_fake(FALSE);
		return FR_INVALID_PARAMETER;
	}
	if (strcasecmp(operation, "call") == 0) return emos_call_service(args);
	return emos_run_utility(operation, args);
}
