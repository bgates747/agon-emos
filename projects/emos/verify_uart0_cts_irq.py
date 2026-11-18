#!/usr/bin/env python3
"""Linked CTS-rearm leaves with explicit UART stubs; no silicon/whole-ISA claim.

The tested instructions are fetched from the actual linked ROM. Model pin edges
at each defer instruction boundary, model MSR latch acknowledgement, then run
only the resulting genuine modem IRQ. No VBlank or foreground call is supplied.
"""
import argparse,copy
from pathlib import Path
from verify_abi import symbols


def run(image,s,label,hw,edges=()):
    h=copy.deepcopy(hw);h.setdefault('latch',False);h.setdefault('trace',[])
    mem={s['uart0_tx_retry_pending']:h['pending']};pc=s[label];a=0x59;z=False
    irq=label!='uart0_async_retry';stack=[None]+([0x59] if irq else []);peak=len(stack)*3
    def u24(p):return int.from_bytes(image[p:p+3],'little')
    def edge():h['pin']=not h['pin'];h['latch']=True;h['trace'].append(('edge',h['pin']))
    for step in range(100):
        if step in edges:edge()
        peak=max(peak,len(stack)*3);at=pc;op=image[pc];pc+=1
        if op==0x3e:a=image[pc];pc+=1
        elif op==0x3a:p=u24(pc);pc+=3;a=mem.get(p,0)
        elif op==0x32:
            p=u24(pc);pc+=3;assert p==s['uart0_tx_retry_pending'];mem[p]=a;h['trace'].append(('pending',a))
        elif op in (0xe6,0xf6):
            v=image[pc];pc+=1;a=a&v if op==0xe6 else a|v;z=a==0
        elif op==0xaf:a=0;z=True
        elif op==0xb7:z=a==0
        elif op==0xed:
            sub=image[pc];p=image[pc+1];pc+=2
            if sub==0x38:
                assert p in (0xa2,0xc1,0xc6),'Only CTS/IER/MSR reads'
                a=(8 if h['pin'] else 0) if p==0xa2 else h['ier'] if p==0xc1 else ((0 if h['pin'] else 16)|(1 if h['latch'] else 0))
                h['trace'].append(('read',p,a))
                if p==0xc6:h['latch']=False
            elif sub==0x39:
                assert p==0xc1,'No THR/GPIO writes';h['ier']=a;h['trace'].append(('ier',a))
            elif sub==0x64:z=(a&p)==0
            else:raise AssertionError(('ED',sub,at))
        elif op==0xcd:
            target=u24(pc);pc+=3
            if target==s.get('_emos_neta_uart0_exit'):h['trace'].append(('existing-guard-exit',True))
            else:assert target==s['uart0_async_retry'];stack.append(pc);pc=target
        elif op in (0xc9,0xc8,0xc0):
            if op==0xc9 or (op==0xc8 and z) or (op==0xc0 and not z):
                pc=stack.pop()
                if pc is None:break
        elif op==0xf1:assert irq;a=stack.pop();assert a==0x59
        elif op==0xfb:assert irq;h['trace'].append(('EI',True))
        elif op==0x5b:
            assert image[pc:pc+2]==bytes.fromhex('ed4d');pc+=2
            assert irq and stack.pop() is None;break
        else:raise AssertionError(('unsupported linked opcode',hex(op),hex(at)))
    else:raise AssertionError('Unbounded rearm leaf')
    assert not stack,'Stack imbalance'
    h['pending']=mem[s['uart0_tx_retry_pending']];h['steps']=step+1;h['stack_peak']=peak
    assert (h['ier']&~10)==(hw['ier']&~10),'Lost RX/other IER bits'
    return h


