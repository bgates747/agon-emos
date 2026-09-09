/*
 * EMOS v1 Core module registry and transient dispatcher.
 *
 * The local ABI is replaceable and must not be described as accepted upstream
 * MOS policy. Discovery is explicit and transactional. External code is never
 * resident, reentrant, interrupt-owned, or exposed as a public pointer.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "emos.h"
#include "emos_uart_probe.h"
#include "emos_keyboard.h"
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
#include "umm_malloc.h"

#define EMOS_HEADER_CRC_OFFSET 0x4C
#define EMOS_SWAP_PATH "/.emos-swap.bin"

typedef struct {
	BYTE providerClass;
	BYTE namespaceLength;
	BYTE nameLength;
	BYTE version[3];
	BYTE coreMin;
	BYTE coreMax;
	BYTE flags;
	UINT24 imageSize;
	UINT24 entryOffset;
	UINT16 requestMin;
	UINT16 requestMax;
	UINT32 payloadCrc;
	UINT32 headerCrc;
	char namespaceName[EMOS_NAMESPACE_SIZE + 1];
	char providerName[EMOS_NAME_SIZE + 1];
	char path[EMOS_PATH_SIZE];
} t_emosRegistryEntry;

typedef struct {
	BYTE count;
	BYTE generation;
	t_emosRegistryEntry entries[EMOS_MAX_PROVIDERS];
} t_emosRegistry;

typedef UINT24 (*t_emosProviderEntry)(t_emosProviderRequest *request);

static t_emosRegistry emosRegistry;
static BOOL emosBusy = FALSE;
static BOOL emosRecoveryRequired = FALSE;
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

static UINT24 emos_read24(const BYTE *ptr) {
	return (UINT24)ptr[0] | ((UINT24)ptr[1] << 8) | ((UINT24)ptr[2] << 16);
}

static UINT32 emos_read32(const BYTE *ptr) {
	return (UINT32)ptr[0] | ((UINT32)ptr[1] << 8) |
		((UINT32)ptr[2] << 16) | ((UINT32)ptr[3] << 24);
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

static UINT32 emos_crc32_update(UINT32 crc, const BYTE *data, UINT24 length) {
	UINT24 i;
	BYTE bit;
	for (i = 0; i < length; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++) {
			crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
		}
	}
	return crc;
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

static int emos_entry_compare(const t_emosRegistryEntry *left, const t_emosRegistryEntry *right) {
	int result = left->providerClass - right->providerClass;
	if (result == 0) result = strcmp(left->namespaceName, right->namespaceName);
	if (result == 0) result = strcmp(left->providerName, right->providerName);
	return result;
}

static BOOL emos_entry_same_image(const t_emosRegistryEntry *left, const t_emosRegistryEntry *right) {
	return emos_entry_compare(left, right) == 0 &&
		left->imageSize == right->imageSize &&
		left->entryOffset == right->entryOffset &&
		left->coreMin == right->coreMin && left->coreMax == right->coreMax &&
		left->flags == right->flags &&
		left->requestMin == right->requestMin && left->requestMax == right->requestMax &&
		left->payloadCrc == right->payloadCrc && left->headerCrc == right->headerCrc &&
		memcmp(left->version, right->version, sizeof(left->version)) == 0;
}

static BOOL emos_has_extension(const char *name) {
	UINT24 length = strlen(name);
	return length > 4 && name[length - 4] == '.' &&
		tolower(name[length - 3]) == 'e' &&
		tolower(name[length - 2]) == 'm' &&
		tolower(name[length - 1]) == 'o';
}

static int emos_validate_header(BYTE *header, UINT24 fileSize, t_emosRegistryEntry *entry) {
	UINT32 expectedHeaderCrc;
	UINT32 actualHeaderCrc;
	BYTE namespaceLength;
	BYTE nameLength;
	BYTE savedCrc[4];

	if (memcmp(header, "EMOD", 4) != 0) return EMOS_INVALID_MODULE;
	if (header[4] != EMOS_FORMAT_MAJOR || header[5] > EMOS_FORMAT_MINOR)
		return EMOS_INCOMPATIBLE;
	if (emos_read16(header + 6) != EMOS_HEADER_SIZE ||
		emos_read24(header + 8) != fileSize || fileSize > EMOS_MODULE_SIZE)
		return EMOS_INVALID_MODULE;
	entry->imageSize = fileSize;
	entry->entryOffset = emos_read24(header + 11);
	if (entry->entryOffset < EMOS_HEADER_SIZE || entry->entryOffset >= fileSize)
		return EMOS_INVALID_MODULE;
	entry->coreMin = header[14];
	entry->coreMax = header[15];
	if (entry->coreMin > EMOS_CORE_ABI || entry->coreMax < EMOS_CORE_ABI)
		return EMOS_INCOMPATIBLE;
	entry->providerClass = header[16];
	if (entry->providerClass != EMOS_PROVIDER_STAR &&
		entry->providerClass != EMOS_PROVIDER_SERVICE) return EMOS_INVALID_MODULE;
	entry->flags = header[17];
	if (entry->flags != 0 || emos_read32(header + 64) != 0)
		return EMOS_INCOMPATIBLE;
	namespaceLength = header[18];
	nameLength = header[19];
	if (!emos_identity_valid(header + 20, namespaceLength, EMOS_NAMESPACE_SIZE, TRUE) ||
		!emos_identity_valid(header + 36, nameLength, EMOS_NAME_SIZE, FALSE) ||
		!emos_zero(header + 20 + namespaceLength, EMOS_NAMESPACE_SIZE - namespaceLength) ||
		!emos_zero(header + 36 + nameLength, EMOS_NAME_SIZE - nameLength))
		return EMOS_INVALID_MODULE;
	if ((entry->providerClass == EMOS_PROVIDER_STAR && namespaceLength != 0) ||
		(entry->providerClass == EMOS_PROVIDER_SERVICE && namespaceLength == 0))
		return EMOS_INVALID_MODULE;
	entry->requestMin = emos_read16(header + 68);
	entry->requestMax = emos_read16(header + 70);
	if (entry->requestMin > EMOS_PROVIDER_REQUEST_SIZE ||
		entry->requestMax < EMOS_PROVIDER_REQUEST_SIZE ||
		!emos_zero(header + 63, 1) || !emos_zero(header + 80, 48))
		return EMOS_INVALID_MODULE;
	entry->payloadCrc = emos_read32(header + 72);
	entry->namespaceLength = namespaceLength;
	entry->nameLength = nameLength;
	memcpy(entry->namespaceName, header + 20, namespaceLength);
	entry->namespaceName[namespaceLength] = 0;
	memcpy(entry->providerName, header + 36, nameLength);
	entry->providerName[nameLength] = 0;
	memcpy(entry->version, header + 60, 3);
	expectedHeaderCrc = emos_read32(header + EMOS_HEADER_CRC_OFFSET);
	entry->headerCrc = expectedHeaderCrc;
	memcpy(savedCrc, header + EMOS_HEADER_CRC_OFFSET, 4);
	memset(header + EMOS_HEADER_CRC_OFFSET, 0, 4);
	actualHeaderCrc = emos_crc32_update(0xFFFFFFFFUL, header, EMOS_HEADER_SIZE) ^ 0xFFFFFFFFUL;
	memcpy(header + EMOS_HEADER_CRC_OFFSET, savedCrc, 4);
	if (actualHeaderCrc != expectedHeaderCrc) return EMOS_INVALID_MODULE;
	return FR_OK;
}

static int emos_validate_file(const char *path, t_emosRegistryEntry *entry) {
	FIL file;
	int result;
	BYTE header[EMOS_HEADER_SIZE];
	BYTE buffer[128];
	UINT bytesRead;
	UINT32 crc = 0xFFFFFFFFUL;
	UINT32 expectedPayloadCrc;
	UINT24 remaining;

	result = f_open(&file, path, FA_READ);
	if (result != FR_OK) return result;
	if (f_size(&file) < EMOS_HEADER_SIZE || f_size(&file) > EMOS_MODULE_SIZE) {
		f_close(&file);
		return EMOS_INVALID_MODULE;
	}
	result = f_read(&file, header, EMOS_HEADER_SIZE, &bytesRead);
	if (result != FR_OK || bytesRead != EMOS_HEADER_SIZE) {
		f_close(&file);
		return result == FR_OK ? EMOS_INVALID_MODULE : result;
	}
	expectedPayloadCrc = emos_read32(header + 72);
	result = emos_validate_header(header, f_size(&file), entry);
	if (result != FR_OK) {
		f_close(&file);
		return result;
	}
	remaining = entry->imageSize - EMOS_HEADER_SIZE;
	while (remaining && result == FR_OK) {
		UINT amount = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
		result = f_read(&file, buffer, amount, &bytesRead);
		if (result == FR_OK && bytesRead != amount) result = FR_INT_ERR;
		if (result == FR_OK) crc = emos_crc32_update(crc, buffer, amount);
		remaining -= amount;
	}
	f_close(&file);
	if (result != FR_OK) return result;
	if ((crc ^ 0xFFFFFFFFUL) != expectedPayloadCrc) return EMOS_INVALID_MODULE;
	return FR_OK;
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
	memset(&emosRegistry, 0, sizeof(emosRegistry));
	emosBusy = FALSE;
	emosRecoveryRequired = FALSE;
	emosPolicy = EMOS_POLICY_CORE;
	emosModeState.mode = EMOS_MODE_LEGACY;
	emosModeState.vduRoute = EMOS_VDU_ONBOARD;
	emosModeState.eduState = EMOS_EDU_INACTIVE;
	emosModeState.generation = 0;
	memset(&emosEduState, 0, sizeof(emosEduState));
	emosEduState.selectedAdapter = emos_default_adapter();
	emosVduBackend = EMOS_VDU_ONBOARD;
	/* No application survives a cold boot, so an interrupted prior session's
	 * private preservation file cannot describe live module-area ownership. */
	f_unlink(EMOS_SWAP_PATH);
	/* main calls this after onboard VDP startup and SD mount. Expose the same
	 * product/build identity as EMOS STATUS without requiring a boot script. */
	emos_print_identity();
}

