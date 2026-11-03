"""Independent validation for the four added firmware/engine cells."""
from pathlib import Path
import hashlib,importlib.util,json,sys
root=Path(__file__).resolve().parents[3];sys.path.insert(0,str(root/'experiments/uart0-performance'))
from metrics import read_report
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
out=root/sys.argv[1];profiles=json.loads((out/'profiles.json').read_text());rows=json.loads((out/'results.json').read_text());assert len(rows)==4
assert {(r['variant'],r['engine']) for r in rows}=={(v,e) for v in ('stock','ours') for e in ('jit','interp')}
packet=b''.join(p.cmd(p.BASE,0,*p.word(256),*p.PAYLOAD[i:i+256]) for i in range(0,8192,256));checks=b''.join(p.compare(i,i) for i in range(8192))
cells=[]
for r in rows:
 assert r['passed'] and r['receiver_bytes']==49152
 source=root/'build/tm001'/r['fixture'];manifest=json.loads((source/'source.json').read_text())
 assert hashlib.sha256((source/'fixture/bin'/(r['variant']+'.bin')).read_bytes()).hexdigest()==r['fixture_sha256']
 cell=out/(r['variant']+'-'+r['engine']);raw=cell/'results/raw';d=read_report((cell/'results/reports/R0001.log').read_text());pin=manifest['mos'][r['mode']]
 assert d['mode']==r['mode'] and d['build']==manifest['build'] and d['mos_crc32']==pin['crc32'] and d['mos_bytes']==pin['size'] and r['MOS_sha256']==pin['sha256']
 wire=(raw/'mos-to-vdp.bin').read_bytes();assert wire.count(packet)==6
 for i in range(0,len(checks),60):assert wire.count(checks[i:i+60])==6,i
 stats=json.loads((raw/'stats.json').read_text());uart=json.loads((raw/'uart.json').read_text());assert not any(stats[k] for k in ('failed','unhandled','unregistered','unmapped'));assert uart['tx_drops']==uart['rx_overruns']==0
 cells.append(dict(case=r['variant'],engine=r['engine'],fixture_sha256=r['fixture_sha256'],data=d,receiver_verified=49152,uart=uart))
result=dict(passed=True,physical=False,performance_claim=False,cells=cells,receiver_verified=49152*4)
(out/'analysis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed=True,cells=4,receiver_verified=result['receiver_verified'],physical=False)))
