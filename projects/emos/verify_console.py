#!/usr/bin/env python3
"""Bind the compatible UART dispatch bridges to the actual linked eZ80 bytes."""
import argparse
from verify_abi import symbols
from pathlib import Path


def verify(image, linked):
    def address(name):
        return linked[name].to_bytes(3, 'little')

    def exact(name, expected):
        start = linked[name]
        if image[start:start+len(expected)] != expected:
            raise ValueError('Console ABI mismatch: '+name)

    exact('_emos_mainboard_put', b'\x21\x03\x00\x00\x39\x7e\xc3'+address('UART0_serial_PUTCH'))
    exact('_emos_vdp_effect', b'\x21\x03\x00\x00\x39\x7e\xd6\x80\xc3'+address('emos_vdp_dispatch'))
    exact('EMOS_vdu_console', b'\xf1\xc3'+address('EMOS_vdu_console_PUTCH'))
    start=linked['EMOS_vdu_console_PUTCH']
    failed=linked['EMOS_vdu_console_PUTCH_failed']
    prologue=b'\xc5\xd5\xe5\xdd\xe5\xfd\xe5\xf5\x21\x00\x00\x00\x6f\xe5\xcd'+address('_emos_console_write_byte')
    restore=b'\xf1\xfd\xe1\xdd\xe1\xe1\xd1\xc1'
    branch_at=start+len(prologue)+2
    exact('EMOS_vdu_console_PUTCH', prologue+b'\xe1\xb7\x20'+bytes([(failed-(branch_at+2))&255])+restore+b'\x37\xc9')
    exact('EMOS_vdu_console_PUTCH_failed', restore+b'\xb7\xc9')
    exact('EMOS_vdu_console_WRITE',
          b'\xd5\xdd\xe5\xfd\xe5\xe5\xc5\xc5\xe5\xcd'+address('_emos_console_write_stream')+
          b'\xd1\xd1\xd1\xe1\x01\x00\x00\x00\x42\x4b\x09\x01\x00\x00\x00\xfd\xe1\xdd\xe1\xd1\xb7\xc0\x37\xc9')
    # Existing profile checks prove each dispatcher reads the backend exactly
    # once. Bind its new route to these verified entry points as well.
    put=image[linked['EMOS_vdu_PUTCH']:linked['EMOS_vdu_console']]
    if b'\xfe\x01\x28' not in put:
        raise ValueError('Compatible byte route is missing')
    write=image[linked['EMOS_vdu_WRITE']:linked['EMOS_vdu_WRITE_onboard']]
    if b'\xfe\x01\xca'+address('EMOS_vdu_console_WRITE') not in write:
        raise ValueError('Compatible stream route is missing')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('source','elf','nm'): p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--objdump',type=Path)
    a=p.parse_args()
    verify(a.elf.with_suffix('.bin').read_bytes(),symbols(a.nm,a.elf))
    print('EMOS compatible UART linked dispatch/register and stock-reply bridges verified')


if __name__=='__main__': main()
