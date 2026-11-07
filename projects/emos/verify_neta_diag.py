#!/usr/bin/env python3
"""Exact optional observer splice checks, not executed IRQ/CPU qualification."""
import argparse
from pathlib import Path
from verify_abi import symbols
p=argparse.ArgumentParser()
for n in ('source','elf','nm','objdump'):p.add_argument('--'+n,type=Path,required=True)
a=p.parse_args();s=symbols(a.nm,a.elf);image=a.elf.with_suffix('.bin').read_bytes()
if '_emos_neta_diag_emit' not in s:
 assert '_emos_neta_diag_hook' not in s
 print('Optional RX observer absent: no resident marker pointer/bridge')
else:
 def addr(n):return s[n].to_bytes(3,'little')
 assert s['_emos_neta_diag_hook']>=0xb0000
 start=s['_emos_neta_diag_emit']
 raw=image[start:start+80]
 # The bridge supplies immediate context, never an acknowledging LSR read.
 assert raw.startswith(bytes.fromhex('f5 3a')+(s['_emos_neta_diag_hook']+2).to_bytes(3,'little')+bytes.fromhex('b7 28'))
 for n in ('_emos_neta_rx_hold','_emos_key_rx'):assert addr(n) in raw
 assert bytes.fromhex('ed109e') in raw, 'D=owner PC_DR sample missing'
 assert bytes.fromhex('ed38d5') not in raw and bytes.fromhex('ed48d5') not in raw
 assert bytes.fromhex('dde9') in raw, 'Register-native call target missing'
 for ev in (1,2,3,4,5,0x81,0x82,0x83,0x84,0x85,0x10,0x90,0x11,0x91,0x21,0x22,0x40):
  code=bytes.fromhex('f53e')+bytes([ev])+b'\xcd'+addr('_emos_neta_diag_emit')+b'\xf1'
  assert code in image, ('missing exact marker',ev)
 assert len(image)<=131072
 print('Optional owner bridge/context/paired source splices fit; app register/IFF execution unqualified')
