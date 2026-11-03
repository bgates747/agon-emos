"""Validate finite performance records; never treat emulator timing as hardware."""
from pathlib import Path
import hashlib,importlib.util,json,re,statistics,sys
root=Path(__file__).resolve().parents[2];out=(root/sys.argv[1]).resolve()
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
rows=json.loads((out/'results.json').read_text());assert len(rows)==6
assert {(r['variant'],r['engine']) for r in rows}=={(v,e) for v in ('baseline','scalar003','otirx003') for e in ('jit','interp')}
packet=b''.join(p.cmd(p.BASE,0,*p.word(256),*p.PAYLOAD[i:i+256]) for i in range(0,8192,256))
checks=b''.join(p.compare(i,i) for i in range(8192))
def fields(line):return {k:int(v,16 if k=='state' else 10) for k,v in re.findall(r'(\w+)=([0-9A-F]+)',line)}
def checksum(units):
 x=0x12345678
 for _ in range(units*16):
  x^=(x<<13)&0xffffffff;x^=x>>17;x^=(x<<5)&0xffffffff
 return x
def clock_ok(d):return d['ticks']>0 and abs(d['ticks']-16*d['wall'])<=d['ticks']//100+64
cells=[]
for row in rows:
 assert row['passed'] and row['receiver_bytes']==49152
 cell=out/(row['variant']+'-'+row['engine']);raw=cell/'results/raw'
 report=(cell/'results/reports/R0001.log').read_text()
 assert report.count('PERF1 PASS')==1 and report.count('VERIFY8192 PASS')==6
 assert 'Comparator negative/positive PASS' in report
 cal=fields(next(l for l in report.splitlines() if l.startswith('CAL ')))
 assert clock_ok(cal) and cal['work']>0 and checksum(cal['work'])==cal['state']
 trials=[fields(l) for l in report.splitlines() if l.startswith('DATA ')]
 assert len(trials)==6 and {(d['rep'],d['paced']) for d in trials}=={(r,p) for r in (1,2,3) for p in (0,1)}
 for d in trials:
  assert clock_ok(d) and checksum(d['work'])==d['state']
  assert d['callbacks']==(0 if row['variant']=='baseline' else (32 if d['paced'] else 1))
  assert d['call']<=d['ticks'] and d['source_observed']<=d['ticks']
  assert (d['work']>0 and d['ticks']>=213996) if d['paced'] else d['work']==0
  d['payload_Bps']=8192*1152000/d['ticks'];d['wire_Bps']=8448*1152000/d['ticks']
  d['foreground_capacity_fraction']=(d['work']/d['ticks'])/(cal['work']/cal['ticks'])
 wire=(raw/'mos-to-vdp.bin').read_bytes();assert wire.count(packet)==6,'Timed payload stream count/content'
 for i in range(0,len(checks),60):assert wire.count(checks[i:i+60])==6,('Comparator command count',i)
 stats=json.loads((raw/'stats.json').read_text());uart=json.loads((raw/'uart.json').read_text())
 assert not any(stats[k] for k in ('failed','unhandled','unregistered','unmapped'))
 assert uart['tx_drops']==uart['rx_overruns']==0
 cells.append(dict(variant=row['variant'],engine=row['engine'],calibration=cal,trials=trials,receiver_verified=49152,wire_sha256=hashlib.sha256(wire).hexdigest(),uart=uart))
summary=dict(passed=True,physical=False,performance_claim=False,cells=cells,receiver_verified=49152*6,firmware_unchanged=True)
(out/'analysis.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(dict(passed=True,cells=len(cells),receiver_verified=summary['receiver_verified'],physical=False)))
