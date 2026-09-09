#!/usr/bin/env python3
"""Replay native USB mapped/retained-serialized keys into the ordinary EMOS CLI.

The UART1 peer is an emulator boundary, not USB hardware or P4 runtime proof.
Graphical completion is explicitly pending human screen review.
"""
import argparse
import json
import os
from pathlib import Path
import select
import socket
import subprocess
import tempfile
import time


def run(args):
    media=args.sdcard.resolve(strict=True)
    packets=(media/'usb-cli-packets.bin').read_bytes()
    if not packets or len(packets)%6 or any(packets[i:i+2]!=b'\x81\x04' for i in range(0,len(packets),6)):
        raise ValueError('Expected serializer-produced stock keyboard packets')
    record={'outcome':'fail','locales':[],'polls':[],'keys_sent':0,
            'scope':'native USB mapping and retained host serializer into real EMOS; no physical claim',
            'withhold_admission':args.withhold_admission}
    transcript=bytearray()
    def save():
        args.output.write_text(json.dumps(record,indent=2)+'\n')
        args.output.with_suffix('.log').write_bytes(transcript)
    with tempfile.TemporaryDirectory(prefix='emos-usb-cli-') as temporary:
        server=socket.socket(socket.AF_UNIX)
        path=temporary+'/uart1';server.bind(path);server.listen(1)
        command=[str(args.emulator.resolve()),'--mos',str(args.firmware.resolve()),
                 '--sdcard',str(media),'--zero','--uart1-peer',path]
        if args.graphical: command+=['--vdp',str(args.vdp.resolve()),'--renderer','sw','--verbose']
        env=os.environ.copy()
        env['LD_LIBRARY_PATH']=str(Path.home()/'.local/lib')+os.pathsep+env.get('LD_LIBRARY_PATH','')
        process=subprocess.Popen(command,cwd=temporary,env=env,stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        peer=None;received=bytearray();scheduled=[]
        started=time.monotonic();finished_at=None;prompt_sent=False
        try:
            while True:
                now=time.monotonic()
                if finished_at is None and now-started>45: raise RuntimeError('Ordinary CLI review deadline')
                for ready in select.select([process.stdout,peer or server],[],[],0.005)[0]:
                    if ready is process.stdout: transcript.extend(os.read(ready.fileno(),65536))
                    elif peer is None: peer,_=server.accept();peer.setblocking(False)
                    else:
                        block=peer.recv(4096)
                        if not block and finished_at is None: raise RuntimeError('Peer closed before completion')
                        received.extend(block)
                while len(received)>=4:
                    request=bytes(received[:4]);del received[:4]
                    if request[:3]==bytes([23,0,0x81]): record['locales'].append(request[3])
                    elif request[:3]==bytes([23,0,0x80]):
                        if record['polls']: raise RuntimeError('Unexpected repeated admission')
                        record['polls'].append(request[3])
                        if not args.withhold_admission:
                            scheduled.append((now+0.05,bytes([0x80,1,request[3]]),False))
                            for i in range(len(packets)//6):
                                scheduled.append((now+0.75+i*0.04,packets[i*6:i*6+6],True))
                    else: raise RuntimeError('Unexpected EMOS request: '+request.hex(' '))
                while scheduled and scheduled[0][0]<=now:
                    _,packet,is_key=scheduled.pop(0);peer.sendall(packet)
                    if is_key: record['keys_sent']+=1
                lines=[line.strip() for line in transcript.replace(b'\r',b'').splitlines()]
                at_prompt=transcript.rstrip().endswith(b'/ *')
                if args.graphical:
                    if finished_at is None and record['keys_sent']==len(packets)//6 and not scheduled:
                        finished_at=now;record['outcome']='awaiting-human-review';save()
                        print('Review the ordinary CLI edits, source query and mainboard return in the emulator.',flush=True)
                else:
                    expected=([b'KEYINPUT FAIL: receiver readiness timeout'] if args.withhold_admission else
                              [b'USB cli',b'Shift 1!',b'USB CLI REVIEW COMPLETE',b'Keyboard input: mainboard'])
                    complete=all(line in lines for line in expected) and at_prompt
                    if complete and finished_at is None:
                        if record['locales']!=[1] or len(record['polls'])!=1:
                            raise RuntimeError('Unexpected admission exchange')
                        if not args.withhold_admission and (record['keys_sent']!=len(packets)//6 or
                                lines.count(b'Keyboard input: extender')!=2):
                            # The final Enter release can follow the prompt by one frame.
                            continue
                        if args.withhold_admission and b'Keyboard input: extender' in lines:
                            raise RuntimeError('Unacknowledged extender admission')
                        finished_at=now
                    if finished_at is not None:
                        if at_prompt and not prompt_sent:
                            process.stdin.write(b'echo MAINBOARD PROMPT RETURNED\n');process.stdin.flush();prompt_sent=True
                        if b'MAINBOARD PROMPT RETURNED' in lines and at_prompt:
                            record['outcome']='pass';record['mainboard_prompt_return']=True;save()
                            print('PASS: '+('absent peer retains mainboard' if args.withhold_admission else
                                  'ordinary CLI text, Backspace/arrows, modifiers, source query and return'))
                            break
                        if now-finished_at>5: raise RuntimeError('Mainboard prompt return failed')
                if process.poll() is not None:
                    if finished_at is None or process.returncode: raise RuntimeError('Emulator exited before completion')
                    break
        except Exception as error:
            record['outcome']='fail';record['error']=str(error);raise
        finally:
            process.terminate()
            try: remainder,_=process.communicate(timeout=5)
            except subprocess.TimeoutExpired: process.kill();remainder,_=process.communicate()
            transcript.extend(remainder)
            if peer: peer.close()
            server.close();save()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('emulator','firmware','sdcard','output'): parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--vdp',type=Path);parser.add_argument('--graphical',action='store_true')
    parser.add_argument('--withhold-admission',action='store_true')
    args=parser.parse_args()
    if args.graphical and (not args.vdp or args.withhold_admission): parser.error('Graphical review requires VDP and a responding peer')
    run(args)
