#!/usr/bin/env python3
"""A05 real eZ80 application with controlled UART/storage peer; no P4 claim.

Use the retained CLI UART harness, not a graphical attention profile. Directory
backed emulator f_sync is unimplemented; send-only tests avoid destination FAT
mutation. Do not treat this peer's in-memory staging as physical durability.
"""
import argparse,hashlib,json,os,select,socket,struct,subprocess,tempfile,time,zlib
from pathlib import Path

def seal(p):
 p[16:20]=struct.pack('<I',zlib.crc32(p[:16]+p[20:]));return bytes([0x8d,len(p)])+p

def run(a):
 data=bytes((i*17+3)&255 for i in range(1027));saved=None;stage=None;declared=None
 transcript=bytearray();buf=bytearray();controls=[];files=[];descriptor=bytearray();nonce=0;last_job=0;prefix=None
 start=time.monotonic();outcome='fail'
 with tempfile.TemporaryDirectory(prefix='app-peer-') as temp:
  listener=socket.socket(socket.AF_UNIX);listener.bind(temp+'/uart');listener.listen(1);peer=None
  proc=subprocess.Popen([str(a.emulator.resolve()),'--mos',str(a.firmware.resolve()),'--sdcard',str(a.sdcard.resolve()),'--zero','--uart1-peer',temp+'/uart'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,stdin=subprocess.PIPE)
  try:
   while time.monotonic()-start<40:
    for fd in select.select([proc.stdout,peer or listener],[],[],.003)[0]:
     if fd is proc.stdout:
      b=os.read(fd.fileno(),65536)
      if not b:raise RuntimeError('emulator exited')
      transcript.extend(b)
     elif peer is None:peer,_=listener.accept()
     else:
      b=peer.recv(4096)
      if not b:raise RuntimeError('peer closed')
      buf.extend(b)
    while len(buf)>=4:
     if buf[:3] in (b'\x17\0\x80',b'\x17\0\x81'):
      p=bytes(buf[:4]);del buf[:4]
      if p[2]==0x80:peer.sendall(bytes([0x80,1,p[3]]))
      continue
     assert buf[:3]==b'\x17\0\xf6',buf.hex()
     n=buf[3]
     if len(buf)<4+n:break
     p=bytearray(buf[4:4+n]);del buf[:4+n]
     assert p[:3]==b'SD\x01' and struct.unpack_from('<H',p,14)[0]==n-20
     assert struct.unpack_from('<I',p,16)[0]==zlib.crc32(p[:16]+p[20:])
     op=p[12];payload=b'';status=0
     if p[3]==4:
      controls.append(op);reply=bytearray(p[:48]);reply[3]=5
      if op==1:
       nonce+=1;reply[20:48]=bytes(28);reply[20:28]=struct.pack('<Q',nonce);reply[46]=13;descriptor=bytearray();prefix=None
      elif op==2:reply[13]=1
      elif op==5:
       assert p[44]==2 and p[45] in (3,4)
       total,off,crc=struct.unpack_from('<HHI',p,48);assert off==len(descriptor)
       if not off:last_job+=1
       descriptor.extend(p[56:]);assert len(descriptor)<=total<=248
       if len(descriptor)==total:assert zlib.crc32(descriptor)==crc
       reply[32:36]=struct.pack('<I',last_job)
      elif op==4:
       prefix=bytes(p[20:48]);assert prefix[24]==2
      elif op==8:reply[13]=9
      else:assert op in (9,10)
      reply[14:16]=struct.pack('<H',28);peer.sendall(seal(reply));continue
     assert p[3]==1 and prefix is not None
     assert struct.unpack_from('<I',p,4)[0]==(zlib.crc32(prefix) or 1)
     files.append(op);q=p[20:]
     if op==1:payload=struct.pack('<IHH',99,212,15)
     elif op==2:status=6;payload=b'\x04'
     elif op==4:
      off,count=struct.unpack_from('<IH',q);assert bytes(q[7:])==b'/p4/source.bin'
      payload=struct.pack('<I',len(data))+data[off:off+count]
     elif op==5:
      tid,size,crc=struct.unpack_from('<III',q);assert size<=1027
      declared=(tid,size,crc);stage=bytearray();payload=struct.pack('<II',tid,0)
     elif op==6:
      tid,off=struct.unpack_from('<II',q);assert tid==declared[0] and off==len(stage)
      stage.extend(q[8:]);payload=struct.pack('<II',tid,len(stage))
     elif op==7:
      assert len(stage)==declared[1] and zlib.crc32(stage)==declared[2]
      payload=struct.pack('<II',declared[1],declared[2])
     elif op==8:saved=bytes(stage);assert saved==data
     else:raise RuntimeError('unexpected file op '+str(op))
     reply=bytearray(p[:20])+payload;reply[3]=2;reply[13]=status;reply[14:16]=struct.pack('<H',len(payload));peer.sendall(seal(reply))
    if b'Application send and caller memory PASS;' in transcript and transcript.rstrip().endswith(b'/ *'):
     assert saved==data;assert (a.sdcard/'agents/extender/results/a05-pass.bin').read_bytes()==b'A05 application send and caller memory PASS\n'
     outcome='pass';break
   if outcome!='pass':raise RuntimeError('No application PASS: '+transcript.decode(errors='replace')[-1500:])
  finally:
   proc.terminate()
   try:tail,_=proc.communicate(timeout=3)
   except subprocess.TimeoutExpired:proc.kill();tail,_=proc.communicate()
   transcript.extend(tail);listener.close()
   if peer:peer.close()
   a.output.parent.mkdir(parents=True,exist_ok=True);a.output.with_suffix('.log').write_bytes(transcript)
   a.output.write_text(json.dumps({'outcome':outcome,'controls':controls,'file_operations':files,'firmware_sha256':hashlib.sha256(a.firmware.read_bytes()).hexdigest(),'elapsed_seconds':round(time.monotonic()-start,3),'transferred_bytes':len(saved) if saved is not None else 0,'scope':'real eZ80 caller with in-memory UART peer; no physical P4 or FAT durability claim'},indent=2)+'\n')
 print('PASS: application transfer and caller return')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 for n in ('emulator','firmware','sdcard','output'):p.add_argument('--'+n,type=Path,required=True)
 run(p.parse_args())
