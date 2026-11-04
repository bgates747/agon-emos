"""Create ten STOPPED guarded profiles, invoking apps by plain command name."""
from pathlib import Path
import hashlib,importlib.util,json,shutil,subprocess,sys
root=Path(__file__).resolve().parents[2];b=root/'build/tm001';build=b/sys.argv[1];out=b/sys.argv[2];out.mkdir(exist_ok=False)
old=root.parent/'agon-mos';template=old/'.emulator/uart0-completion-jit-final';runtime=b/'perf-flexe002/runtime';common=out/'runtime';shutil.copytree(runtime,common)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for n,pin in {'flexe-agon-emulator.bin':'7e7d0e1bc89af72bde7ffbd6c06446aaefda4d1803f89eb1faf3167307ee1e01','flexe-vdp.so':'0866265bd6f3d73ad572f31dc1bb954f308ed9a5d857efa9414d37e1ade13374','esp32-firmware.bin':'201ad8ab4af807c261cb92634286b2fbaedcf3879f140e820b60ab54cf707e2a'}.items():assert h(common/n)==pin
spec=importlib.util.spec_from_file_location('factory',root.parent/'agon-dev-env/scripts/setup_emulator.py');factory=importlib.util.module_from_spec(spec);spec.loader.exec_module(factory)
rows=[]
for c in json.loads((build/'cases.json').read_text())['cases']:
 name=c['command'];source=build/name;fixture=source/'fixture'
 assert h(source/'mos.bin')==c['MOS_sha256'] and h(source/'mos.map')==c['map_sha256']
 for engine in ('jit','interp'):
  p=out/(name+'-'+engine);p.mkdir();(p/'sdcard').mkdir();files=('flexe-agon-emulator.bin','flexe-vdp.so','esp32-firmware.bin','esp32-firmware.elf')
  for n in files:(p/n).symlink_to('../runtime/'+n)
  shutil.copyfile(source/'mos.bin',p/'mos_platform.bin');shutil.copyfile(source/'mos.map',p/'mos_platform.map')
  for n in ('flexe-agon-emulator','.bespoke-vdp-profile'):shutil.copyfile(template/n,p/n)
  (p/'flexe-agon-emulator').chmod(0o755);(p/'.mos-release').write_text('PERF2 '+name+' pinned comparison firmware\n')
  (p/'.mos-release.sha256').write_text(''.join(h(p/n)+'  '+n+'\n' for n in ('mos_platform.bin','mos_platform.map')))
  card=p/'card.img'
  with card.open('xb') as f:f.truncate(32*1024*1024)
  subprocess.run(['mkfs.fat','-F','16',str(card)],check=True,stdout=subprocess.DEVNULL);subprocess.run(['mmd','-i',str(card),'::/uart0'],check=True)
  def put(src,dst):subprocess.run(['mcopy','-i',str(card),str(src),'::/'+dst],check=True)
  put(fixture/'bin'/(name+'.bin'),'uart0/'+name+'.bin');put(fixture/'checks.bin','uart0/checks.bin')
  a=p/'autoexec.txt';a.write_bytes(('CD /uart0\r\n'+name+'\r\nDIR\r\nECHO TM001 CLI RETURN\r\n').encode());put(a,'autoexec.txt')
  (p/'sdcard.img').symlink_to('card.img');factory.name_flexe_profile(p)
  (p/'.runtime.sha256').write_text(''.join(h(p/n)+'  '+n+'\n' for n in (*files,'mos_platform.bin','mos_platform.map','flexe-agon-emulator')))
  for pin in ('.runtime.sha256','.mos-release.sha256'):subprocess.run(['sha256sum','--check','--status',pin],cwd=p,check=True)
  rows.append(dict(path=str(p),variant=name,case=c['case'],transport=c['transport'],engine=engine,fixture=str(source.relative_to(b)),fixture_sha256=h(fixture/'bin'/(name+'.bin')),MOS_sha256=c['MOS_sha256'],map_sha256=c['map_sha256'],plain_command=name,stopped=True))
(out/'profiles.json').write_text(json.dumps(dict(suite='performance2',profiles=rows),indent=2)+'\n');print(out)
