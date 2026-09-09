#!/usr/bin/env python3
"""Prepare a source-bound ordinary CLI review; no SD app, physical media or flash.

The native USB host emits stock events to resident EMOS; this peer substitutes
only UART1 and uses bytes emitted by Extender's actual retained sender methods.
"""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import subprocess
import sys
import yaml
from review_boot import digest, make_profile

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('bundle','fab-root','runtime','packets','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    bundle,fab,runtime=(p.resolve(strict=True) for p in (args.bundle,args.fab_root,args.runtime))
    record=yaml.safe_load((bundle/'build-manifest.yaml').read_text())
    outputs={item['role']:bundle/item['filename'] for item in record['outputs']}
    for item in record['outputs']:
        if digest(bundle/item['filename'])!=item['sha256']: raise ValueError('Changed bundle: '+item['filename'])
    runtime_record=json.loads((runtime/'runtime-inputs.json').read_text())
    for name,expected in runtime_record['files'].items():
        if digest(runtime/name)!=expected: raise ValueError('Changed peer runtime: '+name)
    packet_record=json.loads(args.packets.with_suffix('.json').read_text())
    if packet_record['outcome']!='pass' or packet_record['packet_sha256']!=digest(args.packets):
        raise ValueError('Sender bytes lack matching passing provenance')
    output=args.output.absolute();output.mkdir(parents=True)
    media=output/'sdcard';shutil.copytree(bundle/'emos-sdcard',media)
    shutil.copy2(args.packets,media/'usb-cli-packets.bin')
    shutil.copy2(args.packets.with_suffix('.json'),media/'usb-cli-packets.json')
    (media/'autoexec.txt').write_bytes(b'VDU 22 3\r\nLOAD /bin/EMBOOT.BIN\r\nRUN\r\n'
        b'SET KEYBOARD 1\r\nEMOS KEYINPUT extender\r\n')
    peer_source=ROOT/'scripts/usb_cli_peer.py'
    peer=output/peer_source.name;shutil.copy2(peer_source,peer)
    build='usb-cli-probe-r01'+datetime.now(timezone.utc).strftime('-b%Y-%m-%d-%H-%M-%SZ')
    (output/'review-inputs.json').write_text(json.dumps({
        'build_id':build,'status':'draft','emos_build_id':record['build']['build_id'],
        'scope':'ordinary MOS CLI with native USB mapped retained sender bytes; emulator only',
        'firmware_manifest_sha256':digest(bundle/'build-manifest.yaml'),
        'runtime_manifest_sha256':digest(runtime/'runtime-inputs.json'),
        'sender_sha256':digest(args.packets),'sender_provenance_sha256':digest(args.packets.with_suffix('.json')),
        'inputs':{str(p.relative_to(ROOT)):digest(p) for p in (Path(__file__),peer_source,ROOT/'scripts/review_boot.py')},
    },indent=2)+'\n')
    for withhold in (False,True):
        command=[sys.executable,str(peer),'--emulator',str(runtime/'target/release/agon-cli-emulator'),
                 '--firmware',str(outputs['firmware']),'--sdcard',str(media),
                 '--output',str(output/('absent-peer-result.json' if withhold else 'paired-result.json'))]
        if withhold: command+=['--withhold-admission']
        subprocess.run(command,check=True)
    make_profile(output/'profile',outputs['firmware'],outputs['firmware_map'],media,fab,
                 runtime=runtime,peer=peer,mutable_names=())
    print('Ready for human review: '+str(output/'profile'))


if __name__=='__main__':
    main()
