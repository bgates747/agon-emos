"""Exercise the MOSlet's actual fixed envelope and response/error handling."""
from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "flow.h"
static unsigned op,arg,result,bad_length,calls;
uint32_t getsysvar_time(void){return 0x12345678;}
static uint32_t u24(const uint8_t *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16);}
uint32_t emos_gateway_call(uint8_t *r){
 ++calls;assert(r[0]==66 && r[1]==0 && r[2]==1 && r[3]==0);
 assert(r[4]==3 && r[5]==8 && r[6]==2 && r[7]==0);
 assert(!memcmp(r+25,"ext",3) && !memcmp(r+41,"uartdiag",8));
 assert(u24(r+13)==2 && u24(r+19)==2 && u24(r+22)==0);
 for(unsigned i=28;i<41;i++)assert(r[i]==0);
 for(unsigned i=49;i<66;i++)assert(r[i]==0);
 uint8_t *in=(void *)(uintptr_t)u24(r+10),*out=(void *)(uintptr_t)u24(r+16);
 assert(in[0]==op && in[1]==arg);
 out[0]=1;out[1]=0xab;r[22]=bad_length?1:2;
 return result;
}
int main(void){
 BYTE value=0;
 op=0;assert(flow_open());op=1;assert(flow_get(&value)==1 && value==0xab);
 op=2;arg=255;assert(flow_put(255)==1);op=3;arg=1;assert(flow_ready(1)==1);
 op=4;arg=0;flow_close();
 op=0;result=27;assert(!flow_open());
 op=1;value=0x42;assert(flow_get(&value)==3 && value==0x42);
 result=0;bad_length=1;assert(flow_get(&value)==3 && value==0x42);
 assert(emos_uart_flow_clock()==0x78 && calls==8);
 return 0;
}
'''
class AdapterTests(unittest.TestCase):
 def test_actual_envelope_and_old_firmware_refusal(self):
  with tempfile.TemporaryDirectory() as tmp:
   d=Path(tmp);(d/'agon').mkdir()
   (d/'agon/mos.h').write_text('#include <stdint.h>\ntypedef uint32_t uint24_t;\nuint32_t getsysvar_time(void);\n')
   (d/'test.c').write_text(HARNESS)
   exe=d/'test'
   subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror','-Wno-pointer-to-int-cast','-no-pie',
    '-fsanitize=address,undefined','-I'+str(d),'-I'+str(ROOT/'projects/uartflow/src'),
    str(ROOT/'projects/uartflow/src/gateway.c'),str(d/'test.c'),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True,timeout=10)
if __name__=='__main__':unittest.main()
