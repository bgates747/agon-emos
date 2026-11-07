#!/usr/bin/env python3
"""INTEG-024 linked optional owner glue, retaining every existing validator."""
import argparse
from pathlib import Path
from verify_abi import symbols
from verify_parallel import disassemble_global_extent,direct_transfer_count
p=argparse.ArgumentParser()
for key in ('source','elf','nm','objdump'):p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();s=symbols(a.nm,a.elf)
for n in ('_emos_neta_gateway','_emos_neta_service','_emos_neta_tick','_emos_neta_abort','_emos_neta_cts','_emos_keyboard_app_reserve','_emos_keyboard_app_release'):
 assert n in s,n
for n in ('_u1_start','_u1_step','_u1_emit_uart1','_agon_neta_service'):
 assert n not in s,'Application engine leaked into resident firmware: '+n
for caller,callee in (('_emos_keyboard_tick','_emos_neta_tick'),('_emos_net_reset','_emos_neta_abort'),('_emos_neta_progress','_uart1_keyboard_irq')):
 body=disassemble_global_extent(a.objdump,a.elf,caller)
 assert direct_transfer_count(body,s[callee])==1,(caller,callee)
image=a.elf.with_suffix('.bin').read_bytes()
assert len(image)<=131072
# CTS helper has one owned PC_DR read and no waiting loop or interrupt enable.
assert image[s['_emos_neta_cts']:s['_emos_neta_cts']+6]==bytes.fromhex('ed38 9e e608 c9')
print('App-only engine absent; linked owner tick/reset/RX-first and bounded CTS helper PASS')
