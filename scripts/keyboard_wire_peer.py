#!/usr/bin/env python3
"""Replay the verified retained P4 serializer output into emulated UART1.

This tests real resident EMOS effects, not P4 hardware or a complete P4 runtime.
key-packets.bin is emitted by Extender's maintained sender host test; the
generated profile binds its hash. All twelve keys follow one readiness poll.
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
    packets=(media/'key-packets.bin').read_bytes()
    if len(packets)!=72 or any(packets[i:i+2]!=b'\x81\x04' for i in range(0,72,6)):
        raise ValueError('Expected twelve serializer-produced stock key packets')
    marker=media/'keyboard-state.bin'
    if marker.is_symlink(): raise ValueError('Symlinked result file')
    marker.unlink(missing_ok=True)
    record={'outcome':'fail','locales':[],'polls':[],'keys_sent':0,
            'expected':'input timeout' if args.withhold_keys else 'twelve keys',
            'scope':'retained sender host bytes and real EMOS emulator; no physical claim'}
    transcript=bytearray()
    def save():
        args.output.write_text(json.dumps(record,indent=2)+'\n')
        args.output.with_suffix('.log').write_bytes(transcript)
    with tempfile.TemporaryDirectory(prefix='emos-wire-peer-') as temporary:
        server=socket.socket(socket.AF_UNIX)
        path=temporary+'/uart1'; server.bind(path); server.listen(1)
        command=[str(args.emulator.resolve()),'--mos',str(args.firmware.resolve()),
                 '--sdcard',str(media),'--zero','--uart1-peer',path]
        if args.graphical: command+=['--vdp',str(args.vdp.resolve()),'--renderer','sw','--verbose']
        env=os.environ.copy()
        env['LD_LIBRARY_PATH']=str(Path.home()/'.local/lib')+os.pathsep+env.get('LD_LIBRARY_PATH','')
        process=subprocess.Popen(command,cwd=temporary,env=env,stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        peer=None; received=bytearray(); scheduled=[]
        started=time.monotonic(); result_at=None; prompt_sent=False
        try:
            while True:
                now=time.monotonic()
                if result_at is None and now-started>30: raise RuntimeError('Fixture deadline')
                for ready in select.select([process.stdout,peer or server],[],[],0.005)[0]:
                    if ready is process.stdout: transcript.extend(os.read(ready.fileno(),65536))
                    elif peer is None: peer,_=server.accept(); peer.setblocking(False)
                    else:
                        block=peer.recv(4096)
                        if not block and result_at is None: raise RuntimeError('Peer closed before result')
                        received.extend(block)
                while len(received)>=4:
                    request=bytes(received[:4]); del received[:4]
                    if request[:3]==bytes([23,0,0x81]): record['locales'].append(request[3])
                    elif request[:3]==bytes([23,0,0x80]):
                        if record['polls']: raise RuntimeError('Unexpected second admission')
                        record['polls'].append(request[3])
                        scheduled.append((now+0.05,bytes([0x80,1,request[3]]),False))
                        if not args.withhold_keys:
                            for i in range(12): scheduled.append((now+0.55+i*0.25,packets[i*6:i*6+6],True))
                    else: raise RuntimeError('Unexpected EMOS request: '+request.hex(' '))
                while scheduled and scheduled[0][0]<=now:
                    _,packet,is_key=scheduled.pop(0); peer.sendall(packet)
                    if is_key: record['keys_sent']+=1
                result=marker.read_bytes() if marker.exists() else b''
                if result and result_at is None:
                    expected_result=b'\xff' if args.withhold_keys else b'\x01'
                    expected_count=0 if args.withhold_keys else 12
                    if result!=expected_result or record['keys_sent']!=expected_count or record['locales']!=[1] or len(record['polls'])!=1:
                        raise RuntimeError('Guest rejected packets or incomplete sender exchange')
                    record['outcome']='pass'; result_at=now; save()
                    if args.graphical: print('PASS: twelve keyboard packets and EMOS API effects',flush=True)
                if result_at is not None and not args.graphical:
                    at_prompt=transcript.rstrip().endswith(b'/ *')
                    if at_prompt and not prompt_sent and now-result_at>=0.5:
                        process.stdin.write(b'echo KEYBOARD WIRE PROMPT RETURNED\n'); process.stdin.flush(); prompt_sent=True
                    if b'KEYBOARD WIRE PROMPT RETURNED' in transcript.splitlines() and at_prompt:
                        if b'Keyboard input: mainboard' not in transcript:
                            raise RuntimeError('No mainboard cleanup report')
                        record['prompt_return']=True
                        print('PASS: '+record['expected']+'; EMOS effects and prompt return')
                        break
                    if now-result_at>5: raise RuntimeError('No mainboard prompt return')
                if process.poll() is not None:
                    if result_at is None or process.returncode: raise RuntimeError('Emulator exited before clean result')
                    break
        except Exception as error:
            record['outcome']='fail'; record['error']=str(error); raise
        finally:
            process.terminate()
            try: remainder,_=process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill(); remainder,_=process.communicate()
            transcript.extend(remainder)
            if peer: peer.close()
            server.close(); save()


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('emulator','firmware','sdcard','output'): parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--vdp',type=Path); parser.add_argument('--graphical',action='store_true')
    parser.add_argument('--withhold-keys',action='store_true',help='CLI check of the SD input deadline and cleanup')
    args=parser.parse_args()
    if args.graphical and not args.vdp: parser.error('--graphical requires --vdp')
    if args.graphical and args.withhold_keys: parser.error('Input-timeout check is CLI-only')
    run(args)
