"""Oryx-only: create stopped, guarded, private profiles. Does not run firmware."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,shutil,subprocess,sys
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('name');parser.add_argument('--suite',choices=('functional','performance'),default='functional')
parser.add_argument('--fixture',default='fixture003');args=parser.parse_args()
root=Path(__file__).resolve().parents[2];b=root/'build/tm001';out=b/args.name;out.mkdir(exist_ok=False)
fixture=b/args.fixture/'fixture';performance=args.suite=='performance'

old=root.parent/'agon-mos';base=old/'.emulator/uart0-completion-jit-final';common=out/'runtime';common.mkdir()
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
files={'flexe-agon-emulator.bin':old/'build/uart0-balls/runtime-maintained/flexe-agon-emulator.bin','flexe-vdp.so':old/'build/uart0-balls/runtime-maintained/flexe-vdp.so','esp32-firmware.bin':base/'esp32-firmware.bin','esp32-firmware.elf':base/'esp32-firmware.elf'}
for n,p in files.items():shutil.copyfile(p,common/n)
assert h(common/'flexe-agon-emulator.bin')=='7e7d0e1bc89af72bde7ffbd6c06446aaefda4d1803f89eb1faf3167307ee1e01'
assert h(common/'flexe-vdp.so')=='0866265bd6f3d73ad572f31dc1bb954f308ed9a5d857efa9414d37e1ade13374'
assert h(common/'esp32-firmware.bin')=='201ad8ab4af807c261cb92634286b2fbaedcf3879f140e820b60ab54cf707e2a'
(common/'flexe-agon-emulator.bin').chmod(0o755)
spec=importlib.util.spec_from_file_location('factory',root.parent/'agon-dev-env/scripts/setup_emulator.py');factory=importlib.util.module_from_spec(spec);spec.loader.exec_module(factory)
rows=[]
for variant in ('baseline','scalar003','otirx003'):
 for engine in ('jit','interp'):
  p=out/(variant+'-'+engine);p.mkdir();(p/'sdcard').mkdir()
  for n in files:(p/n).symlink_to('../runtime/'+n)
  for src,dst in (('mos.bin','mos_platform.bin'),('mos.map','mos_platform.map')):shutil.copyfile(b/variant/'bin'/src,p/dst)
  for n in ('flexe-agon-emulator','.bespoke-vdp-profile'):shutil.copyfile(base/n,p/n)
  (p/'flexe-agon-emulator').chmod(0o755)
  (p/'.mos-release').write_text('Eirene TM001 '+variant+'; experimental/off-device only\n')
  (p/'.mos-release.sha256').write_text(''.join(h(p/n)+'  '+n+'\n' for n in ('mos_platform.bin','mos_platform.map')))
  card=p/'card.img'
  with card.open('xb') as f:f.truncate(32*1024*1024)
  subprocess.run(['mkfs.fat','-F','16',str(card)],check=True,stdout=subprocess.DEVNULL);subprocess.run(['mmd','-i',str(card),'::/uart0'],check=True)
  def put(src,dst):subprocess.run(['mcopy','-i',str(card),str(src),'::/'+dst],check=True)
  put(fixture/'bin'/('test.bin' if performance else 'fixture.bin'),'uart0/test.bin');put(fixture/'checks.bin','uart0/checks.bin')
  mode=str(('baseline','scalar003','otirx003').index(variant)) if performance else ('0' if variant=='baseline' else '1')
  a=p/'autoexec.txt';a.write_bytes(('CD /uart0\r\nLOAD test.bin\r\nRUN . '+mode+'\r\nDIR\r\nECHO TM001 CLI RETURN\r\n').encode());put(a,'autoexec.txt')
  (p/'sdcard.img').symlink_to('card.img');factory.name_flexe_profile(p)
  names=[*files,'mos_platform.bin','mos_platform.map','flexe-agon-emulator'];(p/'.runtime.sha256').write_text(''.join(h(p/n)+'  '+n+'\n' for n in names))
  for pin in ('.mos-release.sha256','.runtime.sha256'):subprocess.run(['sha256sum','--check','--status',pin],cwd=p,check=True)
  subprocess.run(['bash','-n','flexe-agon-emulator'],cwd=p,check=True)
  rows.append(dict(path=str(p),variant=variant,engine=engine,MOS_sha256=h(p/'mos_platform.bin'),map_sha256=h(p/'mos_platform.map'),stopped=True))
(out/'profiles.json').write_text(json.dumps(dict(suite=args.suite,fixture=args.fixture,profiles=rows,runtime={n:h(common/n) for n in files},fixture_sha256=h(fixture/'bin'/('test.bin' if performance else 'fixture.bin'))),indent=2)+'\n')
print(out)
