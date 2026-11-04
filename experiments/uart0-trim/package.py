"""Prepare a private, source-pinned matched trial appliance after model checks."""
from pathlib import Path
import hashlib,json,shutil,sys,zlib
root=Path(__file__).resolve().parents[2];base=root/'build/tm001'
build,functional,perf,out=(base/a for a in sys.argv[1:5]);out.mkdir(exist_ok=False)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
info=json.loads((build/'build-info.json').read_text());cases=json.loads((build/'perf/cases.json').read_text())
functional_results=json.loads((functional/'results.json').read_text());perf_results=json.loads((perf/'results.json').read_text())
assert len(functional_results)==2 and len(perf_results)==6
assert all(x['passed'] and not x['physical'] for x in functional_results+perf_results)
assert all(x['MOS_sha256']==info['steps'][-1]['image_sha256'] for x in functional_results)
assert {x['case'] for x in perf_results}=={x['case'] for x in cases['cases']}
for x in perf_results:
 c=next(c for c in cases['cases'] if c['case']==x['case'])
 assert x['MOS_sha256']==c['MOS_sha256'] and x['fixture_sha256']==c['app_sha256']
app=out/'appliance';app.mkdir();source=out/'source';source.mkdir()
def snapshot(src,dst):shutil.copytree(src,dst,ignore=shutil.ignore_patterns('bin','obj','build','__pycache__','.git'))
snapshot(build/'trim',source/'trial-mos')
snapshot(base/'rc1-003/firmware',source/'published-rc1-mos')
for n in ('functional','example'):snapshot(build/n,source/n)
shutil.copyfile(build/'functional/bin/test.bin',app/'test.bin');shutil.copyfile(build/'functional/checks.bin',app/'checks.bin');shutil.copyfile(build/'example/bin/example.bin',app/'example.bin')
shutil.copyfile(base/'rc1-003/fixture/bin/test.bin',app/'checkrc.bin')
for c in cases['cases']:
 location=build/'perf'/c['command'];fixture=location/'fixture'
 assert h(location/'mos.bin')==c['MOS_sha256'] and h(fixture/'bin'/c['app'])==c['app_sha256']
 assert h(fixture/'checks.bin')==h(app/'checks.bin')
 shutil.copyfile(location/'mos.bin',app/c['firmware']);shutil.copyfile(fixture/'bin'/c['app'],app/c['app'])
 c['source']=c['command']+'-source.json';shutil.copyfile(location/'source.json',app/c['source']);snapshot(fixture,source/(c['command']+'-perf'))
prior=base/'perf2-deploy001/package'
assert h(prior/'vdp-stock.bin')=='b807beef35823b13a0a056f11b7464cd1b1c6356dce0e4098b78ebe059ded35f'
shutil.copyfile(prior/'vdp-stock.bin',app/'vdp-stock.bin');licenses=app/'licenses';licenses.mkdir()
for p in prior.iterdir():
 if p.is_file() and ('LICENSE' in p.name or 'CREDITS' in p.name):shutil.copyfile(p,licenses/p.name)
shutil.copyfile(root/'LICENSE',licenses/'EIRENE-MOS-LICENSE.txt')
(app/'cases.json').write_text(json.dumps(cases,indent=2)+'\n');shutil.copyfile(build/'build-info.json',app/'build-info.json')
crc=zlib.crc32((app/'mos-trim.bin').read_bytes());size=(app/'mos-trim.bin').stat().st_size
readme=f"""UART0 SMALLER CANDIDATE - PRIVATE TRIM TRIAL1
Directory: /test/mos-uart0

Use STOCK Bistromathics VDP 2.16.0 for all three configurations.
If needed: flash vdp vdp-stock.bin
Let each flash finish and reboot as prompted. Keep normal updater checks.

A: flash mos mos-stock.bin
After reboot, return here and type: stock
Require PERF2 PASS and normal prompt.

B: flash mos mos-rc1.bin
Confirm Eirene UART0 MOS Version 3.1.0 RC 1.
Return here; type: checkrc  (require RC1 PASS)
Then type: rc1  (require PERF2 PASS)

C: flash mos mos-trim.bin
Confirm Eirene UART0 TRIM MOS Version 3.1.0 Trial 1.
Power off/on once and confirm that same banner/normal prompt.
Return here; type: test   (require TRIM1 PASS)
Type: test again         (require TRIM1 PASS)
Type: example            (require TRIM1 EXAMPLE PASS)
Type: trim               (require PERF2 PASS)

Apps take NO switches: type their names, no LOAD/RUN.
Allow two minutes per app; a stall or FAIL is failure. Record the last
progress line and keep the logs. Each performance app runs all three
scenarios (bulk, paced+calculation, bulk+calculation), three repeats each,
and checks every payload byte. Work counters measure calculation overlap;
smaller code does not guarantee greater speed. Exact-ROM guards refuse
mismatched firmware before any experimental syscall.

Rnnnn.log, next.txt and runs.log use sequential numbering, never the clock.
The example prints a result but creates no report. Reattach the card to Oryx
after all configurations. This trial is unpublished, not stock or approved.
Leave it installed between dedicated UART tests if desired.
Restore when needed: flash mos mos-stock.bin

Trial MOS: {size} bytes; CRC32 {crc:08X}
Source: {info['source_commit']}
The trial moves extra C callback saves into a29-byte fixed-target app gate;
foreground ADL1/MBASE0/EI and exclusive normal UART use are preconditions.
A short CLI readme replaces old on-card history; project archives retain it.
"""
(app/'readme.txt').write_bytes(readme.replace('\n','\r\n').encode())
(out/'README.md').write_text('# TR-001 private matched test package\n\n'+readme+'\nsource/ contains frozen trial/RC1 MOS, functional/example and each matched PERF2 application.\nBuild each with the pinned AgonDev compiler/libraries in appliance/build-info.json.\nThe trial MOS was reproduced from two clean builds. Published RC1 is unchanged.\nModel evidence is functional only; physical speed/stability remains pending.\n')
evidence=out/'evidence';evidence.mkdir()
for name,cohort in (('functional',functional),('perf',perf)):
 d=evidence/name;d.mkdir();shutil.copyfile(cohort/'results.json',d/'results.json');shutil.copyfile(cohort/'profiles.json',d/'profiles.json')
 for row in json.loads((cohort/'results.json').read_text()):shutil.copytree(Path(row['path'])/'results',d/Path(row['path']).name)
manifest={str(p.relative_to(app)):dict(bytes=p.stat().st_size,sha256=h(p)) for p in sorted(app.rglob('*')) if p.is_file()}
(app/'SHA256SUMS').write_text(''.join(v['sha256']+'  '+k+'\n' for k,v in manifest.items()))
(out/'package.json').write_text(json.dumps(dict(source_commit=info['source_commit'],MOS_sha256=h(app/'mos-trim.bin'),MOS_crc32=f'{crc:08X}',private=True,published=False,hardware_pending=True,files=manifest,model_cells=8,model_receiver_bytes=sum(x['receiver_bytes'] for x in functional_results+perf_results)),indent=2)+'\n')
print(out)
