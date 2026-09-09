#!/usr/bin/env python3
"""Review resident keyboard selection and absent-peer recovery on the eZ80.

Fab CLI has no UART1 peer. Success here is retained mainboard input and a
bounded browser-admission failure, not browser keyboard qualification.
"""
import argparse
import json
import os
from pathlib import Path
import select
import shutil
import subprocess
import time
import yaml
from review_boot import digest, make_profile


def await_prompt(process, seconds=20):
    data = bytearray()
    deadline = time.monotonic()+seconds
    while time.monotonic() < deadline:
        if select.select([process.stdout], [], [], 0.1)[0]:
            block = os.read(process.stdout.fileno(), 65536)
            if not block:
                break
            data.extend(block)
        if data.rstrip().endswith(b'/ *'):
            return bytes(data)
    raise RuntimeError('No MOS prompt: '+data.decode(errors='replace')[-1500:])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bundle', type=Path, required=True)
    p.add_argument('--fab-root', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    bundle, fab = args.bundle.resolve(strict=True), args.fab_root.resolve(strict=True)
    record = yaml.safe_load((bundle/'build-manifest.yaml').read_text())
    outputs = {item['role']: bundle/item['filename'] for item in record['outputs']}
    for item in record['outputs']:
        if digest(bundle/item['filename']) != item['sha256']:
            raise SystemExit('Bundle hash mismatch: '+item['filename'])
    review = args.output.absolute()
    review.mkdir(parents=True)  # Preserve prior reviews.
    media = review/'sdcard'
    shutil.copytree(bundle/'emos-sdcard', media)
    commands = ['VDU 22 3', 'LOAD /bin/EMBOOT.BIN', 'RUN', 'EMOS KEYINPUT',
                'EmOs KeYiNpUt MaInBoArD', 'SET KEYBOARD 1', 'EMOS KEYINPUT browser']
    (media/'autoexec.txt').write_bytes(('\r\n'.join(commands)+'\r\n').encode())
    process = subprocess.Popen([str(fab/'target/release/agon-cli-emulator'), '--mos',
                                str(outputs['firmware']), '--sdcard', str(media), '--zero'],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    transcript = bytearray()
    try:
        boot = await_prompt(process)
        transcript.extend(boot)
        expected = [b'BOOT SMOKE SD PASS', b'BOOT SMOKE CLOCK PASS',
                    b'BOOT SMOKE PASS - returning to MOS',
                    b'KEYINPUT FAIL: receiver readiness timeout']
        if any(boot.count(item) != 1 for item in expected) or boot.count(b'Keyboard input: mainboard') != 3:
            raise RuntimeError('Unexpected boot/absent-peer result')
        # CLI stdin is delivered through stock UART0 key events: proving these
        # commands still execute also checks the refactored mainboard handler.
        cases = [
            ('eMoS kEyInPuT', 'Keyboard input: mainboard'),
            ('EMOS KEYINPUT extender', 'Extender keyboard input is not available'),
            ('EMOS KEYINPUT 1', 'Usage: EMOS KEYINPUT'),
            ('EMOS KEYINPUT browser extra', 'Usage: EMOS KEYINPUT'),
            ('EMOS KEYINPUT --browser', 'Usage: EMOS KEYINPUT'),
            ('EMOS KEYINPUT MAINBOARD', 'Keyboard input: mainboard'),
            ('EMOS KEYINPUT browser', 'KEYINPUT FAIL: receiver readiness timeout'),
            ('EMOS KEYINPUT', 'Keyboard input: mainboard'),
        ]
        for command, expected_text in cases:
            process.stdin.write((command+'\n').encode()); process.stdin.flush()
            response = await_prompt(process)
            transcript.extend(response)
            if expected_text.encode() not in response:
                raise RuntimeError('Unexpected result for '+command)
    finally:
        process.terminate(); process.communicate(timeout=5)
        (review/'keyboard-cli.log').write_bytes(transcript)
    (review/'review-inputs.json').write_text(json.dumps({
        'script_sha256': digest(Path(__file__)),
        'manifest_sha256': digest(bundle/'build-manifest.yaml'),
        'cli_sha256': digest(fab/'target/release/agon-cli-emulator'),
        'scope': 'SD/clock, mainboard input/editor, command parsing and absent UART1 peer',
        'hardware_or_browser_input_qualified': False,
    }, indent=2)+'\n')
    make_profile(review/'profile', outputs['firmware'], outputs['firmware_map'], media, fab)
    print('PASS: SD/clock, mainboard command input and repeatable absent-peer recovery')
    print('Human review profile: '+str(review/'profile'))


if __name__ == '__main__':
    main()
