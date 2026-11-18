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
 cts='uart0_cts_irq' in s
 prefix=bytes.fromhex('f3 f5 ed38c2')
 if cts:prefix+=bytes.fromhex('e60f ca')+addr('uart0_cts_irq')
 prefix+=bytes.fromhex('e60e fe02 ca')+addr('fill_uart0_tx_fifo')
 exact('_uart0_handler',prefix+b'\xc5\xd5\xe5\xdd\xe5\xfd\xe5\x08\xf5\xd9\xc5\xd5\xe5')
 # Final fill source release disables THRE then executes the caller thunk.
 for label in ('fill_uart0_tx_fifo','.last_fill'):
  rows=[(op,arg) for _,op,arg in instructions(disassemble_global_extent(a.objdump,a.elf,label))]
  assert ('otirx','') in rows,label
 raw=disassemble_global_extent(a.objdump,a.elf,'fill_uart0_tx_fifo')
 assert raw.count('reti.lil')==(3 if cts else 2) and raw.count('otirx')==2
 # The bounded guard preserves caller IFF when pending can change in the IRQ.
 owned=[(op,arg) for _,op,arg in instructions(disassemble_global_extent(a.objdump,a.elf,'_emos_uart0_async_owned'))]
 assert ('in0','a,(0xc1)') in owned and ('in0','a,(0xc5)') in owned
 if cts:
  exact('_emos_uart0_async_owned',bytes.fromhex('c5 ed57 f3 f5 cd')+addr('rfc25_owned_locked')+bytes.fromhex('4f f1 79 c1 e0 fb c9'))
  exact('fill_uart0_tx_fifo',bytes.fromhex('ed38a2 ed6408 c2')+addr('uart0_tx_defer'))
 else:assert not any(op in ('call','ei','di','otirx') for op,_ in owned)
 assert len(image)<=131072
 print('EMOS RFC25 low-ROM API/TX-before-RX saves/fullFIFO/tail ownership/ROM verified; hardware service gaps unqualified')
if __name__=='__main__':main()