int emos_discover(void) {
	t_emosRegistry *staging;
	DIR directory;
	FILINFO fileInfo;
	int result;
	char *configuredPath;
	const char *directoryPath;
	BOOL directoryOpened = FALSE;

	if (emosBusy) return EMOS_BUSY;
	configuredPath = expandVariableToken("EMOS$Path");
	directoryPath = configuredPath ? configuredPath : "/emos/modules";
	if (!directoryPath[0]) {
		umm_free(configuredPath);
		return FR_INVALID_PARAMETER;
	}
	staging = umm_malloc(sizeof(t_emosRegistry));
	if (!staging) {
		umm_free(configuredPath);
		return MOS_OUT_OF_MEMORY;
	}
	memset(staging, 0, sizeof(t_emosRegistry));
	staging->generation = emosRegistry.generation + 1;
	result = f_opendir(&directory, directoryPath);
	if (result == FR_OK) directoryOpened = TRUE;
	while (result == FR_OK) {
		UINT24 pathLength;
		BYTE insertAt;
		t_emosRegistryEntry candidate;
		result = f_readdir(&directory, &fileInfo);
		if (result != FR_OK || fileInfo.fname[0] == 0) break;
		if ((fileInfo.fattrib & AM_DIR) || !emos_has_extension(fileInfo.fname)) continue;
		if (staging->count >= EMOS_MAX_PROVIDERS) {
			result = EMOS_REGISTRY_FULL;
			break;
		}
		memset(&candidate, 0, sizeof(candidate));
		pathLength = strlen(directoryPath) + strlen(fileInfo.fname) + 2;
		if (pathLength > EMOS_PATH_SIZE) {
			result = EMOS_INVALID_MODULE;
			break;
		}
		sprintf(candidate.path, "%s%s%s", directoryPath,
			directoryPath[strlen(directoryPath) - 1] == '/' ? "" : "/", fileInfo.fname);
		result = emos_validate_file(candidate.path, &candidate);
		if (result != FR_OK) break;
		insertAt = staging->count;
		while (insertAt > 0 && emos_entry_compare(&candidate, &staging->entries[insertAt - 1]) < 0) {
			staging->entries[insertAt] = staging->entries[insertAt - 1];
			insertAt--;
		}
		staging->entries[insertAt] = candidate;
		staging->count++;
	}
	if (directoryOpened) f_closedir(&directory);
	if (result == FR_OK) {
		BYTE index;
		for (index = 1; index < staging->count; index++) {
			if (emos_entry_compare(&staging->entries[index - 1], &staging->entries[index]) == 0) {
				result = EMOS_CONFLICT;
				break;
			}
		}
	}
	if (result == FR_OK) emosRegistry = *staging;
	umm_free(staging);
	umm_free(configuredPath);
	return result;
}

