"""Compare complete returned report sets, without altering any source logs."""
from pathlib import Path
import argparse,json,re,statistics
from metrics import read_report
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory',type=Path);parser.add_argument('--kind',choices=('hardware','emulator'),default='emulator')
args=parser.parse_args();manifest=json.loads((args.directory/'source.json').read_text());groups={n:[] for n in range(3)};runs=[]
for path in sorted(args.directory.glob('R[0-9][0-9][0-9][0-9].log')):
 report=path.read_text()
 if 'PERF1 build='+manifest['build'] not in report:continue
 d=read_report(report);pin=manifest['mos'][d['mode']]
 assert d['mos_bytes']==pin['size'] and d['mos_crc32']==pin['crc32'],path
 groups[d['mode']].extend(d['trials']);runs.append(dict(file=path.name,mode=d['mode']))
assert all(groups.values()),'Need complete standard, scalar and OTIRX reports from this build'
rows=[]
for mode,trials in groups.items():
 bulk=[d for d in trials if not d['paced']];paced=[d for d in trials if d['paced']]
 rows.append(dict(mode=mode,name=('Pristine Eirene','Tom-derived scalar','OTIRX')[mode],
  bulk_payload_Bps=statistics.median(d['payload_Bps'] for d in bulk),
  bulk_wire_Bps=statistics.median(d['wire_Bps'] for d in bulk),
  paced_work_per_second=statistics.median(d['work']*1152000/d['ticks'] for d in paced),
  foreground_capacity_fraction=statistics.median(d['foreground_capacity_fraction'] for d in paced),
  missed_deadlines=sum(d['missed'] for d in paced),max_lateness_ticks=max(d['late'] for d in paced),samples=len(trials)))
for r in rows:
 r['bulk_gain_percent']=100*(r['bulk_wire_Bps']/rows[0]['bulk_wire_Bps']-1)
 r['work_gain_percent']=100*(r['paced_work_per_second']/rows[0]['paced_work_per_second']-1)
print('Result source: '+args.kind+'. '+('Physical measurements.' if args.kind=='hardware' else 'Model diagnostics ONLY; not hardware performance.'))
print('| Sender | Bulk payload B/s | Wire B/s | Bulk gain | Paced work/s | Work gain | Missed deadlines |')
print('|---|---:|---:|---:|---:|---:|---:|')
for r in rows:print('| {name} | {bulk_payload_Bps:.0f} | {bulk_wire_Bps:.0f} | {bulk_gain_percent:+.1f}% | {paced_work_per_second:.1f} | {work_gain_percent:+.1f}% | {missed_deadlines} |'.format(**r))
print('Work gain means more completed foreground calculations, not directly measured CPU usage. Medians use complete matched report sets; inspect deadlines and raw samples as well.')
(args.directory/'comparison.json').write_text(json.dumps(dict(kind=args.kind,build=manifest['build'],runs=runs,rows=rows),indent=2)+'\n')
