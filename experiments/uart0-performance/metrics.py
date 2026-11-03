"""Pure report validation, shared by model and physical result analysis."""
import re
def fields(line):return {k:int(v,16 if k=='state' else 10) for k,v in re.findall(r'(\w+)=([0-9A-F]+)',line)}
def checksum(units):
 x=0x12345678
 for _ in range(units*16):
  x^=(x<<13)&0xffffffff;x^=x>>17;x^=(x<<5)&0xffffffff
 return x
def clock_ok(d):return d['ticks']>0 and abs(d['ticks']-16*d['wall'])<=d['ticks']//100+64

def read_report(report):
 assert report.count('PERF1 PASS')==1 and 'PERF1 FAIL' not in report
 assert report.count('VERIFY8192 PASS')==6 and 'Comparator negative/positive PASS' in report
 mode=int(re.search(r'^MODE ([012]);',report,re.M)[1])
 build=re.search(r'^PERF1 build=([0-9a-f]{16})$',report,re.M)[1]
 pin=re.search(r'^MOS bytes=(\d+) CRC32=([0-9A-F]{8}) expected=([0-9A-F]{8})$',report,re.M)
 assert pin and pin[2]==pin[3]
 cal=fields(next(l for l in report.splitlines() if l.startswith('CAL ')))
 assert clock_ok(cal) and 0<cal['work']<1000000 and checksum(cal['work'])==cal['state']
 trials=[fields(l) for l in report.splitlines() if l.startswith('DATA ')]
 assert len(trials)==6 and {(d['rep'],d['paced']) for d in trials}=={(r,p) for r in (1,2,3) for p in (0,1)}
 for d in trials:
  assert clock_ok(d) and 0<=d['work']<1000000 and checksum(d['work'])==d['state']
  assert d['callbacks']==(0 if mode==0 else (32 if d['paced'] else 1))
  assert d['call']<=d['ticks'] and d['source_observed']<=d['ticks']
  assert (d['work']>0 and d['ticks']>=213996) if d['paced'] else d['work']==0
  d['payload_Bps']=8192*1152000/d['ticks'];d['wire_Bps']=8448*1152000/d['ticks']
  d['foreground_capacity_fraction']=(d['work']/d['ticks'])/(cal['work']/cal['ticks'])
 return dict(mode=mode,build=build,mos_bytes=int(pin[1]),mos_crc32=int(pin[2],16),calibration=cal,trials=trials)
