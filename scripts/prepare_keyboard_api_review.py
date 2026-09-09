#!/usr/bin/env python3
"""Build only the keyboard SD fixture against an unchanged reviewed EMOS image."""
from datetime import datetime, timezone
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import yaml
from review_boot import digest, make_profile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('bundle', 'fab-root', 'runtime', 'output', 'toolchain'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--fixture', choices=('api','wire'), default='api')
    parser.add_argument('--packets', type=Path, help='Retained P4 sender host-test output for wire fixture')
    args = parser.parse_args()
    if args.fixture == 'wire' and not args.packets:
        parser.error('--fixture wire requires --packets')
    bundle, fab, runtime = (p.resolve(strict=True) for p in (args.bundle, args.fab_root, args.runtime))
    record = yaml.safe_load((bundle/'build-manifest.yaml').read_text())
    outputs = {item['role']: bundle/item['filename'] for item in record['outputs']}
    for item in record['outputs']:
        if digest(bundle/item['filename']) != item['sha256']:
            raise ValueError('Changed firmware bundle input: '+item['filename'])
    runtime_record = json.loads((runtime/'runtime-inputs.json').read_text())
    for name, expected in runtime_record['files'].items():
        if digest(runtime/name) != expected:
            raise ValueError('Changed test runtime: '+name)
    output = args.output.absolute(); output.mkdir(parents=True)
    project = ROOT/('projects/keyboard-'+args.fixture)
    name = 'KBAPI' if args.fixture == 'api' else 'KBWIRE'
    peer_source = ROOT/('scripts/keyboard_'+args.fixture+'_peer.py')
    identity = (project/'identity.txt').read_text().strip()
    status_path = project/'status.txt'
    status = status_path.read_text().strip() if args.fixture == 'wire' else 'draft'
    if status not in ('draft','candidate'):
        raise ValueError('Unsupported fixture status')
    source_commit = subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    source_dirty = bool(subprocess.check_output(['git','status','--porcelain'],cwd=ROOT))
    if status == 'candidate' and (source_dirty or record['build']['status'] != 'candidate'
                                 or record['provenance']['dirty'] or record['provenance']['builder_dirty']):
        raise ValueError('Candidate fixture requires clean committed inputs and a candidate EMOS bundle')
    build = identity+datetime.now(timezone.utc).strftime('-b%Y-%m-%d-%H-%M-%SZ')
    (project/'build').mkdir(exist_ok=True)
    (project/'build/build_identity.h').write_text(
        '// Generated; do not edit.\n#define PROBE_BUILD_ID "'+build+'"\n'
        '#define PROBE_STATUS "'+status+'"\n')
    with (output/'build.log').open('w') as log:
        subprocess.run(['make', 'clean'], cwd=project, stdout=log, stderr=subprocess.STDOUT, check=True)
        subprocess.run(['make', 'AGONDEV_TOOLCHAIN='+str(args.toolchain.resolve()), 'all'],
                       cwd=project, stdout=log, stderr=subprocess.STDOUT, check=True)
    media = output/'sdcard'
    shutil.copytree(bundle/'emos-sdcard', media)
    shutil.copy2(project/('bin/'+name+'.bin'), media/('bin/'+name+'.BIN'))
    if args.fixture == 'wire':
        packet_record = json.loads(args.packets.with_suffix('.json').read_text())
        if packet_record['outcome'] != 'pass' or packet_record['packet_sha256'] != digest(args.packets):
            raise ValueError('Sender host-test bytes do not match passing provenance')
        shutil.copy2(args.packets, media/'key-packets.bin')
        shutil.copy2(args.packets.with_suffix('.json'), media/'key-packets.json')
    (media/'autoexec.txt').write_bytes(b'VDU 22 3\r\nLOAD /bin/EMBOOT.BIN\r\nRUN\r\n'
                                     + ('LOAD /bin/'+name+'.BIN\r\nRUN\r\nEMOS KEYINPUT\r\n').encode())
    shutil.copy2(project/('bin/'+name+'.map'), output/(name+'.map'))
    source_files = [project/'Makefile', project/'identity.txt', *sorted((project/'src').iterdir()),
                    Path(__file__), peer_source, ROOT/'scripts/review_boot.py']
    if args.fixture == 'wire': source_files.append(status_path)
    if status == 'candidate' and (subprocess.check_output(['git','status','--porcelain'],cwd=ROOT)
            or subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip() != source_commit):
        raise ValueError('Candidate source state changed during build')
    (output/'fixture-inputs.json').write_text(json.dumps({
        'build_id': build, 'status': status, 'emos_build_id': record['build']['build_id'],
        'source_commit': source_commit, 'source_dirty': source_dirty,
        'toolchain_inputs': {name: digest(args.toolchain.resolve()/name) for name in (
            'bin/ez80-none-elf-clang', 'bin/ez80-none-elf-as', 'bin/ez80-none-elf-ld',
            'lib/libagon.a', 'config/makefile.inc', 'config/linker.conf')},
        'firmware_sha256': digest(outputs['firmware']),
        'fixture_sha256': digest(media/('bin/'+name+'.BIN')),
        'fixture_map_sha256': digest(output/(name+'.map')),
        'firmware_manifest_sha256': digest(bundle/'build-manifest.yaml'),
        'runtime_manifest_sha256': digest(runtime/'runtime-inputs.json'),
        'sender_packets_sha256': digest(media/'key-packets.bin') if args.fixture == 'wire' else None,
        'sender_provenance_sha256': digest(media/'key-packets.json') if args.fixture == 'wire' else None,
        'inputs': {str(p.relative_to(ROOT)): digest(p) for p in source_files},
    }, indent=2)+'\n')
    peer = output/peer_source.name
    shutil.copy2(peer_source, peer)
    subprocess.run([sys.executable, str(peer),
                    '--emulator', str(runtime/'target/release/agon-cli-emulator'),
                    '--firmware', str(outputs['firmware']), '--sdcard', str(media),
                    '--output', str(output/'paired-result.json')], check=True)
    if args.fixture == 'wire':
        subprocess.run([sys.executable, str(peer), '--withhold-keys',
                        '--emulator', str(runtime/'target/release/agon-cli-emulator'),
                        '--firmware', str(outputs['firmware']), '--sdcard', str(media),
                        '--output', str(output/'timeout-result.json')], check=True)
    make_profile(output/'profile', outputs['firmware'], outputs['firmware_map'], media, fab,
                 runtime=runtime, peer=peer)
    print('Human review profile: '+str(output/'profile'))


if __name__ == '__main__':
    main()
