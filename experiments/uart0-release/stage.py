"""Stage stopped guarded RC1 profiles; two plain test invocations plus C example."""
from pathlib import Path
import hashlib,importlib.util,json,shutil,subprocess,sys
root=Path(__file__).resolve().parents[2];base=root/'build/tm001';build=base/sys.argv[1];out=base/sys.argv[2];out.mkdir(exist_ok=False)
info=json.loads((build/'build-info.json').read_text());common=out/'runtime';shutil.copytree(base/'perf-flexe002/runtime',common)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for n,pin in {'flexe-agon-emulator.bin':'7e7d0e1bc89af72bde7ffbd6c06446aaefda4d1803f89eb1faf3167307ee1e01','flexe-vdp.so':'0866265bd6f3d73ad572f31dc1bb954f308ed9a5d857efa9414d37e1ade13374','esp32-firmware.bin':'201ad8ab4af807c261cb92634286b2fbaedcf3879f140e820b60ab54cf707e2a'}.items():assert h(common/n)==pin
spec=importlib.util.spec_from_file_location('factory',root.parent/'agon-dev-env/scripts/setup_emulator.py');factory=importlib.util.module_from_spec(spec);spec.loader.exec_module(factory)
template=root.parent/'agon-mos/.emulator/uart0-completion-jit-final';rows=[]
for engine in ('jit','interp'):
 profile=out/('rc1-'+engine);profile.mkdir();(profile/'sdcard').mkdir();runtime=('flexe-agon-emulator.bin','flexe-vdp.so','esp32-firmware.bin','esp32-firmware.elf')
 for n in runtime:(profile/n).symlink_to('../runtime/'+n)
 for src,dst in (('mos.bin','mos_platform.bin'),('mos.map','mos_platform.map')):shutil.copyfile(build/'firmware/bin'/src,profile/dst)
 for n in ('flexe-agon-emulator','.bespoke-vdp-profile'):shutil.copyfile(template/n,profile/n)
 (profile/'flexe-agon-emulator').chmod(0o755);(profile/'.mos-release').write_text(info['title']+'; focused qualification\n')
 (profile/'.mos-release.sha256').write_text(''.join(h(profile/n)+'  '+n+'\n' for n in ('mos_platform.bin','mos_platform.map')))
 card=profile/'card.img'
 with card.open('xb') as f:f.truncate(32*1024*1024)
 subprocess.run(['mkfs.fat','-F','16',str(card)],check=True,stdout=subprocess.DEVNULL);subprocess.run(['mmd','-i',str(card),'::/uart0'],check=True)
 for src,dst in ((build/'fixture/bin/test.bin','uart0/test.bin'),(build/'fixture/checks.bin','uart0/checks.bin'),(build/'example/bin/example.bin','uart0/example.bin')):subprocess.run(['mcopy','-i',str(card),str(src),'::/'+dst],check=True)
 autoexec=profile/'autoexec.txt';autoexec.write_bytes(b'CD /uart0\r\ntest\r\ntest\r\nexample\r\nDIR\r\nECHO TM001 CLI RETURN\r\n');subprocess.run(['mcopy','-i',str(card),str(autoexec),'::/autoexec.txt'],check=True)
 (profile/'sdcard.img').symlink_to('card.img');factory.name_flexe_profile(profile)
 (profile/'.runtime.sha256').write_text(''.join(h(profile/n)+'  '+n+'\n' for n in (*runtime,'mos_platform.bin','mos_platform.map','flexe-agon-emulator')))
 for n in ('.runtime.sha256','.mos-release.sha256'):subprocess.run(['sha256sum','--check','--status',n],cwd=profile,check=True)
 rows.append(dict(path=str(profile),variant='rc1',engine=engine,MOS_sha256=info['MOS_sha256'],MOS_crc32=info['MOS_crc32'],map_sha256=info['map_sha256'],fixture_sha256=info['test_sha256'],example_sha256=info['example_sha256'],fixture=str(build.relative_to(base)),stopped=True))
(out/'profiles.json').write_text(json.dumps(dict(suite='releasecandidate',profiles=rows),indent=2)+'\n');print(out)