def verify(image,s):
    p=s['uart0_tx_retry_pending'].to_bytes(3,'little');call=lambda n:b'\xcd'+s[n].to_bytes(3,'little')
    # Existing initially-clear modem ownership is checked BEFORE descriptor pushes.
    start=s['emos_uart0_rfc25_submit'];prefix=bytes.fromhex('ed57 e2')
    guard=bytes.fromhex('f3 ed38c1 ed6408 2804 3e1f fb c9 e5')
    assert guard in image[start:start+70], 'Pre-mutation modem owner refusal missing'
    # Initial IIR keeps bit0 so NO-PENDING(1) is never dispatched as modem(0).
    seq=bytes.fromhex('ed38c2 e60f ca')+s['uart0_cts_irq'].to_bytes(3,'little')+bytes.fromhex('e60e fe02 ca')+s['fill_uart0_tx_fifo'].to_bytes(3,'little')
    assert seq in image[s['_uart0_handler']:s['_uart0_handler']+40]
    for raw in range(256):
        modem=(raw&15)==0
        if raw&1:assert not modem,'No-pending IRQ misclassified'
        if (raw&14)in (4,6,12):assert not modem,'RX/status cause diverted'
    total=0;maximum=0
    # Existing entry may contain any unrelated interrupt bits, which survive.
    for ier in (1,3,7,0xf7):
        h=dict(ier=ier,pending=0,pin=True)
        base=run(image,s,'uart0_tx_defer',h)
        assert base['pending']==1 and base['ier']==((ier&~2)|8)
        t=base['trace'];pub=t.index(('pending',1));arm=next(i for i,e in enumerate(t)if e[0]=='ier' and e[1]&8)
        ack=next(i for i,e in enumerate(t)if e[:2]==('read',0xc6));assert pub<arm<ack
        # Genuine CTS-ready edge after IRQ exit resumes without a VBlank.
        base['pin']=False;base['latch']=True
        ready=run(image,s,'uart0_cts_irq',base)
        assert ready['pending']==0 and ready['ier']==((ier&~8)|2) and not ready['latch'];total+=1
        # An edge at any instruction boundary during arm/ack/sample is either
        # caught immediately or remains latched to invoke the same IRQ.
        for edge_step in range(base['steps']):
            v=run(image,s,'uart0_tx_defer',h,(edge_step,))
            if v['pin']: # edge scheduled after a shorter return: apply it now
                v['pin']=False;v['latch']=True
            if v['pending']:
                assert v['ier']&8 and v['latch'],'Lost CTS-ready edge'
                v=run(image,s,'uart0_cts_irq',v)
            assert v['pending']==0 and v['ier']&2 and not v['ier']&8
            maximum=max(maximum,v['stack_peak']);total+=1
        # A falling-ready then reblocked pulse before acknowledgement/sample
        # must not advance the source; a subsequent real ready edge resumes it.
        for first in range(4):
            v=run(image,s,'uart0_tx_defer',h,(first,first+1))
            assert v['pin'] and v['pending'] and not v['ier']&2
            v['pin']=False;v['latch']=True;v=run(image,s,'uart0_cts_irq',v)
            assert not v['pending'] and v['ier']&2;total+=1
    # Spurious/stale cause after release acknowledges only, no reenable/copy.
    h=run(image,s,'uart0_cts_irq',dict(ier=1,pending=0,pin=False,latch=True))
    assert h['ier']==1 and not h['pending'] and not h['latch'];total+=1
    # VBlank fallback uses the exact same leaf, still works without modem cause.
    h=run(image,s,'uart0_async_retry',dict(ier=9,pending=1,pin=False))
    assert h['ier']==3 and not h['pending'];total+=1
    # Missing edge arm and missing acknowledgement are meaningful mutations.
    bad=bytearray(image);at=s['uart0_tx_defer'];where=image.index(bytes.fromhex('f608'),at,at+35);bad[where+1]=0
    try:
        v=run(bad,s,'uart0_tx_defer',dict(ier=3,pending=0,pin=True));assert v['ier']&8
    except AssertionError:pass
    else:raise AssertionError('Missing modem arm mutant escaped')
    bad=bytearray(image);at=s['uart0_cts_irq'];assert image[at:at+3]==bytes.fromhex('ed38c6');bad[at+2]=0xa2
    v=run(bad,s,'uart0_cts_irq',dict(ier=9,pending=1,pin=False,latch=True))
    assert v['latch'],'Missing MSR-ack mutant must retain unacknowledged edge'
    print('CTS IRQ linked leaf/peripheral-model cases',total,'max modeled stack bytes',maximum,'(explicit UART/guard stubs; no silicon timing)')


def main():
    p=argparse.ArgumentParser()
    for k in ('source','elf','nm','objdump'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();s=symbols(a.nm,a.elf)
    if 'uart0_cts_irq' not in s:
        print('CTS IRQ option OFF; no modem handler linked');return
    verify(a.elf.with_suffix('.bin').read_bytes(),s)
if __name__=='__main__':main()
