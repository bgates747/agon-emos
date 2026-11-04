#!/usr/bin/env python3
"""EMOS-001: verify RAM IRQ defaults survive native Eirene BSS clearing."""
import argparse,subprocess,re
from pathlib import Path

def verify(image,s):
 table=s['__2nd_jump_table'];start=s['bss_start'];end=s['bss_end']
 assert start<=table and table+48*4<=end,'IRQ table unexpectedly outside BSS; review startup proof'
 entry=s['__init'];boundary=s['_init_default_vectors'];code=image[entry:boundary]
 addr=lambda n:n.to_bytes(3,'little')
 clear=b'\x21'+addr(start)+b'\x11'+addr(start+1)+b'\x01'+addr(end-start-1)+b'\x36\x00\xcd'+addr(s['ldir_handle_zerolen'])
 init=b'\xcd'+addr(boundary)
 assert code.count(clear)==1,'Expected one exact final BSS clearing sequence'
 assert code.count(init)==1,'Default IRQ initialization must have exactly one startup caller'
 assert code.index(init)==code.index(clear)+len(clear),'IRQ defaults are not initialized immediately after BSS clear'

def main():
 p=argparse.ArgumentParser()
 for n in ('source','elf','nm','objdump'):p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();out=subprocess.check_output([str(a.nm),'-n',str(a.elf)],text=True);s={}
 for l in out.splitlines():
  f=l.split()
  if len(f)>=3 and re.fullmatch('[0-9a-fA-F]+',f[0]):s[f[-1]]=int(f[0],16)
 verify(a.elf.with_suffix('.bin').read_bytes(),s)
 print('EMOS native startup: all48 RAM IRQ defaults initialized after final BSS clearing')
if __name__=='__main__':main()
