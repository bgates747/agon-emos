"""Oryx: prepare one new manual appliance, without accessing an SD card."""
from pathlib import Path
import hashlib,json,re,shutil,sys
root=Path(__file__).resolve().parents[2];build=root/sys.argv[1];out=root/sys.argv[2]
commit=sys.argv[3];assert re.fullmatch('[0-9a-f]{40}',commit),'Supply the full reviewed fixture source commit'

old=root.parent/'agon-mos/build/uart0-balls/run007/appliance'
source=json.loads((build/'source.json').read_text());f=build/'fixture'
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
items=[]
for src,dst,pin in [
 (old/'vdp-test.bin','vdp-test.bin','201ad8ab4af807c261cb92634286b2fbaedcf3879f140e820b60ab54cf707e2a'),
 (old/'vdp-2.16.0.bin','vdp-stock.bin','b807beef35823b13a0a056f11b7464cd1b1c6356dce0e4098b78ebe059ded35f'),
 (old/'mos-stock.bin','mos-stock.bin','d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae')]:
 assert h(src)==pin;items.append((src,dst))
for m,dst in zip(source['mos'],('mos-eirene.bin','mos-scalar.bin','mos-otirx.bin')):
 src=root/'build/tm001'/m['name']/'bin/mos.bin';assert h(src)==m['sha256'];items.append((src,dst))
for src,dst in ((f/'bin/test.bin','test.bin'),(f/'checks.bin','checks.bin'),(root/'LICENSE','MOS-LICENSE.txt'),(root/'experiments/uart0-performance/LICENSE','TEST-LICENSE.txt'),(build/'source.json','source.json')):items.append((src,dst))
for n in ('VDP-LICENSE.txt','DIAGNOSTIC-VDP-LICENSE.txt','ESP-IDF-LICENSE','RX-FIX-CREDITS.txt','CREDITS.txt'):items.append((old/n,n))
for src,dst in items:assert src.is_file(),src
out.mkdir(parents=True,exist_ok=False)
for src,dst in items:shutil.copyfile(src,out/dst)
(out/'appliance.json').write_text(json.dumps(dict(fixture_source_commit=commit,build=source['build'],physical_tested=False,SD_written=False),indent=2)+'\n')
(out/'readme.txt').write_text('''EIRENE UART PERFORMANCE COMPARISON - PERF1
Experimental firmware. This directory is one appliance; keep reports here.

Use the installed standard flash MOSlet; all filenames below are explicit.
No root /MOS.bin alias is used. Each image has normal startup; only the async
runs activate the sender. The experimental scalar/OTIRX banner is identical;
the app distinguishes their exact installed ROMs using its pinned CRC32.

From THIS directory:
  flash vdp vdp-test.bin
Reboot as requested. This is the matched receiver-fixed 2.16 VDP, not stock.
Then do each group in order, returning to THIS directory after each reboot:

  flash mos mos-eirene.bin
  load test.bin
  run . 0

  flash mos mos-scalar.bin
  load test.bin
  run . 1

  flash mos mos-otirx.bin
  load test.bin
  run . 2

For each run allow two minutes. The app clears the screen, reports bulk/paced
repeat and receiver-check progress, then PERF1 PASS and a normal MOS prompt.
A stall is FAIL: note its last line; do not count a silent run as PASS.
The app refuses a selection if installed MOS does not match that exact image.
Do not reuse earlier UTX1 apps against this different Eirene interface.

Each run performs three bulk + three paced samples and verifies every received
payload byte. Paced means 256-payload-byte chunks averaging44100payload B/s,
not a44100Hz single-byte/audio scheduling test. Logging/checks are untimed.
Rnnnn.log, next.txt and runs.log preserve sequential history; reruns get a fresh
number. Leave all reports for analysis; do not rename them between images.
Return the card to Oryx after the three selections so we can compare CPU
headroom and bulk rate. There is no need to time it with a stopwatch.

Stock restoration remains available if switching to another project:
  flash mos mos-stock.bin
  flash vdp vdp-stock.bin
Reboot as requested and confirm normal banners. The Author may instead leave
this experimental pair installed while Console8 is dedicated to UART work.

Source, provenance and SHA256 pins: source.json and SHA256SUMS.
Application source is experiments/uart0-performance in bgates747/eirene-mos,
branch codex/uart0-async. MOS sources: pristine86b694260e3be3b391284ce4aa8ecc758f7f34e9,
experimentalb487e10b7d755ee94ac9516c85a9d828fd3ccd17; scalar changes only
UART0_ASYNC_SCALAR equ0 to1. Test build: '''+source['build']+'\nFixture source commit: '+commit+'\n')
(out/'SHA256SUMS').write_text(''.join(h(p)+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file()))
print(json.dumps(dict(path=str(out),build=source['build'],files=len(list(out.iterdir())),SD_written=False)))
