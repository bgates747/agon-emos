        .assume ADL = 1

        .section .entry,"ax",@progbits
        .global _emos_provider_entry

/* Generic transient star-command provider. All output remains ordinary VDU
 * traffic and therefore passes through EMOS Core's fixed RST 10 dispatcher. */
_emos_provider_entry:
        ld a,'H'
        rst.lil 0x10
        ld a,'e'
        rst.lil 0x10
        ld a,'l'
        rst.lil 0x10
        ld a,'l'
        rst.lil 0x10
        ld a,'o'
        rst.lil 0x10
        ld a,13
        rst.lil 0x10
        ld a,10
        rst.lil 0x10
        ld hl,0
        ret
