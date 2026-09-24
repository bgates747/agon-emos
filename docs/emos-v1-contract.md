# EMOS resident service contract

Current maintained EMOS v0.1.19 behavior. This file keeps its stable path;
[foreground EMOS utilities](emos-utilities.md) owns disk-utility invocation.
This is a product-local ABI, not an upstream MOS module standard.

## Ownership and supported entry points

EMOS is one complete MOS firmware. It owns ordinary VDU routing, committed
mode, keyboard admission, canonical MOS sysvars and Extender UART transport.
Applications use the resident interfaces; they do not take UART/GPIO ownership.
Cold boot uses Legacy and mainboard keyboard selection. Startup may explicitly
select Extender input; changing display mode does not select a keyboard source.

MOS API `0x51` and C-function slot `0x20` reach the resident gateway. It accepts
service operation 2 and resolves compiled services. There is no external `.emo`
discovery, registry, provider execution or swap-file facility. `EMOS DISCOVER`
and `EMOS CLEAR` are reserved commands returning unavailable. An unknown valid
service identity returns `EMOS_NOT_FOUND` with output length zero.

`EMOS EXCOM` and `EMOS LEGACY` request the named display route. Ordinary mode
changes are idle/Core operations; a live application may request the explicit
`--keep-display` variant at complete VDU/query boundaries. EMOS validates and
commits the transition; an error does not authorize drawing on the requested
backend. The [cross-component console contract](https://github.com/bgates747/agon-extender/blob/main/docs/protocols/excom-console.md)
owns the paired handshake. Other mode identities and fake/qualification adapters
in source are not proof of available production hardware modes.

## Gateway request

The 66-byte `t_emosGatewayRequest` in [emos.h](../src/emos.h) uses byte-array
integers to avoid compiler padding. All multibyte fields are little endian.

| Offset | Bytes | Meaning |
|---|---|---|
| 0 | 2 | Size: 66 |
| 2 | 1 | ABI major: 1 |
| 3 | 1 | ABI minor: 0 |
| 4 | 1 | Namespace length: 1..16 |
| 5 | 1 | Service-name length: 1..24 |
| 6 | 2 | Operation: 2 (service) |
| 8 | 2 | Flags: zero |
| 10 | 3 | Input pointer |
| 13 | 3 | Input byte length |
| 16 | 3 | Output pointer |
| 19 | 3 | Output capacity |
| 22 | 3 | Returned output length |
| 25 | 16 | Zero-padded namespace |
| 41 | 24 | Zero-padded service name |
| 65 | 1 | Reserved: zero |

Names begin with lowercase ASCII a–z; remaining characters are lowercase
letters, digits, dot, underscore or hyphen. The dispatcher validates size,
version, operation, flags, names and padding before service dispatch. Buffer
admission is service-specific. A request overlapping MOSlet memory must be
wholly contained there and is admitted only for resident `ext.sdlink`; do not
infer that other resident services accept MOSlet callers. The gateway rejects
reentrancy through its busy state. See [emos.c](../src/emos.c) for dispatch and
[mos_api.asm](../src/mos_api.asm) for target bindings.

## Resident foreground SD transport service

`ext.sdlink` carries bounded messages between a foreground application/EMOSlet
and P4. Complete request/input/output ranges must lie in `0x040000..0x0B7FFF`.
Admission requires Legacy, healthy `EMOS KEYINPUT extender`, EMOS ownership of
UART1 and foreground calls with interrupts enabled. No caller pointer is kept
by the receive ISR.

| First input byte | Operation | Remaining input / output |
|---|---|---|
| 0 | OPEN | No remaining input or output; clears pending mailbox |
| 1 | RECEIVE | No remaining input; output capacity at least 240; returns one complete record or outputLength 0 |
| 2 | SEND | One 20..240-byte record; no output buffer |
| 3 | CLOSE | No remaining input or output; clears admission and mailbox |

The resident buffers are a 240-byte receive mailbox and a 244-byte transmit
buffer. Application entry/exit, explicit close and transport/input faults revoke
admission. EMOS receives private `8D` records and sends private `F6` envelopes;
the [SD wire contract](https://github.com/bgates747/agon-extender/blob/main/docs/protocols/mainboard-sd.md)
owns framing, CRC and host operations. The foreground listener performs file
I/O and record validation; the ISR does not run a filesystem server. P4's
console owner multiplexes complete service packets with keyboard traffic.

[Listener source/build guide](../projects/sdserve/README.md) and
[operator guide](https://github.com/bgates747/agon-extender/blob/main/docs/mainboard-sd.md)
own invocation, checked/fast semantics, sessions and recovery.

## Resident text qualification service

`edu.text-probe` is a bounded diagnostic, not ordinary application printf.
Request and input must lie wholly in ordinary application RAM
`0x040000..0x0AFFFF`; input length is 1..1024. Output pointer, capacity and length
must be zero. EMOS requires Legacy and an idle resident dispatcher, validates
the complete text command grammar, and owns UART setup/exchange/cleanup.

The accepted grammar is printable ASCII, VDU 8..13, 30 and 31,x,y. It does not
change ordinary VDU routing, committed mode or canonical VDP sysvars. The
legacy-named VDPTEXT diagnostic command has a separate fixed exchange.
Source: [emos_uart_probe.c](../src/emos_uart_probe.c). Profile-gated telemetry
and transport probes are diagnostic contracts, not universally available APIs.

## Limits and validation

These interfaces coordinate trusted eZ80 software; they do not isolate arbitrary
code that writes registers or memory directly. Return codes and bounds do not
prove that a caller-linked utility respects its runtime memory limit. The
[utility contract](emos-utilities.md) owns that distinct executable boundary.

[AUDIT-008](tasks/AUDIT-008.md) records the current resident/utility validation
and remaining acceptance gates. A test receipt applies to its identified build,
not any later compile. The superseded module design remains in Git history at
`895715e:docs/emos-v1-contract.md`; it is not part of this current contract.
