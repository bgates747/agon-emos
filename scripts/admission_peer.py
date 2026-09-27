#!/usr/bin/env python3
"""Bounded A04 functional EMOS UART-peer check; not a P4 or hardware test."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import socket
import struct
import subprocess
import tempfile
import time
import zlib


def seal(p):
    p[16:20] = struct.pack('<I', zlib.crc32(p[:16]+p[20:]))
    return bytes([0x8d,len(p)])+p


def run(a):
    transcript=bytearray();wire=[];sent=[];pending=[];phase='boot';offered=False;closed=False;app_keys=False;app_controls=None
    with tempfile.TemporaryDirectory(prefix='admission-peer-') as tmp:
        server=socket.socket(socket.AF_UNIX);server.bind(tmp+'/uart');server.listen(1)
        proc=subprocess.Popen([str(a.emulator.resolve()),'--mos',str(a.firmware.resolve()),
            '--sdcard',str(a.sdcard.resolve()),'--zero','--uart1-peer',tmp+'/uart'],
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        peer=None;buf=bytearray();start=time.monotonic();outcome='fail'
        def keys(text, delay=0):
            for i,ch in enumerate(text.encode()):
                when=time.monotonic()+delay+i*.06
                pending.extend([(when,bytes([0x81,4,ch,0,0,1])),(when+.025,bytes([0x81,4,ch,0,0,0]))])
        try:
            while time.monotonic()-start<20:
                now=time.monotonic()
                for ready in select.select([proc.stdout,peer or server],[],[],.003)[0]:
                    if ready is proc.stdout:
                        block=os.read(ready.fileno(),65536)
                        if not block:raise RuntimeError('emulator exited')
                        transcript.extend(block)
                    elif peer is None:peer,_=server.accept()
                    else:
                        block=peer.recv(4096)
                        if not block:raise RuntimeError('UART peer closed')
                        buf.extend(block)
                while len(buf)>=4:
                    if buf[:3] in (b'\x17\0\x80',b'\x17\0\x81'):
                        command=bytes(buf[:4]);del buf[:4]
                        if command[2]==0x80:peer.sendall(bytes([0x80,1,command[3]]))
                        continue
                    if buf[:3]!=b'\x17\0\xf6':raise RuntimeError('unexpected framing '+buf.hex())
                    n=buf[3]
                    if len(buf)<4+n:break
                    p=bytearray(buf[4:4+n]);del buf[:4+n]
                    assert n==48 and p[:4]==b'SD\x01\x04'
                    assert struct.unpack('<I',p[16:20])[0]==zlib.crc32(p[:16]+p[20:])
                    op=p[12];wire.append(op);p[3]=5
                    if op==1:
                        if a.case=='absent':
                            if phase=='boot':keys('echo OLD PEER CLI OK\r');phase='typing'
                            continue
                        p[20:28]=b'\x71\x19\x53\x07\0\0\0\0';p[46]=5
                    elif op==2:
                        if not offered:
                            p[32:36]=struct.pack('<I',123);p[44]=1;p[45]=3;offered=True
                        else:p[13]=1
                    elif op==3:
                        if a.case=='key-race':
                            # Key is ahead of the acknowledgement on the same UART.
                            peer.sendall(bytes([0x81,4,120,0,0,1]));phase='raced'
                    elif op==10:
                        closed=True
                        if a.case in ('missing','utility') and phase=='boot':keys('echo PROMPT RESTORED\r',.2);phase='typing'
                        continue
                    else:raise RuntimeError('unknown control '+str(op))
                    if a.case=='corrupt' and op==2:
                        packet=bytearray(seal(p));packet[18]^=1;peer.sendall(packet)
                        if phase=='boot':keys('echo BAD CRC CLI OK\r',.3);phase='typing'
                    else:peer.sendall(seal(p))
                pending.sort(key=lambda x:x[0])
                while pending and pending[0][0]<=now:
                    _,packet=pending.pop(0);peer.sendall(packet);sent.append(packet.hex())
                if phase=='raced' and transcript.rstrip().endswith(b'x'):
                    # Clear the partial line with Escape; stale offer must not run.
                    keys('\x1becho RACE CLI OK\r',.1);phase='typing'
                if a.case=='utility' and not app_keys and b'Application editor waiting' in transcript:
                    app_keys=True;app_controls=len(wire);keys('proof\r',.15)
                if app_keys and b'Finite admission fixture returned' not in transcript:
                    assert len(wire)==app_controls, 'Control emitted from public application editor'
                lines=[x.strip() for x in transcript.replace(b'\r',b'').splitlines()]
                expected={'absent':b'OLD PEER CLI OK','corrupt':b'BAD CRC CLI OK',
                          'key-race':b'RACE CLI OK','missing':b'PROMPT RESTORED','utility':b'PROMPT RESTORED'}[a.case]
                if expected in lines and transcript.rstrip().endswith(b'/ *') and not pending:
                    if a.case in ('missing','utility'):assert closed
                    elif a.case!='key-race':assert not closed
                    if a.case=='utility':assert b'Finite admission fixture returned' in lines
                    outcome='pass';break
            if outcome!='pass':raise RuntimeError('timeout: '+phase)
        finally:
            proc.terminate()
            try:tail,_=proc.communicate(timeout=3)
            except subprocess.TimeoutExpired:proc.kill();tail,_=proc.communicate()
            transcript.extend(tail);server.close()
            if peer:peer.close()
            a.output.parent.mkdir(parents=True,exist_ok=True)
            a.output.with_suffix('.log').write_bytes(transcript)
            a.output.write_text(json.dumps({'outcome':outcome,'case':a.case,'controls':wire,
                'key_packets':len(sent),'firmware_sha256':hashlib.sha256(a.firmware.read_bytes()).hexdigest(),
                'elapsed_seconds':round(time.monotonic()-start,3),
                'scope':'real eZ80 firmware with controlled UART peer; no P4/hardware claim'},indent=2)+'\n')
    print('PASS:',a.case)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('emulator','firmware','sdcard','output'):p.add_argument('--'+arg,type=Path,required=True)
    p.add_argument('--case',choices=['absent','corrupt','key-race','missing','utility'],required=True)
    run(p.parse_args())
