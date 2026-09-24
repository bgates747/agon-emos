"""Execute the production launcher with bounded filesystem/run test doubles.

Target ABI and real filesystem behavior are separately checked under Fab.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PREAMBLE = r'''
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>
typedef uintptr_t UINT24;
typedef struct { unsigned fattrib; unsigned long fsize; } FILINFO;
#define EMOS_NAME_SIZE 24
#define EMOS_MODULE_BASE 0xB0000
#define EMOS_MODULE_SIZE 0x8000
#define EMOS_POLICY_CORE 0
#define EMOS_BUSY 31
#define FR_OK 0
#define FR_INVALID_PARAMETER 19
#define MOS_INVALID_EXECUTABLE 20
#define MOS_OVERLAPPING_SYSTEM 21
#define AM_DIR 16
static unsigned emosBusy, emosPolicy, statCalls, loadCalls, runCalls;
static int statResult, loadResult, runResult;
static FILINFO fileInfo;
static char seenPath[64], seenArgs[128];
static int f_stat(char *path, FILINFO *info) {
    ++statCalls; strcpy(seenPath,path); *info=fileInfo; return statResult;
}
static int mos_LOAD(char *path, UINT24 address, UINT24 size) {
    ++loadCalls;
    assert(!strcmp(path,seenPath));
    assert(address==EMOS_MODULE_BASE && size==EMOS_MODULE_SIZE);
    assert(!memcmp((void *)(address+0x40),"\0\0\0\0\0",5));
    return loadResult;
}
static int mos_runBin(UINT24 address, char *args) {
    ++runCalls; assert(address==EMOS_MODULE_BASE);
    strcpy(seenArgs,args ? args : ""); return runResult;
}
'''
HARNESS = r'''
static void reset(void) {
    emosBusy=emosPolicy=statCalls=loadCalls=runCalls=0;
    statResult=loadResult=runResult=0;
    fileInfo.fattrib=0; fileInfo.fsize=100;
    memset((void *)EMOS_MODULE_BASE,0xA5,EMOS_MODULE_SIZE);
    memset((void *)0x40000,0x5A,32);
}
int main(void) {
    assert(mmap((void *)0x40000,32,PROT_READ|PROT_WRITE,
           MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==(void *)0x40000);
    assert(mmap((void *)EMOS_MODULE_BASE,EMOS_MODULE_SIZE,PROT_READ|PROT_WRITE,
           MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==(void *)EMOS_MODULE_BASE);
    reset(); runResult=7;
    assert(emos_run_utility("MiXeD_Name-1","one  two \"three\"")==7);
    assert(!strcmp(seenPath,"/emos/mixed_name-1.bin"));
    assert(!strcmp(seenArgs,"one  two \"three\""));
    assert(loadCalls==1 && runCalls==1);
    for(unsigned i=0;i<32;++i) assert(((unsigned char *)0x40000)[i]==0x5A);
    reset(); assert(emos_run_utility("abcdefghijklmnopqrstuvwx",NULL)==0);
    assert(!strcmp(seenPath,"/emos/abcdefghijklmnopqrstuvwx.bin"));
    const char *bad[]={"","abcdefghijklmnopqrstuvwxy","../x","/x","x.bin",
                       "x/y","x\\y","$x","x*","x y","x\x80"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        reset(); assert(emos_run_utility(bad[i],"")==FR_INVALID_PARAMETER);
        assert(statCalls==0 && loadCalls==0 && runCalls==0);
    }
    for(unsigned policy=1;policy<4;++policy) {
        reset(); emosPolicy=policy;
        assert(emos_run_utility("sdserve","")==EMOS_BUSY && statCalls==0);
        assert(*(unsigned char *)EMOS_MODULE_BASE==0xA5);
    }
    reset(); emosBusy=1;
    assert(emos_run_utility("sdserve","")==EMOS_BUSY && statCalls==0);
    for(int error=1;error<=19;++error) {
        reset(); statResult=error;
        assert(emos_run_utility("probe","")==error && loadCalls==0 && runCalls==0);
        reset(); loadResult=error;
        assert(emos_run_utility("probe","")==error && loadCalls==1 && runCalls==0);
    }
    reset(); fileInfo.fattrib=AM_DIR;
    assert(emos_run_utility("probe","")==MOS_INVALID_EXECUTABLE && loadCalls==0);
    reset(); fileInfo.fsize=68;
    assert(emos_run_utility("probe","")==MOS_INVALID_EXECUTABLE && loadCalls==0);
    reset(); fileInfo.fsize=69;
    assert(emos_run_utility("probe","")==0 && runCalls==1);
    reset(); fileInfo.fsize=32769;
    assert(emos_run_utility("probe","")==MOS_OVERLAPPING_SYSTEM && loadCalls==0);
    reset(); fileInfo.fsize=32768;
    assert(emos_run_utility("probe","")==0 && runCalls==1);
    puts("EMOS utility admission and error tests passed");
}
'''

class EmosUtilityTests(unittest.TestCase):
    def test_production_launcher_boundaries(self):
        source=(ROOT/'src/emos.c').read_text()
        helper=source[source.index('static int emos_run_utility('):source.index('int emos_cmd(')]
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); c=root/'test.c'; exe=root/'test'
            c.write_text(PREAMBLE+helper+HARNESS)
            subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror',
                            '-fsanitize=undefined',str(c),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True,timeout=10)

if __name__=='__main__': unittest.main()