int emos_clear(void) {
	BYTE generation;
	if (emosBusy) return EMOS_BUSY;
	generation = emosRegistry.generation + 1;
	memset(&emosRegistry, 0, sizeof(emosRegistry));
	emosRegistry.generation = generation;
	return FR_OK;
}

static t_emosRegistryEntry *emos_find(BYTE providerClass, const char *namespaceName, const char *name) {
	BYTE index;
	for (index = 0; index < emosRegistry.count; index++) {
		t_emosRegistryEntry *entry = &emosRegistry.entries[index];
		if (entry->providerClass == providerClass &&
			strcmp(entry->namespaceName, namespaceName) == 0 &&
			strcmp(entry->providerName, name) == 0) return entry;
	}
	return NULL;
}

static BOOL emos_range_overlaps_module(UINT24 address, UINT24 length) {
	UINT24 end;
	if (length == 0) return FALSE;
	end = address + length;
	if (end < address) return TRUE;
	return address < EMOS_MODULE_BASE + EMOS_MODULE_SIZE && end > EMOS_MODULE_BASE;
}

static void emos_scrub_module_area(void) {
	memset((void *)EMOS_MODULE_BASE, 0, EMOS_MODULE_SIZE);
}

static int emos_preserve_module_area(BOOL restore) {
	FIL file;
	int result;
	int closeResult;
	UINT bytes;
	UINT24 offset = 0;
	BYTE mode = restore ? FA_READ : (FA_WRITE | FA_CREATE_ALWAYS);
	result = f_open(&file, EMOS_SWAP_PATH, mode);
	if (result != FR_OK) return result;
	if (restore && f_size(&file) != EMOS_MODULE_SIZE) result = FR_INT_ERR;
	while (offset < EMOS_MODULE_SIZE && result == FR_OK) {
		UINT amount = EMOS_MODULE_SIZE - offset > 256 ? 256 : EMOS_MODULE_SIZE - offset;
		if (restore) result = f_read(&file, (void *)(EMOS_MODULE_BASE + offset), amount, &bytes);
		else result = f_write(&file, (void *)(EMOS_MODULE_BASE + offset), amount, &bytes);
		if (result == FR_OK && bytes != amount) result = FR_INT_ERR;
		offset += amount;
	}
	if (!restore && result == FR_OK) result = f_sync(&file);
	closeResult = f_close(&file);
	if (result == FR_OK) result = closeResult;
	if (!restore && result != FR_OK) f_unlink(EMOS_SWAP_PATH);
	return result;
}

