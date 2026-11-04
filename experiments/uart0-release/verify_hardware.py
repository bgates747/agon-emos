"""Check archived RC1 card reports against frozen app/ROM identities."""
from pathlib import Path
import argparse,hashlib,json,re
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('archive',type=Path);parser.add_argument('build',type=Path);args=parser.parse_args()
root=args.archive;card=root/'appliance';info=json.loads((args.build/'build-info.json').read_text());receipt=json.loads((root/'receipt.json').read_text())
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for name,i in receipt['files'].items():assert h(card/name)==i['sha256'],name
for name,pin in (('mos-rc1.bin',info['MOS_sha256']),('test.bin',info['test_sha256']),('example.bin',info['example_sha256']),('checks.bin',info['checks_sha256'])):assert h(card/name)==pin,name
assert receipt['immutable_deployed_files_unchanged'] and not receipt['card_modified']
reports=sorted(card.glob('R[0-9][0-9][0-9][0-9].log'));assert len(reports)==2
journal=(card/'runs.log').read_text();rows=[];expected=info['applications']['fixture']['build']
markers=('MODE async','Ordinary RST18 before opt-in PASS','Comparator negative/positive PASS','Zero-length/masked-context refusal PASS','Boundary lengths1/15/16/17/255/256/257/1024 and NULL callback PASS','Single source/BUSY/callback/input-hook/RX-during-TX PASS','VERIFY8192 PASS','Ordinary RST18 after finish PASS','TM001 MINIMAL PASS','RC1 PASS')
for path in reports:
 text=path.read_text();assert 'FAIL' not in text
 assert re.search(r'^TM001 build='+expected+r'$',text,re.M)
 pin=re.search(r'^RC1 MOS bytes=(\d+) CRC32=([0-9A-F]{8}) expected=([0-9A-F]{8})$',text,re.M);assert pin and int(pin[1])==info['MOS_bytes'] and pin[2]==pin[3]==info['MOS_crc32']
 for marker in markers:assert text.splitlines().count(marker)==1,(path,marker)
 for status in ('START','PASS'):assert journal.splitlines().count(path.stem+' '+status+' RC1 build='+expected)==1,(path,status)
 rows.append(dict(file=path.name,report_number=int(path.stem[1:]),build=expected,sha256=h(path),passed=True,receiver_bytes=8192))
assert rows[1]['report_number']==rows[0]['report_number']+1
assert int((card/'next.txt').read_text())==rows[-1]['report_number']+1
result=dict(passed=True,physical=True,board='Heber Agon Console8',source_commit=info['source_commit'],MOS_sha256=info['MOS_sha256'],installed_ROM_prefix_bytes=info['MOS_bytes'],installed_ROM_prefix_crc32=info['MOS_crc32'],reports=rows,receiver_bytes=16384,failed_or_missing_reports=[],next_report=rows[-1]['report_number']+1,operator_report='Tests done, all appeared to pass; final run followed the agreed stock-VDP protocol',VDP=dict(name='Stock Bistromathics 2.16.0',basis='agreed final-run protocol and Author completion report',independently_fingerprinted=False,staged_stock_sha256=receipt['files']['vdp-stock.bin']['sha256']),cold_boot_banner_and_example=dict(basis='Author overall test-completion report',independently_logged=False),emulator_tests=dict(run=False,waived_by_Author=True),keyboard_hook_installed=True,keyboard_events_injected=False,performance_remeasured=False,receive_recovery_race_retested=False,card_modified=False,archive_sha256=receipt['archive_sha256'])
(root/'analysis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
