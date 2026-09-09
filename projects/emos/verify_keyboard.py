#!/usr/bin/env python3
"""Bind resident keyboard IRQ, callback ABI and UART guards to the linked bytes.

This verifies maintained assembly bridges, not electrical UART timing or the
later ordinary-input/callback qualification owned by INTEG-009 Work 3.
"""
import argparse
from pathlib import Path
import re

from verify_abi import symbols


def verify(image, linked):
    def address(name):
        if name not in linked:
            raise ValueError('missing keyboard symbol: ' + name)
        return linked[name].to_bytes(3, 'little')

    def exact(name, expected):
        start = linked[name]
        if image[start:start+len(expected)] != expected:
            raise ValueError('keyboard bridge/ABI mismatch: ' + name)

    def call(name):
        return b'\xcd' + address(name)

    # BYTE results use A. In particular a C caller must not receive I or the
    # input operation by mistake when the bridge meant to return true/false.
    exact('_emos_keyboard_lock', b'\xed\x57\xf3\xe2' +
          address('keyboard_lock_disabled') + b'\x3e\x01\xc9')
    exact('keyboard_lock_disabled', b'\xaf\xc9')
    exact('_emos_keyboard_clock', b'\x3a'+address('_clock')+b'\xc9')
    exact('keyboard_vector_success', b'\x3e\x01\xc9')
    exact('keyboard_vector_fail', b'\xaf\xc9')
    exact('keyboard_vector_compare', b'\xb7\xed\x52\x20' +
          bytes([(linked['keyboard_vector_fail']-(linked['keyboard_vector_compare']+5)) & 255]) +
          b'\x78')  # compare's NZ result survives until its rejection branch

    saved = b'\xdd\xe5\xfd\xe5\x08\xf5\xd9\xc5\xd5\xe5'
    restored = b'\xe1\xd1\xc1\xd9\xf1\x08\xfd\xe1\xdd\xe1'
    exact('_emos_keyboard_irq_entry', b'\xf3\xf5\xc5\xd5\xe5' + saved +
          call('_uart1_keyboard_irq') + restored + b'\xe1\xd1\xc1\xf1\xfb\x5b\xed\x4d')
    exact('emos_keyboard_tick_entry', saved + call('_emos_keyboard_tick') + restored + b'\xc9')
    exact('_emos_keyboard_effect', b'\xdd\xe5\xfd\xe5\x21\x09\x00\x00\x39\xed\x17' +
          call('emos_keyboard_payload') + b'\xfd\xe1\xdd\xe1\xc9')
    # The callback receives DE and can modify this UART's payload before the
    # sysvars are written; its pointer is held on the stack, not UART0 scratch.
    exact('emos_keyboard_payload', b'\xd5\x2a'+address('_user_kbvector') +
          b'\x11\x00\x00\x00\xb7\xed\x52\x28\x0c\xd1\xd5\x21' +
          address('keyboard_payload_publish') + b'\xe5\x2a'+address('_user_kbvector')+b'\xe9')
    exact('keyboard_payload_publish', b'\xe1\x7e\x32'+address('_keyascii')+
          b'\x23\x7e\x32'+address('_keymods')+b'\x23\x46\x23\x7e\x4f\x32'+
          address('_keydown')+b'\x3a'+address('_keycount')+b'\x3c\x32'+address('_keycount')+
          b'\x78\x32'+address('_keycode')+b'\xc3'+address('keyboard_handler'))
    exact('_emos_keyboard_settings', b'\x21\x03\x00\x00\x39\xed\x27\x11'+
          address('_keydelay')+b'\x01\x05\x00\x00\xed\xb0\xc9')
    for wrapper, target in [('vdp_protocol_KEY','_emos_keyboard_mainboard'),
                            ('vdp_protocol_KEYSTATE','_emos_keyboard_mainboard_settings')]:
        exact(wrapper, b'\x21'+address('_vdp_protocol_data')+b'\xe5'+call(target)+b'\xe1\xc9')
    for wrapper in ('UART1_serial_TX','UART1_serial_RX','UART1_serial_GETCH','UART1_serial_PUTCH'):
        exact(wrapper, b'\xf5\x3a'+address('_uart1_keyboard_owned')+b'\xb7\xc2'+address('UART_keyboard_busy')+b'\xf1'
              if wrapper in ('UART1_serial_TX','UART1_serial_RX') else
              b'\xf5\x3a'+address('_uart1_keyboard_owned')+b'\xb7\xc2'+address('UART_keyboard_busy'))
    exact('UART_keyboard_busy', b'\xf1\xb7\xc9')
    if linked['keyboard_lookup_end']-linked['keyboard_lookup'] != 248*4:
        raise ValueError('stock virtual-key lookup bound changed')
    exact('keyboard_map', b'\xfe\xf9\xd0\x3d')
    # Check actual interrupt callers retain the housekeeping and UART0 parser.
    vblank = image[linked['_vblank_handler']:linked['_uart0_handler']]
    if call('emos_keyboard_tick_entry') not in vblank:
        raise ValueError('VBlank does not service keyboard deadlines/cleanup')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--elf', type=Path, required=True)
    p.add_argument('--nm', type=Path, required=True)
    p.add_argument('--objdump', type=Path)  # common profile interface
    args = p.parse_args()
    header = (args.source/'src/emos_keyboard.h').read_text()
    if not re.search(r'^#define EMOS_KEY_MAX 248$', header, re.M):
        raise SystemExit('receiver/table bounds differ')
    verify(args.elf.with_suffix('.bin').read_bytes(), symbols(args.nm,args.elf))
    print('EMOS keyboard linked IRQ, BYTE-return, callback, keymap and public UART guards verified')


if __name__ == '__main__':
    main()