static int emos_invoke(t_emosRegistryEntry *entry, t_emosProviderRequest *request) {
	FIL file;
	int result;
	UINT bytesRead;
	BOOL preserved = FALSE;
	BOOL fileOpened = FALSE;
	t_emosProviderEntry provider;
	t_emosProviderRequest requestSnapshot;

	if (!entry) return EMOS_NOT_FOUND;
	if (emosBusy) return EMOS_BUSY;
	if (emos_range_overlaps_module(emos_read24(request->input), emos_read24(request->inputLength)) ||
		emos_range_overlaps_module(emos_read24(request->output), emos_read24(request->outputCapacity)))
		return EMOS_UNSAFE_CALLER;
	if (emosPolicy == EMOS_POLICY_UNSAFE || emosPolicy == EMOS_POLICY_MOSLET)
		return EMOS_UNSAFE_CALLER;
	if (emosRecoveryRequired) {
		result = emos_preserve_module_area(TRUE);
		if (result == FR_OK) {
			f_unlink(EMOS_SWAP_PATH);
			emosRecoveryRequired = FALSE;
		}
		/* The interrupted request is never replayed implicitly. */
		return EMOS_RECOVERY_FAILED;
	}
	memcpy(&requestSnapshot, request, sizeof(requestSnapshot));
	emosBusy = TRUE;
	if (emosPolicy == EMOS_POLICY_COMPATIBLE) {
		result = emos_preserve_module_area(FALSE);
		if (result != FR_OK) {
			emosBusy = FALSE;
			return result;
		}
		preserved = TRUE;
	}
	result = f_open(&file, entry->path, FA_READ);
	if (result == FR_OK) fileOpened = TRUE;
	if (result == FR_OK) result = f_read(&file, (void *)EMOS_MODULE_BASE, entry->imageSize, &bytesRead);
	if (result == FR_OK && bytesRead != entry->imageSize) result = EMOS_INVALID_MODULE;
	if (fileOpened) f_close(&file);
	if (result == FR_OK) {
		t_emosRegistryEntry loaded;
		BYTE *header = (BYTE *)EMOS_MODULE_BASE;
		UINT32 expectedPayloadCrc = emos_read32(header + 72);
		UINT32 actualPayloadCrc;
		result = emos_validate_header(header, entry->imageSize, &loaded);
		actualPayloadCrc = emos_crc32_update(0xFFFFFFFFUL,
			header + EMOS_HEADER_SIZE, entry->imageSize - EMOS_HEADER_SIZE) ^ 0xFFFFFFFFUL;
		if (result == FR_OK && actualPayloadCrc != expectedPayloadCrc) result = EMOS_INVALID_MODULE;
		if (result == FR_OK && !emos_entry_same_image(entry, &loaded)) result = EMOS_INVALID_MODULE;
		if (result == FR_OK) {
			provider = (t_emosProviderEntry)(EMOS_MODULE_BASE + entry->entryOffset);
			result = provider(request);
			if (memcmp(request, &requestSnapshot, 20) != 0 ||
				request->reserved != requestSnapshot.reserved ||
				emos_read24(request->outputLength) > emos_read24(request->outputCapacity)) {
				emos_write24(request->outputLength, 0);
				result = EMOS_PROVIDER_FAILED;
			} else if (result > EMOS_REGISTRY_FULL) {
				result = EMOS_PROVIDER_FAILED;
			}
		}
	}
	if (preserved) {
		int restoreResult = emos_preserve_module_area(TRUE);
		if (restoreResult == FR_OK) f_unlink(EMOS_SWAP_PATH);
		else {
			emosRecoveryRequired = TRUE;
			result = EMOS_RECOVERY_FAILED;
		}
	} else emos_scrub_module_area();
	emosBusy = FALSE;
	return result;
}

