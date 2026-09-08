#!/usr/bin/env python3
"""Check target no-peer timeout, then prepare an isolated graphical review.

The CLI exits at stdin EOF, even while MOS is busy. Keep stdin open until the
bounded command returns to its prompt; an early process exit is not a PASS.
This script opens no physical serial port and never edits removable media.
"""
import argparse
import os
from pathlib import Path
import select
import shutil
import subprocess
import time

import yaml

from review_boot import make_profile, digest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--firmware', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--fab-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--smoke-bundle', type=Path,
                        help='include the same firmware build’s ordinary SD/clock smoke')
    args = parser.parse_args()
    output = args.output.absolute()
    output.mkdir(parents=True)  # Refuse to replace a prior review.
    firmware = output / 'MOS.bin'
    mos_map = output / 'MOS.map'
    shutil.copyfile(args.firmware, firmware)
    shutil.copyfile(args.map, mos_map)
    fab = args.fab_root.resolve(strict=True)
    media = output / 'sdcard'
    media.mkdir()
    commands = ['VDU 22 3']
    smoke_markers = []
    if args.smoke_bundle:
        bundle = args.smoke_bundle.resolve(strict=True)
        record = yaml.safe_load((bundle / 'build-manifest.yaml').read_text())
        outputs = {item['role']: item for item in record['outputs']}
        if digest(firmware) != outputs['firmware']['sha256']:
            raise SystemExit('Smoke bundle and review firmware differ')
        if digest(mos_map) != outputs['firmware_map']['sha256']:
            raise SystemExit('Smoke bundle and review map differ')
        smoke = bundle / outputs['smoke_program']['filename']
        if digest(smoke) != outputs['smoke_program']['sha256']:
            raise SystemExit('Smoke program hash differs from manifest')
        (media / 'bin').mkdir()
        (media / 'emos-boot').mkdir()
        shutil.copyfile(smoke, media / 'bin/EMBOOT.BIN')
        (media / 'emos-boot/check.txt').write_bytes(b'EMOS SD CHECK\r\n')
        commands.extend(['LOAD /bin/EMBOOT.BIN', 'RUN'])
        smoke_markers = [b'BOOT SMOKE SD PASS', b'BOOT SMOKE CLOCK PASS',
                         b'BOOT SMOKE PASS - returning to MOS']
    commands.append('EMOS UARTTEST')
    (media / 'autoexec.txt').write_bytes(('\r\n'.join(commands) + '\r\n').encode())
    process = subprocess.Popen([str(fab / 'target/release/agon-cli-emulator'),
                                '--mos', str(firmware), '--sdcard', str(media), '--zero'],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
    data = bytearray()
    expected = b'UART ROUND TRIP FAIL: no reply from P4'
    deadline = time.monotonic() + 20
    try:
        while time.monotonic() < deadline:
            if select.select([process.stdout], [], [], 0.2)[0]:
                block = os.read(process.stdout.fileno(), 65536)
                if not block:
                    break
                data.extend(block)
            if expected in data and data.rstrip().endswith(b'/ *'):
                break
    finally:
        process.terminate()
        process.communicate(timeout=5)
        (output / 'no-peer.log').write_bytes(data)
    if data.count(expected) != 1 or not data.rstrip().endswith(b'/ *'):
        raise SystemExit('FAIL: expected one no-reply timeout and prompt return; see no-peer.log')
    if b'UART ROUND TRIP PASS' in data or data.count(b'UART ROUND TRIP FAIL:') != 1:
        raise SystemExit('FAIL: unexpected diagnostic outcome')
    if any(marker not in data for marker in smoke_markers) or b'BOOT SMOKE FAIL:' in data:
        raise SystemExit('FAIL: ordinary boot smoke did not pass')
    make_profile(output / 'profile', firmware, mos_map, media, fab)
    print('PASS: emulated eZ80 no-peer timeout and prompt return')
    print('Human review: cd ' + str(output / 'profile') + ' && ./fab-agon-emulator')


if __name__ == '__main__':
    main()
