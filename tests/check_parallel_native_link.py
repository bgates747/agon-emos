#!/usr/bin/env python3
"""Exercise native-profile link guards against actual full ELF images.

Requires pyelftools, the same dependency as AUDIT-008 ELF accounting. Mutations
are made only in temporary copies, never in a retained build or source tree.
No target execution or hardware access occurs.
"""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
from time import monotonic
import tempfile
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[1]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--ordinary', type=Path, required=True)
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--toolchain', type=Path, required=True)
    a = p.parse_args()
    tools = a.toolchain / 'bin'
    def check(elf, native, expected, reason):
        checker = 'verify_parallel_native.py' if native else 'verify_parallel.py'
        r = subprocess.run([
            sys.executable, str(ROOT / 'projects/emos' / checker),
            '--source', str(a.source), '--elf', str(elf),
            '--nm', str(tools / 'ez80-none-elf-nm'),
            '--objdump', str(tools / 'ez80-none-elf-objdump'),
        ], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if (r.returncode == 0) != expected or (reason and reason not in r.stdout):
            raise AssertionError(r.stdout)
        print(('PASS ' if expected else 'REFUSED ') + checker + ': ' + r.stdout.strip())

    check(a.native, True, True, '')
    check(a.ordinary, False, True, '')
    check(a.native, False, False, 'ordinary image links private native payload symbols')
    check(a.ordinary, True, False, 'linked Port C writer set changed')
    with a.native.open('rb') as f:
        elf = ELFFile(f)
        symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
        section = elf.get_section_by_name('.startup')
        delta = section['sh_offset'] - section['sh_addr']
    original = a.native.read_bytes()
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / 'mutant.elf'
        # Corrupt a genuine linked instruction, keeping the symbol table intact.
        di = symbols['native_control'] + delta + 3  # LD A,I; PUSH AF; DI
        assert original[di] == 0xf3
        damaged = bytearray(original); damaged[di] = 0
        target.write_bytes(damaged)
        check(target, True, False, 'exact IFF2 save/DI/restore sequence')

        output = original.index(bytes.fromhex('ed399e'), symbols['native_send'] + delta)
        damaged = bytearray(original); damaged[output + 2] = 0x9f
        target.write_bytes(damaged)
        check(target, True, False, 'register-write inventory changed')

        # Redirect the C guard's sole raw-leaf CALL one byte into the leaf.
        call = bytes([0xcd]) + symbols['_emos_parallel_native_bytes'].to_bytes(3, 'little')
        offset = original.index(call, symbols['_emos_parallel_native_payload'] +
                                elf_offset(a.native, '_emos_parallel_native_payload'))
        damaged = bytearray(original)
        damaged[offset + 1:offset + 4] = (symbols['_emos_parallel_native_bytes'] + 1).to_bytes(3, 'little')
        target.write_bytes(damaged)
        check(target, True, False, 'native raw leaf is not called solely by its guard')
        # Redirect a genuine call to the dormant coordinator: private software
        # integration must still contain zero physical activation callers.
        damaged = bytearray(original)
        damaged[offset + 1:offset + 4] = symbols['_emos_parallel_native_coordinate'].to_bytes(3, 'little')
        target.write_bytes(damaged)
        check(target, True, False, 'private coordinator acquired an unreviewed activation caller')

        call = bytes([0xcd]) + symbols['_emos_parallel_native_payload'].to_bytes(3, 'little')
        offset = original.index(call, symbols['_emos_parallel_native_coordinate'] +
                                elf_offset(a.native, '_emos_parallel_native_coordinate'))
        damaged = bytearray(original)
        damaged[offset + 1:offset + 4] = (symbols['_emos_parallel_native_payload'] + 1).to_bytes(3, 'little')
        target.write_bytes(damaged)
        check(target, True, False, 'native guard is not called solely by the coordinator')
    print('PASS 9 actual-ELF profile/negative-control cases; no hardware claims')

def elf_offset(path, name):
    with path.open('rb') as f:
        elf = ELFFile(f)
        symbol = next(s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name == name)
        section = elf.get_section(symbol['st_shndx'])
        return section['sh_offset'] - section['sh_addr']

if __name__ == '__main__':
    start = datetime.now(timezone.utc).isoformat()
    clock = monotonic()
    try:
        main()
    finally:
        print(json.dumps({'start_utc': start,
                          'end_utc': datetime.now(timezone.utc).isoformat(),
                          'elapsed_host_seconds': round(monotonic() - clock, 3),
                          'scope': 'host link checks, not target performance'}))
