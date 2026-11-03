"""Analyze the complete five-case appliance across explicit fixture build IDs."""
from pathlib import Path
import argparse,hashlib,json,statistics,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from metrics import read_report
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('--allow-incomplete',action='store_true');parser.add_argument('--kind',choices=('hardware','emulator'),default='emulator');args=parser.parse_args()
root=args.directory;registry=json.loads((root/'cases.json').read_text());cases=registry['cases'];groups={c['id']:[] for c in cases};runs=[]
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for path in sorted(root.glob('R[0-9][0-9][0-9][0-9].log')):
 s=path.read_text()
 if 'PERF1 build=' not in s:continue
 d=read_report(s);matches=[c for c in cases if c['build']==d['build'] and c['mode']==d['mode']];assert len(matches)==1,(path,'Unregistered build/mode')
 c=matches[0];manifest=json.loads((root/c['manifest']).read_text());pin=manifest['mos'][d['mode']]
 assert d['mos_crc32']==pin['crc32'] and d['mos_bytes']==pin['size']
 assert h(root/c['app'])==c['app_sha256'] and h(root/c['firmware'])==pin['sha256']
 groups[c['id']].extend(d['trials']);runs.append(dict(file=path.name,case=c['id']))
missing=[c['id'] for c in cases if not groups[c['id']]]
assert args.allow_incomplete or not missing,('Missing cases',missing)
rows=[]
for c in cases:
 trials=groups[c['id']]
 if not trials:continue
 bulk=[d for d in trials if not d['paced']];paced=[d for d in trials if d['paced']]
 rows.append(dict(case=c['id'],name=c['name'],samples=len(trials),bulk_payload_Bps=statistics.median(d['payload_Bps'] for d in bulk),bulk_wire_Bps=statistics.median(d['wire_Bps'] for d in bulk),paced_work_per_second=statistics.median(d['work']*1152000/d['ticks'] for d in paced),foreground_capacity_fraction=statistics.median(d['foreground_capacity_fraction'] for d in paced),missed_deadlines=sum(d['missed'] for d in paced)))
reference=next((r for r in rows if r['case']==registry['reference']),None)
for r in rows:
 r['bulk_gain_percent']=100*(r['bulk_wire_Bps']/reference['bulk_wire_Bps']-1) if reference else None
 r['work_gain_percent']=100*(r['paced_work_per_second']/reference['paced_work_per_second']-1) if reference else None
result=dict(kind=args.kind,complete=not missing,missing_cases=missing,reference=registry['reference'],runs=runs,rows=rows)
(root/'five-case-comparison.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
