"""Validate exact-image RC1 model reports and full receiver streams."""
from pathlib import Path
import hashlib,importlib.util,json,re,sys
root=Path(__file__).resolve().parents[2];base=root/'build/tm001';out=base/sys.argv[1];rows=json.loads((out/'results.json').read_text());assert len(rows)==2 and {r['engine'] for r in rows}=={'jit','interp'}
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
packet=bytes((23,0,128,0x51))+b''.join(p.cmd(p.BASE,0,*p.word(256),*p.PAYLOAD[n:n+256]) for n in range(0,8192,256));checks=b''.join(p.compare(n,n) for n in range(8192));cells=[]
for r in rows:
 assert r['passed'];profile=out/('rc1-'+r['engine']);reports=profile/'results/reports';wire=(profile/'results/raw/mos-to-vdp.bin').read_bytes()
 assert b'Eirene UART0 MOS Version 3.1.0 RC 1' in wire and b'RC1 EXAMPLE PASS' in wire and b'TM001 CLI RETURN' in wire
 for n in (1,2):
  text=(reports/f'R{n:04}.log').read_text();assert text.count('RC1 PASS')==1 and 'FAIL' not in text
  assert 'CRC32='+r['MOS_crc32']+' expected='+r['MOS_crc32'] in text
  for marker in ('MODE async','Ordinary RST18 before opt-in PASS','Ordinary RST18 after finish PASS','Boundary lengths1/15/16/17/255/256/257/1024 and NULL callback PASS','Zero-length/masked-context refusal PASS','Single source/BUSY/callback/input-hook/RX-during-TX PASS','VERIFY8192 PASS'):assert marker in text,marker
 assert int((reports/'next.txt').read_text())==3
 assert (reports/'runs.log').read_text().count(' PASS RC1 ')==2
 assert wire.count(packet)==2
 for i in range(0,len(checks),60):assert wire.count(checks[i:i+60])==2,i
 assert wire.count(bytes(1842))==2 and bytes(1843) not in wire
 example=re.search(rb'RC1 EXAMPLE PASS; work=(\d+) state=([0-9A-F]{8})',wire);assert example
 state=0x12345678
 for _ in range(int(example[1])):state^=(state<<13)&0xffffffff;state^=state>>17;state^=(state<<5)&0xffffffff
 assert state==int(example[2],16)
 stats=json.loads((profile/'results/raw/stats.json').read_text());uart=json.loads((profile/'results/raw/uart.json').read_text());assert not any(stats[k] for k in ('failed','unhandled','unregistered','unmapped'));assert uart['tx_drops']==uart['rx_overruns']==0
 cells.append(dict(engine=r['engine'],passed=True,receiver_bytes=16384,MOS_sha256=r['MOS_sha256'],example_work=int(example[1]),wire_sha256=hashlib.sha256(wire).hexdigest(),uart=uart))
result=dict(passed=True,physical=False,cells=cells,receiver_bytes=32768,plain_commands=True,example_checksum_valid=True);(out/'analysis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
