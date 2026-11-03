"""Oryx: four STOPPED profiles for the two added, pinned firmware cases."""
from pathlib import Path
import hashlib,importlib.util,json,shutil,subprocess,sys
root=Path(__file__).resolve().parents[3];b=root/'build/tm001';build=b/sys.argv[1];out=b/sys.argv[2];out.mkdir(exist_ok=False)
old=root.parent/'agon-mos';template=old/'.emulator/uart0-completion-jit-final';runtime=b/'perf-flexe002/runtime';common=out/'runtime';shutil.copytree(runtime,common)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert h(common/'flexe-agon-emulator.bin')=='7e7d0e1bc89af72bde7ffbd6c06446aaefda4d1803f89eb1faf3167307ee1e01'
assert h(common/'flexe-vdp.so')=='0866265bd6f3d73ad572f31dc1bb954f308ed9a5d857efa9414d37e1ade13374'
assert h(common/'esp32-firmware.bin')=='201ad8ab4af807c261cb92634286b2fbaedcf3879f140e820b60ab54cf707e2a'
spec=importlib.util.spec_from_file_location('factory',root.parent/'agon-dev-env/scripts/setup_emulator.py');factory=importlib.util.module_from_spec(spec);spec.loader.exec_module(factory)
rows=[]
for c in json.loads((build/'cases.json').read_text()):
 name=c['case'];source=build/name;fixture=source/'fixture'
 m=root.parents[1]/'flexe-emulator/mos_platform.map' if name=='stock' else old/'build/uart0-balls/run007/appliance/mos-candidate.map'
 # root=/home/smith/Agon/mystuff/eirene-mos; stock release lives under Agon.
 assert h(m)==('d69e60bbce61a7b4b3eef318ba395f11c4e1a5b585755755113992dc94edcb86' if name=='stock' else '66494f3dcd19fb1f71bf6bc62c8ab690316705ce2123037a8c1db051555f2370')
 for engine in ('jit','interp'):
  p=out/(name+'-'+engine);p.mkdir();(p/'sdcard').mkdir()
  files=('flexe-agon-emulator.bin','flexe-vdp.so','esp32-firmware.bin','esp32-firmware.elf')
  for n in files:(p/n).symlink_to('../runtime/'+n)
  shutil.copyfile(source/'mos.bin',p/'mos_platform.bin');shutil.copyfile(m,p/'mos_platform.map')
  assert h(p/'mos_platform.bin')==c['MOS_sha256']
  for n in ('flexe-agon-emulator','.bespoke-vdp-profile'):shutil.copyfile(template/n,p/n)
  (p/'flexe-agon-emulator').chmod(0o755)
  (p/'.mos-release').write_text('PERF1 added comparison '+name+'; private profile\n')
  (p/'.mos-release.sha256').write_text(''.join(h(p/n)+'  '+n+'\n' for n in ('mos_platform.bin','mos_platform.map')))
  card=p/'card.img'
  with card.open('xb') as f:f.truncate(32*1024*1024)
  subprocess.run(['mkfs.fat','-F','16',str(card)],check=True,stdout=subprocess.DEVNULL);subprocess.run(['mmd','-i',str(card),'::/uart0'],check=True)
  def put(src,dst):subprocess.run(['mcopy','-i',str(card),str(src),'::/'+dst],check=True)
  put(fixture/'bin'/(name+'.bin'),'uart0/test.bin');put(fixture/'checks.bin','uart0/checks.bin')
  a=p/'autoexec.txt';a.write_bytes(('CD /uart0\r\nLOAD test.bin\r\nRUN . '+str(c['mode'])+'\r\nDIR\r\nECHO TM001 CLI RETURN\r\n').encode());put(a,'autoexec.txt')
  (p/'sdcard.img').symlink_to('card.img');factory.name_flexe_profile(p)
  (p/'.runtime.sha256').write_text(''.join(h(p/n)+'  '+n+'\n' for n in (*files,'mos_platform.bin','mos_platform.map','flexe-agon-emulator')))
  for pin in ('.runtime.sha256','.mos-release.sha256'):subprocess.run(['sha256sum','--check','--status',pin],cwd=p,check=True)
  rows.append(dict(path=str(p),variant=name,engine=engine,mode=c['mode'],fixture=str(source.relative_to(b)),fixture_sha256=h(fixture/'bin'/(name+'.bin')),MOS_sha256=c['MOS_sha256'],map_sha256=h(m),stopped=True))
(out/'profiles.json').write_text(json.dumps(dict(suite='performance',extra=True,profiles=rows),indent=2)+'\n');print(out)
