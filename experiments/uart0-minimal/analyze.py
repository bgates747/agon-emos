"""Independent byte-stream/health checks for the finite actual-firmware cells."""
from pathlib import Path
import hashlib,json,sys
from packets import BASE,PAYLOAD,cmd,word
root=Path(__file__).resolve().parents[2];out=(root/sys.argv[1]).resolve()
rows=json.loads((out/'results.json').read_text());assert len(rows)==6
assert {(r['variant'],r['engine']) for r in rows}=={(v,e) for v in ('baseline','scalar003','otirx003') for e in ('jit','interp')}
packet=bytes((23,0,128,0x51))+b''.join(cmd(BASE,0,*word(256),*PAYLOAD[n:n+256]) for n in range(0,8192,256))
check=root/'build/tm001/fixture003/fixture/checks.bin'
# Normative command construction is separate from the application packet builder.
checks=check.read_bytes() if check.exists() else b''.join(cmd(BASE+1,6,0x23,*word(BASE),*word(n),*word(BASE+4),*word(n)) for n in range(8192))
summary=[]
for r in rows:
 p=out/(r['variant']+'-'+r['engine']);raw=p/'results/raw';wire=(raw/'mos-to-vdp.bin').read_bytes()
 assert r['passed'] and r['receiver_bytes']==8192
 assert wire.count(packet)==1,(r['variant'],r['engine'],'payload framing/count')
 for i in range(0,len(checks),60):assert wire.count(checks[i:i+60])==1,('receiver comparator command',i)
 if r['variant']!='baseline':
  assert wire.count(bytes(1842))==1 and bytes(1843) not in wire,'boundary/null byte count'
 stats=json.loads((raw/'stats.json').read_text());uart=json.loads((raw/'uart.json').read_text())
 assert not any(stats[k] for k in ('failed','unhandled','unregistered','unmapped'))
 assert uart['tx_drops']==uart['rx_overruns']==0
 normalized=wire;boot_probes=0
 while normalized.startswith(bytes((23,0,128,1))):normalized=normalized[4:];boot_probes+=1
 summary.append(dict(boot_probes=boot_probes,post_boot_wire_sha256=hashlib.sha256(normalized).hexdigest(),variant=r['variant'],engine=r['engine'],passed=True,receiver_payload=8192,command_bytes=len(packet),boundary_bytes=1842 if r['variant']!='baseline' else 0,uart=uart,wire_bytes=len(wire),wire_sha256=hashlib.sha256(wire).hexdigest()))
assert len({r['post_boot_wire_sha256'] for r in summary if r['variant']!='baseline'})==1,'Async streams differ after boot probes'
result=dict(async_post_boot_streams_identical=True,passed=True,cells=summary,receiver_bytes=8192*6,async_cells=4,physical=False,keyboard_traffic_tested=False,forced_CTS_stall_tested=False,performance_claim=False)
(out/'analysis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
