#!/usr/bin/env python3
"""Bounded, local-only Fab CLI check of the actual EMOS utility dispatcher.

Uses the same official CLI runtime as projects/mos-port/verify_boot.py.
Its fake VDP verifies loading/CLI behavior, not graphics or physical UART.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import time

COMMANDS = [
    'EMOS STATUS', 'EmOs PrObE alpha "two words"', 'emos probe return7',
    'emos probe again', 'emos abcdefghijklmnopqrstuvwx boundary',
    'emos abcdefghijklmnopqrstuvwxy', 'emos ../probe', 'emos probe.bin',
    'emos missing', 'emos short', 'emos bad', 'emos huge', 'emos directory',
    'emos wrongmode', 'emos discover', 'emos clear', 'emos call core.echo abandoned',
    'emos fake on', 'emos mode dual', 'emos status', 'emos mode legacy', 'emos fake off',
    'emos sdserve /', 'LOAD /app.bin 0x40000', 'RUN . application',
    'LOAD /sentinel.bin 0x40000', 'emos probe after-app',
    'echo Audit utility checks finished',
]
ERRORS = {
    'emos abcdefghijklmnopqrstuvwxy': 'Invalid parameter',
    'emos ../probe': 'Invalid parameter', 'emos probe.bin': 'Invalid parameter',
    'emos missing': 'Could not find file', 'emos short': 'Invalid executable',
    'emos bad': 'Invalid executable', 'emos huge': 'Load overlaps system area',
    'emos directory': 'Invalid executable', 'emos wrongmode': 'Invalid executable',
    'emos discover': 'EMOS backend unavailable', 'emos clear': 'EMOS backend unavailable',
    'emos call core.echo abandoned': 'EMOS provider not found',
}


def validate(text):
    assert 'FAIL:' not in text, 'probe reported a failure'
    assert 'Error executing autoexec' not in text, 'autoexec did not finish'
    blocks = text.replace('\r', '').split('/ *')
    boot = blocks[0]
    by_command = {b.split('\n', 1)[0]: b for b in blocks[1:]}
    assert 'arg1=[autoexec]' in boot and 'PASS: application sentinel preserved' in boot
    for cmd in COMMANDS:
        assert cmd in by_command, 'missing command: '+cmd
    for cmd, expected in ERRORS.items():
        block = by_command[cmd]
        assert expected in block, (cmd, block)
        assert 'Utility probe:' not in block, 'stale MOSlet executed: '+cmd
    assert 'Utility probe:' not in by_command['EMOS STATUS'], 'disk file shadowed built-in'
    for cmd in ['EmOs PrObE alpha "two words"', 'emos probe return7', 'emos probe again',
                'emos abcdefghijklmnopqrstuvwx boundary', 'emos probe after-app']:
        block = by_command[cmd]
        for token in ['invocation=1', 'PASS: application sentinel preserved',
                      'PASS: MOSlet nested dispatch rejected', 'PASS: resident built-in remains available']:
            assert token in block, (cmd, token)
    # AgonDev's stock argv parser splits quoted whitespace; retain its behavior.
    argblock = by_command['EmOs PrObE alpha "two words"']
    assert 'arg1=[alpha]' in argblock and 'arg2=["two]' in argblock and 'arg3=[words"]' in argblock
    assert 'Access denied or directory full' in by_command['emos probe return7']
    assert 'EMOS: Dual' in by_command['emos status']
    assert 'mode generation 2' in by_command['emos probe after-app']
    assert 'PASS: application nested dispatch rejected' in by_command['RUN . application']
    assert 'SD service unavailable (EMOS 35)' in by_command['emos sdserve /']
    assert by_command[COMMANDS[-1]].count('Audit utility checks finished') == 2
    assert blocks[-1] == '', 'MOS prompt missing at completion'


def digest(path):
    raw = path.read_bytes()
    return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('cli','firmware','utility','application','listener','work','output'):
        p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    if a.work.exists(): raise SystemExit('Use a fresh, nonexistent local work directory')
    for name in ('cli','firmware','utility','application','listener'):
        if not getattr(a,name).is_file(): raise SystemExit('Missing '+name)
    started=datetime.now(timezone.utc);start=time.monotonic()
    sd=a.work.resolve()/'sdcard';(sd/'emos').mkdir(parents=True)
    probe=a.utility.read_bytes()
    for name in ('probe','status','abcdefghijklmnopqrstuvwx'):
        (sd/'emos'/f'{name}.bin').write_bytes(probe)
    (sd/'emos/short.bin').write_bytes(b'\0'*68)
    (sd/'emos/bad.bin').write_bytes(b'\0'*100)
    wrong=bytearray(probe);wrong[0x44]=2;(sd/'emos/wrongmode.bin').write_bytes(wrong)
    assert len(probe)<32769
    (sd/'emos/huge.bin').write_bytes(probe+b'\0'*(32769-len(probe)))
    (sd/'emos/directory.bin').mkdir()
    (sd/'sentinel.bin').write_bytes(bytes((i*17+3)&255 for i in range(4096)))
    (sd/'app.bin').write_bytes(a.application.read_bytes())
    (sd/'emos/sdserve.bin').write_bytes(a.listener.read_bytes())
    (sd/'autoexec.txt').write_bytes(b'VDU 22 3\r\nLOAD /sentinel.bin 0x40000\r\nEmOs PrObE autoexec\r\n')
    before={str(f.relative_to(sd)):digest(f) for f in sd.rglob('*') if f.is_file()}
    command=[str(a.cli.resolve()),'--mos',str(a.firmware.resolve()),'--sdcard',str(sd),
             '--unlimited-cpu','--zero']
    result=subprocess.run(command,cwd=a.work,input=('\n'.join(COMMANDS)+'\n').encode(),
                          stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
    a.output.mkdir(parents=True,exist_ok=True)
    (a.output/'transcript.txt').write_bytes(result.stdout)
    assert result.returncode==0, 'CLI emulator process failed'
    validate(result.stdout.decode(errors='replace'))
    after={str(f.relative_to(sd)):digest(f) for f in sd.rglob('*') if f.is_file()}
    assert before==after, 'unexpected SD fixture modification'
    record={'fixture_identity':'emos-utility-probe-r01',
            'run_id':'AUDIT-008-'+started.strftime('%Y-%m-%d-%H-%M-%SZ'),
            'elapsed_seconds':time.monotonic()-start,'status':'pass',
            'scope':'Fab CLI loader/ABI checks; fake VDP; no bench or physical SD',
            'listener_scope':'MOSlet launch and expected unavailable-transport return only',
            'inputs':{name:digest(getattr(a,name)) for name in ('cli','firmware','utility','application','listener')},
            'commands':COMMANDS,'media_unchanged':True}
    (a.output/'result.json').write_text(json.dumps(record,indent=2)+'\n')
    print('EMOS utility emulator checks passed; %.1f seconds'%record['elapsed_seconds'])

if __name__=='__main__':main()
