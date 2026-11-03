"""Validate finite performance records; never treat emulator timing as hardware."""
from pathlib import Path
import hashlib,importlib.util,json,sys
from metrics import read_report
root=Path(__file__).resolve().parents[2];out=(root/sys.argv[1]).resolve()
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
profiles=json.loads((out/'profiles.json').read_text());assert profiles['suite']=='performance'
fixture=root/'build/tm001'/profiles['fixture'];manifest=json.loads((fixture/'source.json').read_text())
assert hashlib.sha256((fixture/'fixture/bin/test.bin').read_bytes()).hexdigest()==profiles['fixture_sha256']
rows=json.loads((out/'results.json').read_text());assert len(rows)==6
assert {(r['variant'],r['engine']) for r in rows}=={(v,e) for v in ('baseline','scalar003','otirx003') for e in ('jit','interp')}
packet=b''.join(p.cmd(p.BASE,0,*p.word(256),*p.PAYLOAD[i:i+256]) for i in range(0,8192,256))
checks=b''.join(p.compare(i,i) for i in range(8192))
cells=[]
for row in rows:
 assert row['passed'] and row['receiver_bytes']==49152
 cell=out/(row['variant']+'-'+row['engine']);raw=cell/'results/raw'
 report=(cell/'results/reports/R0001.log').read_text()
 data=read_report(report);cal=data['calibration'];trials=data['trials']
 assert data['mode']==('baseline','scalar003','otirx003').index(row['variant'])
 pin=manifest['mos'][data['mode']]
 assert data['build']==manifest['build'] and data['mos_bytes']==pin['size'] and data['mos_crc32']==pin['crc32']
 assert row['MOS_sha256']==pin['sha256']
 wire=(raw/'mos-to-vdp.bin').read_bytes();assert wire.count(packet)==6,'Timed payload stream count/content'
 for i in range(0,len(checks),60):assert wire.count(checks[i:i+60])==6,('Comparator command count',i)
 stats=json.loads((raw/'stats.json').read_text());uart=json.loads((raw/'uart.json').read_text())
 assert not any(stats[k] for k in ('failed','unhandled','unregistered','unmapped'))
 assert uart['tx_drops']==uart['rx_overruns']==0
 cells.append(dict(variant=row['variant'],engine=row['engine'],calibration=cal,trials=trials,receiver_verified=49152,wire_sha256=hashlib.sha256(wire).hexdigest(),uart=uart))
summary=dict(passed=True,physical=False,performance_claim=False,cells=cells,receiver_verified=49152*6,firmware_unchanged=True,build=manifest['build'],fixture_sha256=profiles['fixture_sha256'])
(out/'analysis.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(dict(passed=True,cells=len(cells),receiver_verified=summary['receiver_verified'],physical=False)))
