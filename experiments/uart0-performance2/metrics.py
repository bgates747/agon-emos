"""Validate PERF2 without reinterpreting historical PERF1 work-unit counts."""
import re

def fields(line):return {k:int(v,16 if k=='state' else 10) for k,v in re.findall(r'(\w+)=([0-9A-F]+)',line)}
def checksum(units):
 x=0x12345678
 for _ in range(units):
  x^=(x<<13)&0xffffffff;x^=x>>17;x^=(x<<5)&0xffffffff
 return x

def clock_ok(d):return d['ticks']>0 and abs(d['ticks']-16*d['wall'])<=d['ticks']//100+64

def read_report(report):
 assert report.count('PERF2 PASS')==1 and 'PERF2 FAIL' not in report
 assert report.count('VERIFY8192 PASS')==9 and 'Comparator negative/positive PASS' in report
 header=re.search(r'^PERF2 build=([0-9a-f]{16}) case=([a-z0-9-]+)$',report,re.M);assert header
 config=fields(next(l for l in report.splitlines() if l.startswith('CONFIG ')))
 assert config['transport'] in (0,1,2) and config['work_steps']==1 and config['tick_hz']==1152000 and config['wall_hz']==72000
 assert config['payload']==8192 and config['wire']==8448 and config['window']==213996
 pin=re.search(r'^MOS bytes=(\d+) CRC32=([0-9A-F]{8}) expected=([0-9A-F]{8})$',report,re.M);assert pin and pin[2]==pin[3]
 cal=fields(next(l for l in report.splitlines() if l.startswith('CAL ')))
 assert clock_ok(cal) and 0<cal['work']<1000000 and checksum(cal['work'])==cal['state']
 trials=[fields(l) for l in report.splitlines() if l.startswith('DATA ')]
 assert len(trials)==9 and {(d['rep'],d['scenario']) for d in trials}=={(r,s) for r in (1,2,3) for s in (0,1,2)}
 for d in trials:
  assert clock_ok(d) and 0<=d['work']<1000000 and checksum(d['work'])==d['state']
  assert d['callbacks']==(0 if config['transport']==0 else (32 if d['scenario']==1 else 1))
  assert d['call']<=d['ticks'] and d['source_observed']<=d['ticks']
  assert d['credited']==(max(0,d['work']-1) if d['scenario']==2 else d['work'])
  if d['scenario']==0:assert d['work']==0 and d['missed']==d['late']==0
  if d['scenario']==1:assert d['ticks']>=213996
  else:assert d['missed']==d['late']==0
  d['payload_Bps']=8192*1152000/d['ticks'];d['wire_Bps']=8448*1152000/d['ticks']
  d['work_per_second']=d['credited']*1152000/d['ticks']
  d['foreground_capacity_fraction']=(d['credited']/d['ticks'])/(cal['work']/cal['ticks'])
 return dict(build=header[1],case=header[2],config=config,mos_bytes=int(pin[1]),mos_crc32=int(pin[2],16),calibration=cal,trials=trials)
