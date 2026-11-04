"""Prepare the five-command appliance, preserving the complete prior package."""
from pathlib import Path
import hashlib,json,shutil,sys,tarfile
root=Path(__file__).resolve().parents[2];build=root/sys.argv[1];delivery=root/sys.argv[2];out=delivery/'package';out.mkdir(exist_ok=False)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
before=json.loads((delivery/'before.json').read_text());assert h(delivery/'before.tar.gz')==before['archive_sha256']
previous=out/'previous';previous.mkdir()
with tarfile.open(delivery/'before.tar.gz','r:gz') as t:
 for n,pin in before['files'].items():
  rel=Path(n).relative_to('test/mos-uart0');assert len(rel.parts)==1
  data=t.extractfile(n).read();assert hashlib.sha256(data).hexdigest()==pin['sha256'];(previous/rel).write_bytes(data)
registry=json.loads((build/'cases.json').read_text());cases=[]
for c in registry['cases']:
 command=c['command'];source=build/command
 shutil.copyfile(source/'fixture/bin'/(command+'.bin'),out/(command+'.bin'))
 shutil.copyfile(source/'source.json',out/(command+'.json'));shutil.copyfile(source/'mos.bin',out/c['firmware'])
 assert h(out/c['firmware'])==c['MOS_sha256']
 cases.append(dict(case=c['case'],command=command,name=c['name'],build=c['build'],transport=c['transport'],app=command+'.bin',app_sha256=h(out/(command+'.bin')),source=command+'.json',firmware=c['firmware'],MOS_sha256=c['MOS_sha256']))
shutil.copyfile(build/'stock/fixture/checks.bin',out/'checks.bin')
for n in ('vdp-test.bin','vdp-stock.bin','CREDITS.txt','DIAGNOSTIC-VDP-LICENSE.txt','ESP-IDF-LICENSE','MOS-LICENSE.txt','RX-FIX-CREDITS.txt','TEST-LICENSE.txt','VDP-LICENSE.txt','next.txt','runs.log'):
 shutil.copyfile(previous/n,out/n)
for p in previous.glob('R[0-9][0-9][0-9][0-9].log'):shutil.copyfile(p,out/p.name)
with (out/'runs.log').open('ab') as f:f.write(b'PERF2 campaign: 5 no-argument fixtures, 9 trials each; complete PERF1 package in previous/.\r\n')
(out/'cases.json').write_text(json.dumps(dict(schema='PERF2-five-1',reference='official-stock',report_version=2,work_steps=1,cases=cases,previous_package='previous/',first_report=int((out/'next.txt').read_text())),indent=2)+'\n')
shutil.copyfile(build/'core-check.json',out/'core-check.json')
(out/'readme.txt').write_bytes(b'''UART PERFORMANCE SUITE - FIVE SIMPLE COMMANDS
Directory: /test/mos-uart0
Each app runs ALL scenarios automatically. No switches, LOAD or RUN needed.

Keep the matched vdp-test.bin VDP. If it was replaced since the last UART run,
use: flash vdp vdp-test.bin, then reboot as prompted.

Flash the MOS for each row. After reboot, return to /test/mos-uart0 and type
ONLY the app name in the right column:

MOS flash command                 App command
flash mos mos-stock.bin            stock
flash mos mos-eirene.bin           eirene
flash mos mos-scalar.bin           scalar
flash mos mos-otirx.bin            otirx
flash mos mos-ours.bin             ours

Each fixture identifies its installed MOS and refuses a mismatched image.
It performs 3 repeats of EACH scenario:
  Bulk only: send as fast as possible without foreground calculation.
  Paced + calculation: average 44100 payload bytes/second while calculating.
  Bulk + calculation: send flat out and calculate until the UART drains.
Every trial checks all 8192 received payload bytes. Disk logging/checking is
outside the timed interval. Calibration and checksums validate useful work.

Allow about one minute per app; if longer than TWO MINUTES something is wrong.
Require PERF2 PASS and return to the normal MOS prompt. A stall is FAIL;
record its last progress line. No firmware is flashed automatically.

Run all five apps once against their matching MOS. New reports start R0047
and share next.txt/runs.log. Leave every report for analysis, including failures.
PERF2 uses a finer work unit; do not merge its work counts with PERF1 results.
The entire earlier appliance and versions remain in previous/; old reports
are also retained here. All five new fixtures are needed for the NEW scenario.

Return the card to Oryx after all five. Stock restoration, when leaving UART:
flash mos mos-stock.bin; flash vdp vdp-stock.bin; reboot as prompted.
Source/build/image pins: cases.json, per-app JSON files and SHA256SUMS.
'''.replace(b'\n',b'\r\n'))
(out/'SHA256SUMS').write_text(''.join(h(p)+'  '+str(p.relative_to(out))+'\n' for p in sorted(out.rglob('*')) if p.is_file() and p!=out/'SHA256SUMS'))
manifest={str(p.relative_to(out)):dict(bytes=p.stat().st_size,sha256=h(p)) for p in sorted(out.rglob('*')) if p.is_file()}
(delivery/'prepared.json').write_text(json.dumps(dict(files=manifest,first_report=int((out/'next.txt').read_text()),prior_archive_sha256=before['archive_sha256'],prior_package_preserved=True),indent=2)+'\n')
print(json.dumps(dict(path=str(out),files=len(manifest),commands=[c['command'] for c in cases],first_report=int((out/'next.txt').read_text()))))
