"""Run the retained gateway's actual validation/routing code in mapped guest RAM."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
PREAMBLE=r'''
#define _GNU_SOURCE
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define TRUE 1
#define FALSE 0
#define MOS_DEFINES_H
#include <defines.h>
#include "emos.h"
#define FR_OK 0
#define FR_INVALID_PARAMETER 19
#define FR_TIMEOUT 15
#define EMOS_NOT_FOUND 27
#define EMOS_BUSY 31
#define EMOS_UNSAFE_CALLER 32
#define EMOS_UNAVAILABLE 35
#define EMOS_TEXT_LIMIT 512
static BOOL emosBusy;
static struct { BYTE mode; } emosModeState;
static unsigned sdCalls, textCalls;
static BYTE emos_text_valid(const BYTE *p, UINT16 n) { return n && *p=='A'; }
static BYTE emos_text_probe(const BYTE *p, UINT16 n) {
    assert(emosBusy && n==1 && *p=='A'); ++textCalls;return 1;
}
static UINT24 emos_sdlink_gateway(t_emosGatewayRequest *r) {
    assert(emosBusy && r);++sdCalls;return 15;
}
'''
HARNESS=r'''
static t_emosGatewayRequest *request(unsigned address,const char *ns,const char *name) {
    t_emosGatewayRequest *r=(void *)(uintptr_t)address;
    memset(r,0,sizeof(*r));r->size[0]=66;r->abiMajor=1;r->operation[0]=2;
    r->namespaceLength=strlen(ns);r->nameLength=strlen(name);
    memcpy(r->namespaceName,ns,r->namespaceLength);memcpy(r->providerName,name,r->nameLength);
    return r;
}
int main(void) {
    t_emosGatewayRequest *r;
    assert(mmap((void *)0x40000,0x80000,PROT_READ|PROT_WRITE,
           MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==(void *)0x40000);
    assert(sizeof(*r)==66);
    assert(emos_gateway(NULL)==32);
    r=request(0xB7FBF,"ext","sdlink");assert(emos_gateway(r)==32);
    r=request(0xB0000,"ext","sdlink");
    assert(emos_gateway(r)==15 && sdCalls==1 && !emosBusy);
    emosBusy=1;assert(emos_gateway(r)==31 && sdCalls==1);emosBusy=0;
    r=request(0xB0000,"core","echo");assert(emos_gateway(r)==32);
    r=request(0x40000,"core","echo");memset(r->outputLength,0xFF,3);
    assert(emos_gateway(r)==27 && emos_read24(r->outputLength)==0);
    r=request(0x40000,"ext","sdlink");r->abiMajor=2;assert(emos_gateway(r)==19);
    r->abiMajor=1;r->operation[0]=1;assert(emos_gateway(r)==19);
    r->operation[0]=2;r->flags[0]=1;assert(emos_gateway(r)==19);
    r->flags[0]=0;r->namespaceName[15]=1;assert(emos_gateway(r)==19);
    r=request(0x40000,"edu","text-probe");
    emos_write24(r->input,0x41000);emos_write24(r->inputLength,1);
    *(BYTE *)0x41000='A';
    assert(emos_gateway(r)==0 && textCalls==1 && !emosBusy);
    emosModeState.mode=2;assert(emos_gateway(r)==35 && textCalls==1);
    emosModeState.mode=0;emosBusy=1;assert(emos_gateway(r)==31);emosBusy=0;
    emos_write24(r->input,0xB0000);assert(emos_gateway(r)==19);
    r=request(0xB0000,"edu","text-probe");assert(emos_gateway(r)==32);
    puts("Resident gateway validation and admission passed");
}
'''

class ResidentGatewayTests(unittest.TestCase):
    def test_retained_gateway(self):
        s=(ROOT/'src/emos.c').read_text()
        helpers=s[s.index('static UINT16 emos_read16'):s.index('static void emos_print_identity')]
        # write16 is used by CLI service construction, not the gateway itself.
        a=helpers.index('static void emos_write16');b=helpers.index('static void emos_write24',a)
        helpers=helpers[:a]+helpers[b:]
        gateway=s[s.index('static BOOL emos_range_overlaps_module'):s.index('BYTE emos_application_enter')]
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp);c=d/'test.c';exe=d/'test'
            c.write_text(PREAMBLE+helpers+gateway+HARNESS)
            subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror',
                '-Wno-endif-labels','-Wno-misleading-indentation','-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast','-fsanitize=undefined',
                '-I'+str(ROOT/'tests/host'),'-iquote'+str(ROOT/'src'),str(c),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True,timeout=10)

if __name__=='__main__':unittest.main()
