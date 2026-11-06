#!/usr/bin/env python3
"""Bind failure-only capture and shared Deadline initialization to linked bytes.

These checks inspect the actual native image; they do not claim electrical
UART timing or execution of an interrupt on a host CPU.
"""
import argparse
from pathlib import Path
from verify_abi import symbols


def verify(image, linked, enabled):
    def addr(name, offset=0):
        return (linked[name]+offset).to_bytes(3, 'little')

    def exact(name, expected):
        start=linked[name]
        if image[start:start+len(expected)]!=expected:
            raise ValueError('control TX linked mismatch: '+name)

    exact('_emos_keyboard_deadline_start',
          b'\x21\x03\x00\x00\x39\xed\x27\x3a'+addr('_clock')+
          b'\x77\x23\xaf\x77\x23\x77\x23\x77\x23\x77\x23\x36\x04\xc9')
    if ('_emos_tx_fault' in linked)!=enabled:
        raise ValueError('diagnostic selection disagrees with linked symbols')
    if not enabled:
        if '_emos_tx_probe' in linked or 'keyboard_block_record' in linked:
            raise ValueError('default image contains diagnostic storage/code')
        return
    if linked['_emos_tx_fault']-linked['_emos_tx_probe']!=3:
        raise ValueError('diagnostic scratch layout changed')
    start=linked['keyboard_block_record']
    done=linked['keyboard_block_record_done']
    exact('keyboard_block_record',
          b'\xf5\x3a'+addr('_emos_tx_fault')+b'\xb7\x20'+
          bytes([(done-(start+8))&255])+
          b'\xed\x43'+addr('_emos_tx_fault',4)+
          b'\x22'+addr('_emos_tx_fault',13)+b'\xc5\xd5\xe5\xdd\x27\x00\x22'+
          addr('_emos_tx_fault',10)+b'\x21\x1b\x00\x00\x39\xed\x27\x22'+
          addr('_emos_tx_fault',1)+b'\x21'+addr('_emos_tx_probe')+
          b'\x11'+addr('_emos_tx_fault',7)+b'\x01\x03\x00\x00\xed\xb0'+
          b'\xe1\xd1\xc1\xf1\x32'+addr('_emos_tx_fault')+b'\xc9\xf1\xc9')
    exact('keyboard_block_pc_sample',
          b'\x32'+addr('_emos_tx_probe',1)+b'\xf5\x3e\x03\x32'+
          addr('_emos_tx_probe',2)+b'\xf1\xc9')
    exact('keyboard_block_error',
          b'\x3e\x02\xcd'+addr('keyboard_block_record')+
          b'\xed\x38\x9e\xf6\x04\xed\x39\x9e\xaf\xed\x39\xd1\xfb\x3e\x02')
    exact('keyboard_block_fail', b'\x3e\x01\xcd'+addr('keyboard_block_record')+b'\xaf')
    # Native branch meanings: masked LSR must be exactly THRE and CTS must
    # be low before THR is written. No diagnostic path adds a peripheral read.
    body=image[linked['_emos_keyboard_transmit_block']:linked['_emos_keyboard_deadline_start']]
    reads={port:body.count(b'\xed\x38'+bytes([port])) for port in (0xd5,0x9e)}
    if reads!={0xd5:1,0x9e:3}:
        raise ValueError('diagnostic changed existing acknowledging/status reads')
    for pattern in (b'\xe6\xbe\xfe\x20', b'\xe6\x08', b'\xed\x39\xd0'):
        if pattern not in body:raise ValueError('UART transmit guard/write missing')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('source','elf','nm'):p.add_argument('--'+name,required=True,type=Path)
    p.add_argument('--objdump',type=Path)
    p.add_argument('--enabled',action='store_true')
    a=p.parse_args()
    verify(a.elf.with_suffix('.bin').read_bytes(),symbols(a.nm,a.elf),a.enabled)
    print('Shared Deadline layout and selected failure-only TX capture linked checks passed')


if __name__=='__main__':main()
