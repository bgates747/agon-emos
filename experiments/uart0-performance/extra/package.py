"""Prepare an additive five-case appliance from a verified card backup."""
from pathlib import Path
import hashlib,json,shutil,sys,tarfile
root=Path(__file__).resolve().parents[3];build=root/sys.argv[1];delivery=root/sys.argv[2];out=delivery/'package';out.mkdir(exist_ok=False)
before=json.loads((delivery/'before.json').read_text());h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert h(delivery/'before.tar.gz')==before['archive_sha256']
with tarfile.open(delivery/'before.tar.gz','r:gz') as t:
 for n,pin in before['files'].items():
  relative=Path(n).relative_to('test/mos-uart0');assert len(relative.parts)==1
  data=t.extractfile(n).read();assert hashlib.sha256(data).hexdigest()==pin['sha256'];(out/relative).write_bytes(data)
assert not (out/'previous.txt').exists();shutil.copyfile(out/'readme.txt',out/'previous.txt')
for name in ('stock','ours'):
 f=build/name;shutil.copyfile(f/'fixture/bin'/(name+'.bin'),out/(name+'.bin'));shutil.copyfile(f/'source.json',out/(name+'.json'))
 if name=='ours':shutil.copyfile(f/'mos.bin',out/'mos-ours.bin')
assert h(out/'mos-stock.bin')==json.loads((out/'stock.json').read_text())['mos'][0]['sha256']
original=json.loads((out/'source.json').read_text())
cases=[]
for case,name,manifest,mode,app,firmware in (
 ('official-stock','Official stock MOS 3.0.2','stock.json',0,'stock.bin','mos-stock.bin'),
 ('eirene-stock','Pristine Eirene ordinary output','source.json',0,'test.bin','mos-eirene.bin'),
 ('eirene-scalar','Minimal Eirene scalar async','source.json',1,'test.bin','mos-scalar.bin'),
 ('eirene-otirx','Minimal Eirene OTIRX async','source.json',2,'test.bin','mos-otirx.bin'),
 ('previous-otirx','Previous full OTIRX borrowed/direct','ours.json',2,'ours.bin','mos-ours.bin')):
 m=json.loads((out/manifest).read_text());assert h(out/firmware)==m['mos'][mode]['sha256']
 cases.append(dict(id=case,name=name,manifest=manifest,mode=mode,build=m['build'],app=app,app_sha256=h(out/app),firmware=firmware))
(out/'cases.json').write_text(json.dumps(dict(schema='PERF1-five-cases-1',reference='official-stock',cases=cases,retained_reports=['R0042.log','R0043.log','R0044.log'],new_cases_only=['official-stock','previous-otirx']),indent=2)+'\n')
(out/'readme.txt').write_bytes(b'''UART PERFORMANCE - FIVE CASES IN ONE PACKAGE
Directory: /test/mos-uart0. Run ONLY the TWO NEW cases below.
Prior Eirene cases and reports R0042..R0044 are complete and retained.

Keep the matched vdp-test.bin VDP used for the previous three cases.
If it has been replaced since then: flash vdp vdp-test.bin, then reboot.
After every MOS flash/reboot, return to /test/mos-uart0.

NEW CASE A - official stock MOS 3.0.2:
  flash mos mos-stock.bin
  load stock.bin
  run . 0

NEW CASE B - our previous full OTIRX candidate, borrowed/direct callback:
  flash mos mos-ours.bin
  load ours.bin
  run . 2

Each app runs 3 bulk + 3 paced trials and receiver-byte checks.
Allow two minutes EACH; expect PERF1 PASS and the normal MOS prompt.
A stall is FAIL: record its last progress line. No firmware is flashed by apps.
Each app refuses the wrong selection and checks the installed MOS image.
Do not load test.bin for these new cases: it is the unchanged Eirene app.

The workload, calculation kernel, timing and receiver checks match the first
three cases. New apps supply the two firmware-specific API/identity adapters.
Our earlier candidate uses batch16, borrowed sources and direct callbacks;
copying RST18 is OFF. Firmware images were reused, not rebuilt.

New reports start R0045, then R0046 if both pass first time. Do not delete any
reports, runs.log or next.txt. Return the card to Oryx after both cases.
All five firmwares/apps and results stay in this directory for joint analysis.
Previous three-case instructions: previous.txt (no rerun requested).
Full file pins: SHA256SUMS. Explicit case/build registry: cases.json.
Restoration if leaving UART work: flash mos mos-stock.bin and, if needed,
flash vdp vdp-stock.bin; reboot as prompted and confirm normal banners.
'''.replace(b'\n',b'\r\n'))
(out/'SHA256SUMS').write_text(''.join(h(p)+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS'))
for n,pin in before['files'].items():
 f=out/Path(n).name
 if f.name not in ('readme.txt','SHA256SUMS'):assert h(f)==pin['sha256'],f
(delivery/'prepared.json').write_text(json.dumps(dict(unchanged_prior_files=True,previous_archive_sha256=before['archive_sha256'],next_report=int((out/'next.txt').read_text()),files={p.name:dict(bytes=p.stat().st_size,sha256=h(p)) for p in sorted(out.iterdir()) if p.is_file()}),indent=2)+'\n')
print(json.dumps(dict(path=str(out),files=len(list(out.iterdir())),next_report=45,prior_reports_preserved=True)))
