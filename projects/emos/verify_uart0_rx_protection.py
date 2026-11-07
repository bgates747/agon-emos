#!/usr/bin/env python3
"""Exact linked private UART0 RX-reserve guard; no executed CPU/IRQ claim."""
import argparse
from pathlib import Path
from verify_abi import symbols
from verify_parallel import disassemble_global_extent,direct_transfer_count,instructions
p=argparse.ArgumentParser(description=__doc__)
for key in ('source','elf','nm','objdump'):p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();s=symbols(a.nm,a.elf);image=a.elf.with_suffix('.bin').read_bytes()
def addr(n):return s[n].to_bytes(3,'little')
def call(n):return b'\xcd'+addr(n)
def exact(n,data):
 at=s[n];assert image[at:at+len(data)]==data,n
for n in ('_emos_neta_hook','_emos_neta_uart0_enter','_emos_neta_uart0_exit','_emos_neta_uart0_rx','_emos_neta_uart0_hold','_emos_neta_rx_deferred','_emos_neta_rx_ready'):
 assert n in s,n
for n in ('_emos_neta_uart0_hold','_emos_neta_rx_deferred'):assert s[n]>=0xb0000,n
entry=(bytes.fromhex('3a')+(s['_emos_neta_hook']+2).to_bytes(3,'little')+
 bytes.fromhex('b7 c8 3a')+addr('_uart1_keyboard_owned')+bytes.fromhex('fe01 c0 3a')+
 addr('_emos_key_faulted')+bytes.fromhex('b7 c0 3e01 32')+addr('_emos_neta_uart0_hold')+
 bytes.fromhex('ed389e f604 ed399e c9'))
exact('_emos_neta_uart0_enter',entry)
# Original AF is saved before the A-only early guard, and no large save precedes it.
exact('_uart0_handler',bytes.fromhex('f3 f5')+call('_emos_neta_uart0_enter'))
saved=bytes.fromhex('c5 d5 e5 dde5 fde5 08 f5 d9 c5 d5 e5')
restored=bytes.fromhex('e1 d1 c1 d9 f1 08 fde1 dde1 e1 d1 c1 c9')
tail=(bytes.fromhex('3a')+addr('_emos_neta_uart0_hold')+bytes.fromhex('b7 c8')+saved+
 call('_emos_neta_uart0_rx')+bytes.fromhex('b7 2808 ed389e e6fb ed399e')+restored)
exact('_emos_neta_uart0_exit',tail)
# No nested EI in either guard. Every original caller still restores AF/EI/RETI.
for n,data in (('_emos_neta_uart0_enter',entry),('_emos_neta_uart0_exit',tail)):
 rows=disassemble_global_extent(a.objdump,a.elf,n)
 assert not any(op in ('ei','di') for _,op,_ in instructions(rows))
 raw=image[s[n]:s[n]+len(data)];assert bytes.fromhex('ed38d5') not in raw,'New acknowledging LSR read'
rx=disassemble_global_extent(a.objdump,a.elf,'_uart0_handler')
assert direct_transfer_count(rx,s['_emos_neta_uart0_enter'])==1
assert direct_transfer_count(rx,s['_emos_neta_uart0_exit'])==1
raw=disassemble_global_extent(a.objdump,a.elf,'fill_uart0_tx_fifo')
assert direct_transfer_count(raw,s['_emos_neta_uart0_exit'])==3,'Missing full/final/CTS-defer tail'
body=image[s['fill_uart0_tx_fifo']:s['uart0_async_retry']]
assert body.count(call('_emos_neta_uart0_exit')+bytes.fromhex('f1 fb 5bed4d'))==3
for caller,callee in (('_emos_neta_uart0_rx','_uart1_keyboard_irq'),('_emos_neta_uart0_rx','_emos_neta_rx_ready'),('_uart1_keyboard_irq_done','_emos_neta_rx_ready')):
 assert direct_transfer_count(disassemble_global_extent(a.objdump,a.elf,caller),s[callee])==1,(caller,callee)
# Cap and clean-empty use the existing drain, never an added throwaway status read.
cap=image[s['keyboard_irq_cap_done']-24:s['keyboard_irq_complete']]
empty=image[s['keyboard_irq_complete']:s['keyboard_irq_complete']+8]
assert bytes.fromhex('3e01 32')+addr('_emos_neta_rx_deferred') in cap
assert bytes.fromhex('af 32')+addr('_emos_neta_rx_deferred') in empty
source=(a.source/'src/emos_neta.c').read_text()
assert 'demand||emos_neta_rx_deferred' in source,'Deferred RX omitted from idle/tick'
assert 'live=demand!=0||emos_neta_rx_deferred' in source,'Abort would hide unfinished RX'
# Checker negative controls independently reject mutations to scope/STOP/full saves.
for name,offset in (('missing-early-hold',23),('missing-tail-save',6),('unconditional-READY',len(tail)-len(restored)-9)):
 n='_emos_neta_uart0_enter' if name=='missing-early-hold' else '_emos_neta_uart0_exit'
 expected=entry if n.endswith('enter') else tail
 original=image;mutated=bytearray(image);mutated[s[n]+offset]^=1;image=bytes(mutated)
 try:exact(n,expected)
 except AssertionError:pass
 else:raise AssertionError('Guard mutation escaped checker: '+name)
 finally:image=original
print('Linked earlySTOP/guard-owned RX-first/full-context/all UART0 exits/deferred-cap closure PASS; timing and IRQ execution unqualified')
