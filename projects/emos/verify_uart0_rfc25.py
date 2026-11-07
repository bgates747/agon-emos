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
 assert raw.count('reti.lil')==3 and raw.count('otirx')==2
 # Preserve caller IFF and BC around the COMPLETE atomic ownership snapshot.
 # Foreground mode request and already-DI admit callers use the same leaf.
 exact('_emos_uart0_async_owned',bytes.fromhex('c5 ed57 f3 f5 cd')+addr('rfc25_owned_locked')+bytes.fromhex('4f f1 79 c1 e0 fb c9'))
 owned=[(op,arg) for _,op,arg in instructions(disassemble_global_extent(a.objdump,a.elf,'rfc25_owned_locked'))]
 assert ('in0','a,(0xc1)') in owned and ('in0','a,(0xc5)') in owned
 assert not any(op in ('call','ei','di','otirx') for op,_ in owned)
 # Exact bounded donor defer/rearm and EMOS pending-before-IER ownership.
 pending=addr('uart0_tx_retry_pending')
 exact('uart0_tx_defer',bytes.fromhex('ed38c1 e6fd ed39c1 3e01 32')+pending+bytes.fromhex('f1 fb 5bed4d'))
 exact('uart0_async_retry',bytes.fromhex('3a')+pending+bytes.fromhex('b7 c8 ed38a2 ed6408 c0 af 32')+pending+bytes.fromhex('ed38c1 f602 ed39c1 c9'))
 head=s['fill_uart0_tx_fifo']
 assert image[head:head+7]==bytes.fromhex('ed38a2 ed6408 20')
 assert head+8+int.from_bytes(image[head+7:head+8],'little',signed=True)==s['uart0_tx_defer']
 gate=s['rfc25_owned_locked'];seq=bytes.fromhex('3a')+pending+bytes.fromhex('b7 20')
 off=image.find(seq,gate,gate+32);ier=image.find(bytes.fromhex('ed38c1'),gate,gate+48)
 assert gate<off<ier,'Pending borrow must precede IER/TEMT completion'
 assert off+7+int.from_bytes(image[off+6:off+7],'little',signed=True)==s['rfc25_busy']
 vb=s['_vblank_handler'];call=b'\xcd'+addr('uart0_async_retry')
 assert image[vb:vb+80].count(call)==1,'Bounded rearm missing from saved VBlank'
 source=(a.source/'src/emos_uart0_rfc25_io.asm').read_text().lower()
 assert 'wait_cts' not in source,'Unbounded optional TX wait remains'
 assert source.index('ld (uart0_tx_retry_pending),a')>source.index('jr nz,rfc25_return')
 assert len(image)<=131072
 print('EMOS RFC25 low-ROM API/TX-before-RX saves/fullFIFO/tail ownership/ROM verified; hardware service gaps unqualified')
if __name__=='__main__':main()
