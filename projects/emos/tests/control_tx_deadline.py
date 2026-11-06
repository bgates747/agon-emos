#!/usr/bin/env python3
"""Execute actual C deadline/reply functions with a controlled host clock.

Native six-byte initialization/capture is checked separately against ELF bytes;
the host's wider UINT24 surrogate is not a native layout or timing claim.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def function(path, signature):
    s=path.read_text();start=s.index(signature);a=s.index('{',start);depth=1;i=a+1
    while depth:
        if s[i]=='{':depth+=1
        if s[i]=='}':depth-=1
        i+=1
    return s[start:i]


def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);a=p.parse_args()
    body=r'''
#include <assert.h>
#include <stdint.h>
typedef uint8_t BYTE;typedef uint16_t UINT16;typedef uint32_t UINT24;
typedef struct {BYTE last;UINT16 elapsed;UINT24 budget;} t_emosDeadline;
typedef t_emosDeadline Deadline;
static BYTE tick,advance,ready_at, fault_at,emos_key_faulted;
static volatile BYTE ready;
BYTE emos_keyboard_clock(void){
    BYTE r=tick;if(advance)++tick;
    if(ready_at&&tick==ready_at)ready=1;
    if(fault_at&&tick==fault_at)emos_key_faulted=1;
    return r;
}
/* Host substitute only; native initializer bytes are independently checked. */
void emos_keyboard_deadline_start(t_emosDeadline*d){d->last=emos_keyboard_clock();d->elapsed=0;d->budget=262144;}
'''
    body+=function(a.source/'src/emos_keyboard.c','BYTE emos_keyboard_deadline_step')+'\n'
    body+=function(a.source/'src/emos_console.c','static BYTE wait_reply')+'\n'
    body+=r'''
int main(void){
    t_emosDeadline d={250,0,2};tick=5;
    assert(emos_keyboard_deadline_step(&d)&&d.elapsed==11&&d.last==5&&d.budget==262144);
    d.elapsed=598;tick=6;assert(emos_keyboard_deadline_step(&d)&&d.elapsed==599);
    tick=7;assert(!emos_keyboard_deadline_step(&d)&&d.elapsed==600);
    d=(t_emosDeadline){7,0,2};assert(emos_keyboard_deadline_step(&d)&&d.budget==1);
    assert(!emos_keyboard_deadline_step(&d)&&d.budget==0);
    d=(t_emosDeadline){7,0,1};tick=8;
    assert(emos_keyboard_deadline_step(&d)&&d.budget==262144);
    ready=1;assert(wait_reply(&ready));
    emos_key_faulted=1;assert(!wait_reply(&ready));
    emos_key_faulted=0;ready=0;tick=0;advance=1;ready_at=20;
    assert(wait_reply(&ready)&&tick<=21);
    ready=0;ready_at=0;tick=0;fault_at=20;
    assert(!wait_reply(&ready)&&emos_key_faulted&&tick<=21);
    emos_key_faulted=0;fault_at=0;tick=0;
    assert(!wait_reply(&ready));
    advance=0;assert(!wait_reply(&ready));
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='control-tx-deadline-') as d:
        f=Path(d)/'check.c';out=Path(d)/'check';f.write_text(body)
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(f),'-o',str(out)],check=True,timeout=30)
        subprocess.run([str(out)],check=True,timeout=15)
    print('Actual C deadline/reply: wrap,600 boundary,fuse,progress,ready,fault passed')


if __name__=='__main__':main()