UINT24 emos_gateway(t_emosGatewayRequest *request) {
	t_emosProviderRequest providerRequest;
	t_emosRegistryEntry *entry;
	char namespaceName[EMOS_NAMESPACE_SIZE + 1];
	char providerName[EMOS_NAME_SIZE + 1];
	UINT24 result;

	if (!request || emos_range_overlaps_module((UINT24)request, sizeof(*request)))
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
     * change or application-owned transport. Reserved ahead of discovery.
     * The request and input must be wholly in ordinary application RAM;
     * static application buffers meet this even when MOS owns the stack. */
    if (strcmp(namespaceName, "edu") == 0 && strcmp(providerName, "text-probe") == 0) {
        UINT24 address = emos_read24(request->input);
        UINT24 length = emos_read24(request->inputLength);
        if ((UINT24)request < 0x040000 || (UINT24)request > 0x0B0000 - sizeof(*request) ||
            !length || length > EMOS_TEXT_LIMIT || address < 0x040000 ||
            address >= 0x0B0000 || length > 0x0B0000 - address ||
            !emos_zero(request->output, 9)) return FR_INVALID_PARAMETER;
        if (emosBusy || emosRecoveryRequired) return EMOS_BUSY;
        if (emosModeState.mode != EMOS_MODE_LEGACY) return EMOS_UNAVAILABLE;
        if (!emos_text_valid((const BYTE *)address, (UINT16)length)) return FR_INVALID_PARAMETER;
        emosBusy = TRUE;
        result = emos_text_probe((const BYTE *)address, (UINT16)length) ? FR_OK : FR_TIMEOUT;
        emosBusy = FALSE;
        return result;
    }
	entry = emos_find(EMOS_PROVIDER_SERVICE, namespaceName, providerName);
	memset(&providerRequest, 0, sizeof(providerRequest));
	emos_write16(providerRequest.size, EMOS_PROVIDER_REQUEST_SIZE);
	providerRequest.abiMajor = EMOS_CORE_ABI;
	emos_write16(providerRequest.operation, emos_read16(request->operation));
	memcpy(providerRequest.input, request->input, 12);
	result = emos_invoke(entry, &providerRequest);
	memcpy(request->outputLength, providerRequest.outputLength, 3);
	return result;
}

