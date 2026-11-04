"""Validate this task's returned matched Console8 run without touching the card."""
from pathlib import Path
import hashlib,importlib.util,json,re,sys
root=Path(__file__).resolve().parents[2];archive=Path(sys.argv[1]);build=Path(sys.argv[2]);rcbuild=Path(sys.argv[3]);card=archive/'appliance'
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt=json.loads((archive/'receipt.json').read_text());assert receipt['archive_verified'] and receipt['immutable_deployed_files_unchanged'] and not receipt['card_modified']
assert h(archive/'returned.tar.gz')==receipt['archive_sha256']
for name,d in receipt['files'].items():assert h(card/name)==d['sha256'],name
info=json.loads((build/'build-info.json').read_text());rcinfo=json.loads((rcbuild/'build-info.json').read_text());registry=json.loads((card/'cases.json').read_text())
spec=importlib.util.spec_from_file_location('metrics',root/'experiments/uart0-performance2/metrics.py');metrics=importlib.util.module_from_spec(spec);spec.loader.exec_module(metrics)
summary=json.loads((card/'perf2-comparison.json').read_text());assert summary['kind']=='hardware' and summary['complete'] and not summary['failed_attempts']
reports=sorted(card.glob('R[0-9][0-9][0-9][0-9].log'));assert [p.stem for p in reports]==[f'R{i:04}' for i in range(54,60)]
assert int((card/'next.txt').read_text())==60
journal=(card/'runs.log').read_text();validated=[];rows={}
common=('MODE async','Ordinary RST18 before opt-in PASS','Comparator negative/positive PASS','Boundary lengths1/15/16/17/255/256/257/1024 and NULL callback PASS','Single source/BUSY/callback/input-hook/RX-during-TX PASS','VERIFY8192 PASS','Ordinary RST18 after finish PASS','TM001 MINIMAL PASS')
for p in reports:
 text=p.read_text();assert 'FAIL' not in text
 if 'PERF2 build=' in text:
  d=metrics.read_report(text);c=next(c for c in registry['cases'] if c['case']==d['case']);assert d['build']==c['build']
  source=json.loads((card/c['source']).read_text());assert d['mos_bytes']==source['mos_bytes'] and d['mos_crc32']==source['mos_crc'] and d['config']['transport']==source['transport']
  assert h(card/c['app'])==c['app_sha256'] and h(card/c['firmware'])==c['MOS_sha256']
  for status in ('START','PASS'):assert journal.splitlines().count(p.stem+' '+status+' PERF2 build='+d['build']+' case='+d['case'])==1
  assert all(t['missed']==0 for t in d['trials'])
  assert d['case'] not in rows;rows[d['case']]=d
  validated.append(dict(file=p.name,kind='PERF2',case=d['case'],build=d['build'],sha256=h(p),passed=True,receiver_bytes=9*8192))
 else:
  trial=p.stem in ('R0057','R0058');label='TRIM1' if trial else 'RC1'
  b=info['functional']['build'] if trial else rcinfo['applications']['fixture']['build']
  n=info['functional']['MOS_bytes'] if trial else rcinfo['MOS_bytes'];crc=info['functional']['MOS_crc32'] if trial else rcinfo['MOS_crc32']
  assert text.splitlines().count('TM001 build='+b)==1
  for marker in (*common,label+' PASS',('Zero-length refusal; context is a caller precondition PASS' if trial else 'Zero-length/masked-context refusal PASS')):assert text.splitlines().count(marker)==1,(p,marker)
  assert text.splitlines().count(f'{label} MOS bytes={n} CRC32={crc} expected={crc}')==1
  for status in ('START','PASS'):assert journal.splitlines().count(p.stem+' '+status+' '+label+' build='+b)==1
  validated.append(dict(file=p.name,kind=label,build=b,sha256=h(p),passed=True,receiver_bytes=8192))
assert set(rows)=={c['case'] for c in registry['cases']}
by={r['case']:r for r in summary['rows']};rc=by['published-rc1']['scenarios'];trial=by['trim-trial1']['scenarios']
result=dict(passed=True,physical=True,board='Heber Agon Console8',firmware_source=info['source_commit'],source_build='tr001-002',reports=validated,receiver_bytes=sum(x['receiver_bytes'] for x in validated),performance_trials=27,missing_or_failed_reports=[],missed_deadlines=0,next_report=60,summary=summary,parsed_trials=rows,trim_vs_rc1=dict(paced_work_gain_percent=100*(trial[1]['credited_work_per_second']/rc[1]['credited_work_per_second']-1),bulk_work_gain_percent=100*(trial[2]['credited_work_per_second']/rc[2]['credited_work_per_second']-1),bulk_throughput_gain_percent=100*(trial[0]['payload_Bps']/rc[0]['payload_Bps']-1)),VDP=dict(name='Stock Bistromathics 2.16.0',basis='Agreed all-case protocol and Author completion report',independently_fingerprinted=False),example_and_cold_boot=dict(basis='Author overall completion report; the example creates no log',independently_logged=False),archive_sha256=receipt['archive_sha256'],card_modified=False,limitations=['Three short repeats per scenario; tiny differences do not establish a sustained speedup.','Work counters measure foreground calculation overlap, not exclusive CPU utilization.','Different MOSes ran sequentially; VDP identity and the example/cold-boot result were not independently logged.'],release_promoted=False)
(archive/'analysis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed=True,receiver_bytes=result['receiver_bytes'],reports=len(validated),trim_vs_rc1=result['trim_vs_rc1'])))
