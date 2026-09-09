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
    args = parser.parse_args()
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
    project = ROOT/'projects/keyboard-api'
    identity = (project/'identity.txt').read_text().strip()
    build = identity+datetime.now(timezone.utc).strftime('-b%Y-%m-%d-%H-%M-%SZ')
    (project/'build').mkdir(exist_ok=True)
    (project/'build/build_identity.h').write_text(
        '// Generated; do not edit.\n#define PROBE_BUILD_ID "'+build+'"\n')
    with (output/'build.log').open('w') as log:
        subprocess.run(['make', 'clean'], cwd=project, stdout=log, stderr=subprocess.STDOUT, check=True)
        subprocess.run(['make', 'AGONDEV_TOOLCHAIN='+str(args.toolchain.resolve()), 'all'],
                       cwd=project, stdout=log, stderr=subprocess.STDOUT, check=True)
    media = output/'sdcard'
    shutil.copytree(bundle/'emos-sdcard', media)
    shutil.copy2(project/'bin/KBAPI.bin', media/'bin/KBAPI.BIN')
    (media/'autoexec.txt').write_bytes(b'VDU 22 3\r\nLOAD /bin/EMBOOT.BIN\r\nRUN\r\n'
                                     b'LOAD /bin/KBAPI.BIN\r\nRUN\r\nEMOS KEYINPUT\r\n')
    shutil.copy2(project/'bin/KBAPI.map', output/'KBAPI.map')
    source_files = [project/'Makefile', project/'identity.txt', *sorted((project/'src').iterdir()),
                    Path(__file__), ROOT/'scripts/keyboard_api_peer.py', ROOT/'scripts/review_boot.py']
    (output/'fixture-inputs.json').write_text(json.dumps({
        'build_id': build, 'status': 'draft', 'emos_build_id': record['build']['build_id'],
        'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
        'source_dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT)),
        'toolchain_inputs': {name: digest(args.toolchain.resolve()/name) for name in (
            'bin/ez80-none-elf-clang', 'bin/ez80-none-elf-as', 'bin/ez80-none-elf-ld',
            'lib/libagon.a', 'config/makefile.inc', 'config/linker.conf')},
        'firmware_sha256': digest(outputs['firmware']),
        'fixture_sha256': digest(media/'bin/KBAPI.BIN'),
        'fixture_map_sha256': digest(output/'KBAPI.map'),
        'firmware_manifest_sha256': digest(bundle/'build-manifest.yaml'),
        'runtime_manifest_sha256': digest(runtime/'runtime-inputs.json'),
        'inputs': {str(p.relative_to(ROOT)): digest(p) for p in source_files},
    }, indent=2)+'\n')
    subprocess.run([sys.executable, str(ROOT/'scripts/keyboard_api_peer.py'),
                    '--emulator', str(runtime/'target/release/agon-cli-emulator'),
                    '--firmware', str(outputs['firmware']), '--sdcard', str(media),
                    '--output', str(output/'paired-result.json')], check=True)
    make_profile(output/'profile', outputs['firmware'], outputs['firmware_map'], media, fab,
                 runtime=runtime, peer=ROOT/'scripts/keyboard_api_peer.py')
    print('Human review profile: '+str(output/'profile'))


if __name__ == '__main__':
    main()
