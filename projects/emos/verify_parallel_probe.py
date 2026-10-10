#!/usr/bin/env python3
"""PORT-008 fixed qualification composition; retains all native/ordinary guards."""
import argparse
from pathlib import Path
from verify_parallel_native import verify_native
p=argparse.ArgumentParser(description=__doc__)
for n in ("source","elf","nm","objdump"):p.add_argument("--"+n,type=Path,required=True)
a=p.parse_args()
verify_native(a.source,a.elf,a.nm,a.objdump,qualification=True)
print("EMOS private probe: sole gateway activation, all native/ordinary checks retained")