int emos_dispatch_command(char *command, char *args, BOOL *matched) {
	t_emosProviderRequest request;
	char canonical[EMOS_NAME_SIZE + 1];
	UINT24 commandLength = strlen(command);
	UINT24 index;
	t_emosRegistryEntry *entry;
	if (commandLength > EMOS_NAME_SIZE) {
		if (matched) *matched = FALSE;
		return EMOS_NOT_FOUND;
	}
	for (index = 0; index < commandLength; index++)
		canonical[index] = tolower((unsigned char)command[index]);
	canonical[commandLength] = 0;
	entry = emos_find(EMOS_PROVIDER_STAR, "", canonical);
	if (matched) *matched = entry != NULL;
	if (!entry) return EMOS_NOT_FOUND;
	memset(&request, 0, sizeof(request));
	emos_write16(request.size, EMOS_PROVIDER_REQUEST_SIZE);
	request.abiMajor = EMOS_CORE_ABI;
	emos_write16(request.operation, EMOS_OPERATION_STAR);
	emos_write24(request.input, (UINT24)args);
	emos_write24(request.inputLength, args ? strlen(args) + 1 : 0);
	return emos_invoke(entry, &request);
}

BYTE emos_application_enter(UINT8 *image, UINT24 address) {
	BYTE previous = emosPolicy;
	BYTE flags;
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
	/* Once the application exits there is no live owner to recover. Retain a
	 * failed-restore image while a nested caller returns to an outer live
	 * application; only leaving the top-level application discards it. */
	if (emosRecoveryRequired && previousPolicy == EMOS_POLICY_CORE) {
		f_unlink(EMOS_SWAP_PATH);
		emosRecoveryRequired = FALSE;
		emos_scrub_module_area();
	}
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
	if (emosEduState.selectedAdapter == EMOS_ADAPTER_FAKE) {
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
	emosEduState.active = TRUE;
	emosEduState.generation++;
	emosEduState.lastStatus = FR_OK;
	return FR_OK;
}

static int emos_adapter_recover(void) {
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
	if (emosBusy) return EMOS_BUSY;
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
        if (!strcasecmp(source, "extender")) {
            printf("Extender keyboard input is not available\r\n");
            return EMOS_UNAVAILABLE;
        }
        if (!strcasecmp(source, "browser")) selected = EMOS_KEY_BROWSER;
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
        emos_key_source == EMOS_KEY_BROWSER ? "browser" : "mainboard",
        emos_key_faulted ? " (fault)" : "");
    return status;
usage:
    printf("Usage: EMOS KEYINPUT [mainboard|browser|extender]\r\n");
    return FR_INVALID_PARAMETER;
}

int emos_cmd(char *args) {
	char *operation;
	int result = extractString(args, &args, NULL, &operation, EXTRACT_FLAG_AUTO_TERMINATE);
	if (result == FR_INVALID_PARAMETER) {
		emos_print_identity();
		printf("EMOS v1: %s, registry %d, generation %d\r\n",
			emos_mode_name(emosModeState.mode), emosRegistry.count, emosRegistry.generation);
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
        return emos_uart_flow() ? FR_OK : FR_TIMEOUT;
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
	if (strcasecmp(operation, "discover") == 0) {
		result = emos_discover();
		if (result == FR_OK) printf("EMOS: discovered %d provider(s)\r\n", emosRegistry.count);
		return result;
	}
	if (strcasecmp(operation, "clear") == 0) return emos_clear();
	if (strcasecmp(operation, "status") == 0) {
		BYTE index;
		emos_print_identity();
		printf("EMOS v1: %s, registry %d, generation %d\r\n",
			emos_mode_name(emosModeState.mode), emosRegistry.count, emosRegistry.generation);
		printf("  VDU route %d, EDU %s, adapter %s, mode generation %d\r\n",
			emosModeState.vduRoute, emosModeState.eduState ? "active" : "inactive",
			emos_adapter_name(emosEduState.selectedAdapter),
			emosModeState.generation);
		for (index = 0; index < emosRegistry.count; index++) {
			t_emosRegistryEntry *entry = &emosRegistry.entries[index];
			printf("  %s%s%s %d.%d.%d %s\r\n",
				entry->namespaceName, entry->namespaceLength ? "." : "",
				entry->providerName, entry->version[0], entry->version[1],
				entry->version[2], entry->path);
		}
		return FR_OK;
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
	return FR_INVALID_PARAMETER;
}
