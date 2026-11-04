"""Summarize the three scenarios from complete no-argument PERF2 reports."""
from pathlib import Path
import argparse,hashlib,json,statistics
from metrics import read_report
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('--kind',choices=('hardware','emulator'),default='emulator');parser.add_argument('--allow-incomplete',action='store_true');args=parser.parse_args()
root=args.directory;registry=json.loads((root/'cases.json').read_text());cases=registry['cases'];groups={c['case']:[] for c in cases};runs=[];failed=[];historical=[]
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for path in sorted(root.glob('R[0-9][0-9][0-9][0-9].log')):
 s=path.read_text()
 if 'PERF2 build=' not in s:historical.append(path.name);continue
 if 'PERF2 FAIL' in s or 'PERF2 PASS' not in s:
  failed.append(dict(file=path.name,sha256=h(path),reason='Failed or incomplete PERF2 attempt; retained for inspection'));continue
 d=read_report(s);matches=[c for c in cases if c['case']==d['case'] and c['build']==d['build']];assert len(matches)==1,(path,'Unknown PERF2 build/case')
 c=matches[0];manifest=json.loads((root/c['source']).read_text())
 assert d['mos_crc32']==manifest['mos_crc'] and d['mos_bytes']==manifest['mos_bytes'] and d['config']['transport']==manifest['transport']
 assert h(root/c['app'])==c['app_sha256'] and h(root/c['firmware'])==c['MOS_sha256']
 groups[c['case']].extend(d['trials']);runs.append(dict(file=path.name,case=c['case'],build=c['build']))
missing=[c['case'] for c in cases if not groups[c['case']]];assert args.allow_incomplete or not missing,('Missing cases',missing)
rows=[]
for c in cases:
 trials=groups[c['case']]
 if not trials:continue
 scenarios=[]
 for scenario in (0,1,2):
  data=[d for d in trials if d['scenario']==scenario]
  scenarios.append(dict(scenario=scenario,name=('bulk-only','paced-plus-work','bulk-plus-work')[scenario],samples=len(data),payload_Bps=statistics.median(d['payload_Bps'] for d in data),wire_Bps=statistics.median(d['wire_Bps'] for d in data),credited_work_per_second=statistics.median(d['work_per_second'] for d in data),foreground_capacity_fraction=statistics.median(d['foreground_capacity_fraction'] for d in data),missed_deadlines=sum(d['missed'] for d in data),raw_work_range=[min(d['work'] for d in data),max(d['work'] for d in data)]))
 rows.append(dict(case=c['case'],command=c['command'],name=c['name'],scenarios=scenarios,bulk_throughput_change_with_work_percent=100*(scenarios[2]['wire_Bps']/scenarios[0]['wire_Bps']-1)))
reference=next((r for r in rows if r['case']==registry['reference']),None)
for r in rows:
 for scenario in r['scenarios']:
  s=scenario['scenario'];scenario['wire_gain_over_stock_percent']=100*(scenario['wire_Bps']/reference['scenarios'][s]['wire_Bps']-1) if reference else None
  baseline=reference['scenarios'][s]['credited_work_per_second'] if reference else 0
  scenario['work_gain_over_stock_percent']=100*(scenario['credited_work_per_second']/baseline-1) if baseline else None
result=dict(kind=args.kind,report_version=2,work_steps=1,complete=not missing,missing_cases=missing,failed_attempts=failed,historical_PERF1_reports=historical,reference=registry['reference'],runs=runs,rows=rows,bulk_work_count_is_conservative_lower_bound=True)
(root/'perf2-comparison.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
