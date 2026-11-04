#!/usr/bin/env python3
"""Explicit ordinary ExExt composition: common ABI/pad/boot and owner checks.

The obsolete indefinite epoch engine is excluded only here. Native payload,
UART divisor, complete stock keyboard callback ABI and dispatch checks remain.
This is linked-byte verification, not a bench or throughput result.
"""
import argparse
from pathlib import Path
from verify_parallel_native import verify_native
from verify_parallel import (ParallelError, linked_symbols, direct_transfer_count,
                             disassemble_global_extent)
from verify_abi import symbols

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('source','elf','nm','objdump'):p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args()
    verify_native(a.source,a.elf,a.nm,a.objdump,stream=True)
    s=symbols(a.nm,a.elf);image=a.elf.with_suffix('.bin').read_bytes()
    def exact(name,data):
        if image[s[name]:s[name]+len(data)]!=data:raise ParallelError('ExExt bridge changed: '+name)
    exact('_emos_exext_interrupt_service', b'\xfb\xcd'+s['_emos_exext_service'].to_bytes(3,'little')+b'\xf3\xc9')
    for owner,callee in (('_emos_keyboard_tick','_emos_exext_tick'),
                         ('_emos_exext_tick','_emos_exext_interrupt_service'),
                         ('_emos_exext_interrupt_service','_emos_exext_service')):
        if direct_transfer_count(disassemble_global_extent(a.objdump,a.elf,owner),s[callee])!=1:
            raise ParallelError('ExExt service lost sole owner: '+owner)
    if '_emos_parallel_probe' in s or '_emos_parallel_engine_write' in s:
        raise ParallelError('ExExt includes a diagnostic or obsolete epoch endpoint')
    if len(image)>131072:raise ParallelError('ExExt exceeds physical ROM')
    print('EMOS ordinary ExExt linked owners, native guard, pad inventory and real VBlank bridge verified')
if __name__=='__main__':main()
