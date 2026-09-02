/*
 * EMOS v1 Core module and service contract.
 *
 * This is a provisional, versioned EMOS ABI. It is deliberately isolated from
 * the still-unimplemented upstream MOS Modules proposal. External providers
 * are synchronous transient images; Core MOS never exposes their pointers.
 */

#ifndef EMOS_H
#define EMOS_H

#include "defines.h"

#define EMOS_FORMAT_MAJOR            1
#define EMOS_FORMAT_MINOR            0
#define EMOS_CORE_ABI                1
#define EMOS_HEADER_SIZE             128
#define EMOS_MODULE_BASE             0x0B0000
#define EMOS_MODULE_SIZE             0x008000
#define EMOS_PROVIDER_REQUEST_SIZE   24
#define EMOS_GATEWAY_REQUEST_SIZE    66
#define EMOS_PROVIDER_STAR           1
#define EMOS_PROVIDER_SERVICE        2
#define EMOS_MAX_PROVIDERS           16
#define EMOS_NAMESPACE_SIZE          16
#define EMOS_NAME_SIZE               24
#define EMOS_PATH_SIZE               128

#define EMOS_OPERATION_STAR          1
#define EMOS_OPERATION_SERVICE       2

#define EMOS_MODE_LEGACY             0
#define EMOS_MODE_DUAL               1
#define EMOS_MODE_EXCLUSIVE_COMPAT   2
#define EMOS_MODE_EXCLUSIVE_EXTENDED 3

#define EMOS_POLICY_CORE             0
#define EMOS_POLICY_SAFE             1
#define EMOS_POLICY_COMPATIBLE       2
#define EMOS_POLICY_UNSAFE           3
#define EMOS_POLICY_MOSLET           4

/* Byte-array fields make this request exactly 24 bytes on both maintained
 * compilers without depending on structure packing or UINT24 alignment. */
typedef struct {
	BYTE size[2];
	BYTE abiMajor;
	BYTE abiMinor;
	BYTE operation[2];
	BYTE flags[2];
	BYTE input[3];
	BYTE inputLength[3];
	BYTE output[3];
	BYTE outputCapacity[3];
	BYTE outputLength[3];
	BYTE reserved;
} t_emosProviderRequest;

/* Public gateway request. Providers never see this structure or retain its
 * pointers. Names are canonical lowercase ASCII and zero padded. */
typedef struct {
	BYTE size[2];
	BYTE abiMajor;
	BYTE abiMinor;
	BYTE namespaceLength;
	BYTE nameLength;
	BYTE operation[2];
	BYTE flags[2];
	BYTE input[3];
	BYTE inputLength[3];
	BYTE output[3];
	BYTE outputCapacity[3];
	BYTE outputLength[3];
	BYTE namespaceName[EMOS_NAMESPACE_SIZE];
	BYTE providerName[EMOS_NAME_SIZE];
	BYTE reserved;
} t_emosGatewayRequest;

/* Fail compilation if either maintained compiler inserts unexpected padding.
 * The host generator tests the same numeric constants and byte layout. */
typedef char t_emosProviderRequestSizeCheck[
	sizeof(t_emosProviderRequest) == EMOS_PROVIDER_REQUEST_SIZE ? 1 : -1];
typedef char t_emosGatewayRequestSizeCheck[
	sizeof(t_emosGatewayRequest) == EMOS_GATEWAY_REQUEST_SIZE ? 1 : -1];

void emos_init(void);
int emos_discover(void);
int emos_clear(void);
int emos_dispatch_command(char *command, char *args, BOOL *matched);
UINT24 emos_gateway(t_emosGatewayRequest *request);
int emos_cmd(char *args);
BYTE emos_application_enter(UINT8 *image, UINT24 address);
void emos_application_leave(BYTE previousPolicy);
BYTE emos_get_mode(void);
int emos_request_mode(BYTE mode);

#endif /* EMOS_H */
