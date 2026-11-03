"""Stock buffered-VDP byte comparator, carried forward from the UART project.

No private VDP protocol or external build directory is required.
"""
from pathlib import Path
BASE=0x6af0
PAYLOAD=bytes((n*157+7)&255 for n in range(8192))
def word(n):return bytes((n&255,n>>8))
def cmd(n,*body):return bytes((23,0,160))+word(n)+bytes(body)
def clear(n):return cmd(n,2)
def write(n,b):return cmd(n,0,*word(len(b)),*b)
def compare(at,reference):return cmd(BASE+1,6,0x23,*word(BASE),*word(at),*word(BASE+4),*word(reference))
def generate(fixture):
 bad=cmd(BASE+2,5,2,0,0,1)
 reset=cmd(BASE+2,5,2,0,0,0)
 reply=cmd(BASE+3,5,0x22,3,0,*word(BASE+2),0,0)+cmd(BASE+3,1)
 arrays=dict(setup=b''.join(clear(BASE+i) for i in range(5))+write(BASE+1,bad)+write(BASE+2,b'\0')+write(BASE+3,bytes((23,0,128,0)))+write(BASE+4,PAYLOAD),
  reset_flag=reset,reply=reply,mismatch_probe=clear(BASE)+write(BASE,b'\7')+reset+compare(0,1)+reply,
  match_probe=reset+compare(0,0)+reply,cleanup=b''.join(clear(BASE+i) for i in range(5)))
 (fixture/'src/packets.h').write_text(''.join('static const uint8_t v_'+n+'[]={'+','.join(map(str,b))+'};\n' for n,b in arrays.items()))
 (fixture/'checks.bin').write_bytes(b''.join(compare(n,n) for n in range(len(PAYLOAD))))
