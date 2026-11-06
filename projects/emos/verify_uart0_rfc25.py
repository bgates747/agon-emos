#!/usr/bin/env python3
"""INTEG-021 exact linked RFC25 admission/ISR and historical-profile boundary."""
import argparse
from pathlib import Path
from verify_abi import symbols
from verify_parallel import disassemble_global_extent,instructions

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('source','elf','nm','objdump'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();s=symbols(a.nm,a.elf);image=a.elf.with_suffix('.bin').read_bytes()
 def addr(name):return s[name].to_bytes(3,'little')
 def exact(name,data):
  at=s[name];assert image[at:at+len(data)]==data,name
 table=s['mos_api_block1_start'];trampoline=s['mos_api_uart0_rfc25'];assert trampoline<65536
 assert image[table+0x25*2:table+0x25*2+2]==trampoline.to_bytes(2,'little')
 for slot in (0x42,0x43):assert image[table+slot*2:table+slot*2+2]==s['mos_api_not_implemented'].to_bytes(2,'little')
 assert 'uart0_async_write' not in s and 'uart0_async_isr' not in s
 exact('mos_api_uart0_rfc25',b'\xc3'+addr('emos_uart0_rfc25_submit'))
 # Classify TX before the inherited RX BC/DE/HL and index/alternate saves.
 prefix=b'\xf3\xf5\xed\x38\xc2\xe6\x0e\xfe\x02\xca'+addr('fill_uart0_tx_fifo')
 exact('_uart0_handler',prefix+b'\xc5\xd5\xe5\xdd\xe5\xfd\xe5\x08\xf5\xd9\xc5\xd5\xe5')
 # Final fill source release disables THRE then executes the caller thunk.
 for label in ('fill_uart0_tx_fifo','.last_fill'):
  rows=[(op,arg) for _,op,arg in instructions(disassemble_global_extent(a.objdump,a.elf,label))]
  assert ('otirx','') in rows,label
 raw=disassemble_global_extent(a.objdump,a.elf,'fill_uart0_tx_fifo')
 assert raw.count('reti.lil')==2 and raw.count('otirx')==2
 # The ownership guard is a bounded foreground read, no wait, callback or EI.
 owned=[(op,arg) for _,op,arg in instructions(disassemble_global_extent(a.objdump,a.elf,'_emos_uart0_async_owned'))]
 assert ('in0','a,(0xc1)') in owned and ('in0','a,(0xc5)') in owned
 assert not any(op in ('call','ei','di','otirx') for op,_ in owned)
 assert len(image)<=131072
 print('EMOS RFC25 low-ROM API/TX-before-RX saves/fullFIFO/tail ownership/ROM verified; hardware service gaps unqualified')
if __name__=='__main__':main()
