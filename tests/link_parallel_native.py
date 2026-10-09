#!/usr/bin/env python3
"""Link already-qualified native objects at RAM address 0x40000, never firmware.

Run after the maintained parallel-native-candidate profile compile/link attempt.
An overflow never authorizes oversized firmware or deployment. The image also
remains useful after the resident composition fits: it isolates the same leaves.
This separate image executes only in tests/uart_put_cpu's instruction interpreter.
"""
from pathlib import Path
from datetime import datetime, timezone
from time import monotonic
import argparse, hashlib, json, subprocess

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-root', type=Path, required=True,
                   help='mos-agondev projects/mos-port directory after compilation')
    p.add_argument('--toolchain', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a=p.parse_args(); b=a.build_root.resolve(); t=a.toolchain.resolve()/'bin'
    out=a.output.resolve()
    objects=[b/'obj/src/emos_parallel_native.o',
             b/'obj/asm/src/emos_parallel_native_io.o',
             b/'runtime/build/libmos_runtime.a']
    for path in objects:
        if not path.is_file():p.error(f'compiled input missing: {path}')
    if out.exists():p.error('RAM-only output must be new; preserve previous evidence')
    out.mkdir(parents=True)
    start=datetime.now(timezone.utc).isoformat(); clock=monotonic()
    script=out/'ram-only.ld'
    script.write_text('SECTIONS { . = 0x40000; .text : { *(.text*) *(.STARTUP) } '
                      '.data : { *(.data*) } .bss : { *(.bss*) } }\n')
    subprocess.run([str(t/'ez80-none-elf-ld'),'-T',str(script),'-o',str(out/'payload.elf'),
                    *map(str,objects)],check=True)
    subprocess.run([str(t/'ez80-none-elf-objcopy'),'-O','binary',str(out/'payload.elf'),
                    str(out/'payload.bin')],check=True)
    (out/'nm.txt').write_text(subprocess.check_output(
        [str(t/'ez80-none-elf-nm'),'-n',str(out/'payload.elf')],text=True))
    def identity(path):
        return {'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    record={'kind':'RAM_ONLY_INSTRUCTION_TEST_NOT_FIRMWARE','load_address':0x40000,
            'start_utc':start,'end_utc':datetime.now(timezone.utc).isoformat(),
            'elapsed_host_seconds':monotonic()-clock,
            'inputs':{str(path):identity(path) for path in objects},
            'outputs':{name:identity(out/name) for name in ('payload.elf','payload.bin','nm.txt')}}
    (out/'ram-test.json').write_text(json.dumps(record,indent=2)+'\n')
    print('RAM-only instruction test image linked; not flashable firmware')
if __name__=='__main__':main()
