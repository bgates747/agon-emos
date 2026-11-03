"""Finite Oryx leased functional campaign. First failure stops the campaign."""
from pathlib import Path
import json,os,shlex,shutil,signal,subprocess,sys,time
root=Path(__file__).resolve().parents[2];out=root/'build/tm001'/sys.argv[1]
profiles=json.loads((out/'profiles.json').read_text());results=[];performance=profiles.get('suite')=='performance'
def lease():
 token=os.environ['UART0_ORYX_LEASE'];env=root.parent/'agon-dev-env'
 args=[str(env/'.venv/bin/python'),str(env/'cross-agent/vdplet-rollout/oryx_lease.py'),'assert-owner','--owner','agon-mos','--lease',token]
 subprocess.run(['ssh','-o','BatchMode=yes','raspberrypi',shlex.join(args)],check=True,stdout=subprocess.DEVNULL)
for row in profiles['profiles']:
 p=Path(row['path']);r=p/'results';r.mkdir();art=p/'last-run';proc=None;error=None;start=time.monotonic()
 try:
  for pin in ('.runtime.sha256','.mos-release.sha256'):subprocess.run(['sha256sum','--check','--status',pin],cwd=p,check=True)
  with (r/'frontend.log').open('w') as log:
   lease() # immediately before every actual emulator process
   proc=subprocess.Popen(['./flexe-agon-emulator','--precise-interrupts'],cwd=p,stdout=log,stderr=subprocess.STDOUT,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',AGON_FLEXE_ENGINE=row['engine']),start_new_session=True)
   n=0;last=start
   while time.monotonic()-start<300:
    f=art/'mos-to-vdp.bin';wire=f.read_bytes() if f.exists() else b''
    if len(wire)!=n:n=len(wire);last=time.monotonic()
    if b'TM001 CLI RETURN' in wire:break
    assert b'TM001 MINIMAL FAIL' not in wire and b'PERF1 FAIL' not in wire,'Fixture reports FAIL'
    assert b'Error executing autoexec' not in wire,'MOS command failed'
    assert proc.poll() is None,'Frontend exited early'
    assert time.monotonic()-last<45,'No UART progress for45seconds'
    time.sleep(.1)
   else:raise TimeoutError('300second cell limit')
 except Exception as exc:error=repr(exc)
 finally:
  if proc and proc.poll() is None:
   expected=(p/'flexe-agon-emulator.bin').resolve()
   for child in Path('/proc').glob('[0-9]*'):
    try:
     if os.getpgid(int(child.name))==proc.pid and (child/'exe').resolve()==expected:os.kill(int(child.name),signal.SIGTERM)
    except (OSError,PermissionError):pass
   try:proc.wait(timeout=10)
   except subprocess.TimeoutExpired:os.killpg(proc.pid,signal.SIGKILL);proc.wait()
  if art.exists():shutil.copytree(art,r/'raw')
  reports=r/'reports';reports.mkdir();subprocess.run(['mcopy','-s','-i',str(p/'card.img'),'::/uart0/*',str(reports)],check=True,stdout=subprocess.DEVNULL)
 try:
  assert error is None,error
  report=(reports/'R0001.log').read_text();assert ('PERF1 PASS' if performance else 'TM001 MINIMAL PASS') in report and 'VERIFY8192 PASS' in report
  assert int((reports/'next.txt').read_text())==2
  if performance:
   assert report.count('VERIFY8192 PASS')==6 and report.count('DATA rep=')==6
   assert 'START' in (reports/'runs.log').read_text() and 'PASS' in (reports/'runs.log').read_text()
   assert 'MODE '+str(('baseline','scalar003','otirx003').index(row['variant']))+';' in report
  else:
   assert ('MODE standard' if row['variant']=='baseline' else 'MODE async') in report
  if not performance and row['variant']!='baseline':assert 'RX-during-TX PASS' in report and 'NULL callback PASS' in report and 'masked-context refusal PASS' in report
  stats=json.loads((r/'raw/stats.json').read_text());uart=json.loads((r/'raw/uart.json').read_text())
  assert not any(stats[k] for k in ('failed','unhandled','unregistered','unmapped'))
  assert uart['tx_drops']==uart['rx_overruns']==0
 except Exception as exc:
  if error is None:error=repr(exc)
 result=dict(**row,passed=error is None,error=error,elapsed=time.monotonic()-start,physical=False,keyboard_input_injected=False,receiver_bytes=8192*(6 if performance else 1) if error is None else 0)
 (r/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');results.append(result);(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(result),flush=True)
 if error:raise SystemExit('Campaign stopped at first failed cell')
