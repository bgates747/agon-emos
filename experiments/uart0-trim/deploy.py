"""Deploy only the authorized Console8 UART folder after a verified archive."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,sys,tarfile
root=Path(__file__).resolve().parents[2];base=root/'build/tm001';package=base/sys.argv[1];out=base/sys.argv[2];out.mkdir(exist_ok=False)
mount=Path('/run/media/smith/AGONC8');target=mount/'test/mos-uart0'
fs=json.loads(subprocess.check_output(['findmnt','-J','-o','SOURCE,TARGET,LABEL,UUID,FSTYPE','--mountpoint',str(mount)],text=True))['filesystems'];assert len(fs)==1
fs=fs[0];assert fs['target']==str(mount) and fs['label']=='AGONC8' and fs['uuid']=='8658-1C23' and fs['fstype']=='vfat',fs
assert target.is_dir() and not target.is_symlink()
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def manifest(folder):return {str(p.relative_to(folder)):dict(bytes=p.stat().st_size,sha256=h(p)) for p in sorted(folder.rglob('*')) if p.is_file()}
subprocess.run(['sha256sum','--check','--status','SHA256SUMS'],cwd=package/'appliance',check=True)
before=manifest(target);shutil.copytree(target,out/'appliance');assert manifest(out/'appliance')==before
archive=out/'returned.tar.gz'
with tarfile.open(archive,'w:gz') as t:t.add(out/'appliance',arcname='appliance')
with tarfile.open(archive) as t:
 members=[m for m in t.getmembers() if m.isfile()];assert len(members)==len(before)
 for m in members:
  key=str(Path(m.name).relative_to('appliance'));assert hashlib.sha256(t.extractfile(m).read()).hexdigest()==before[key]['sha256']
next_id=int((target/'next.txt').read_text());reports=[int(p.stem[1:]) for p in target.glob('R[0-9][0-9][0-9][0-9].log')];assert 0<next_id<10000 and (not reports or next_id>max(reports))
journal=(target/'runs.log').read_bytes();expected=json.loads((package/'package.json').read_text())
assert manifest(target)==before, 'Card changed during archive; stop without deployment'
stage=target.with_name('mos-uart0.stage-tr001');backup=target.with_name('mos-uart0.old-tr001');assert not stage.exists() and not backup.exists()
shutil.copytree(package/'appliance',stage);(stage/'next.txt').write_bytes(f'{next_id}\r\n'.encode());(stage/'runs.log').write_bytes(journal)
for name,record in expected['files'].items():assert h(stage/name)==record['sha256'],name
subprocess.run(['sync','-f',str(stage)],check=True)
target.rename(backup)
try:
 stage.rename(target)
 subprocess.run(['sha256sum','--check','--status','SHA256SUMS'],cwd=target,check=True)
 assert int((target/'next.txt').read_text())==next_id and (target/'runs.log').read_bytes()==journal
 after=manifest(target);assert set(after)==set(manifest(package/'appliance'))|{'next.txt','runs.log'}
except BaseException:
 if target.exists():shutil.rmtree(target)
 backup.rename(target);raise
shutil.rmtree(backup);subprocess.run(['sync','-f',str(target)],check=True)
receipt=dict(volume=fs,target=str(target),archive_sha256=h(archive),archive_verified=True,source_package=str(package),source_commit=expected['source_commit'],next_report=next_id,prior_reports_archived=reports,running_log_preserved=True,card_files=after,only_UART_folder_modified=True,root_MOS_untouched=True,physical_firmware_flashed=False,remounted=False,ejected=False)
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(target=str(target),next_report=next_id,files=len(after),archive_verified=True,MOS_crc32=expected['MOS_crc32'])))
