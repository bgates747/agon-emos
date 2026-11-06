#!/usr/bin/env python3
"""INTEG-023 UART-NM composition: no resident parallel network admission."""
import argparse
from pathlib import Path
from verify_abi import symbols
from verify_parallel import disassemble_global_extent

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('source','elf','nm','objdump'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();s=symbols(a.nm,a.elf)
    for name in ('_emos_net_gateway','_emos_net_packet','_emos_net_reset',
                 '_emos_reply_callback','emos_uart0_rfc25_submit','_emos_rfc25_admit'):
        assert name in s,name
    for name in ('_emos_netp_gateway','_emos_netp_link_open','_emos_netp_token'):
        assert name not in s,name
    packet=disassemble_global_extent(a.objdump,a.elf,'_emos_net_packet')
    assert '_emos_reply_callback' in packet
    assert a.elf.with_suffix('.bin').stat().st_size<=131072
    print('UART-NM admission/callback and guarded RFC25 linked; no NETP owner; ROM fits')

if __name__=='__main__':main()
