"""Independent wire/report validation of the ten real-firmware model cells."""
from pathlib import Path
import hashlib,importlib.util,json,sys
from metrics import read_report
root=Path(__file__).resolve().parents[2];out=root/sys.argv[1];rows=json.loads((out/'results.json').read_text());assert len(rows)==10
assert {(r['variant'],r['engine']) for r in rows}=={(v,e) for v in ('stock','eirene','scalar','otirx','ours') for e in ('jit','interp')}
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
packet=b''.join(p.cmd(p.BASE,0,*p.word(256),*p.PAYLOAD[i:i+256]) for i in range(0,8192,256));checks=b''.join(p.compare(i,i) for i in range(8192));cells=[]
for r in rows:
 assert r['passed'] and r['receiver_bytes']==73728
 source=root/'build/tm001'/r['fixture'];manifest=json.loads((source/'source.json').read_text())
 assert hashlib.sha256((source/'fixture/bin'/(r['variant']+'.bin')).read_bytes()).hexdigest()==r['fixture_sha256']
 cell=out/(r['variant']+'-'+r['engine']);raw=cell/'results/raw';report=(cell/'results/reports/R0001.log').read_text();d=read_report(report)
 assert d['build']==manifest['build'] and d['case']==manifest['case'] and d['config']['transport']==manifest['transport']
 assert d['mos_crc32']==manifest['mos_crc'] and d['mos_bytes']==manifest['mos_bytes'] and r['MOS_sha256']==manifest['MOS_sha256']
 wire=(raw/'mos-to-vdp.bin').read_bytes();assert wire.count(packet)==9
 for i in range(0,len(checks),60):assert wire.count(checks[i:i+60])==9,i
 stats=json.loads((raw/'stats.json').read_text());uart=json.loads((raw/'uart.json').read_text());assert not any(stats[k] for k in ('failed','unhandled','unregistered','unmapped'));assert uart['tx_drops']==uart['rx_overruns']==0
 assert (cell/'autoexec.txt').read_bytes()==('CD /uart0\r\n'+r['variant']+'\r\nDIR\r\nECHO TM001 CLI RETURN\r\n').encode()
 cells.append(dict(command=r['variant'],engine=r['engine'],fixture_sha256=r['fixture_sha256'],data=d,receiver_verified=73728,uart=uart))
result=dict(passed=True,physical=False,performance_claim=False,plain_commands_tested=True,cells=cells,receiver_verified=73728*10)
(out/'analysis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed=True,cells=10,receiver_verified=result['receiver_verified'],physical=False)))
