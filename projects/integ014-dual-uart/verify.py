"""Check linked observer register envelopes and pinned ROM calls before hardware."""
from pathlib import Path
import hashlib,json,re
root=Path(__file__).resolve().parent
binary=(root/'bin/DUALUART.bin').read_bytes();text=(root/'bin/DUALUART.map').read_text();binding=json.loads((root/'build/binding.json').read_text())
symbols={m[1]:int(m[0],16) for m in re.findall(r'^\s+(0x[0-9a-f]+)\s+([_A-Za-z][_A-Za-z0-9]*)\s*$',text,re.M)}
def addr(name):return symbols[name].to_bytes(3,'little')
def rom(name):return binding['symbols'][name].to_bytes(3,'little')
def get(name,n):start=symbols[name]-0x40000;assert start>=0;return binary[start:start+n]
expected=b'\xf3\xe5\x2a'+addr('_diag1_count')+b'\x23\x22'+addr('_diag1_count')+b'\xe1\xc3'+rom('ROM_UART1_IRQ')
assert get('_diag1_irq',len(expected))==expected,'UART1 shim changes flags/registers or target'
assert get('_diag_rom_first',5)==b'\x3a\0\0\0\xc9','ROM byte-zero read must be assembly'
assert get('_diag_uart0_put',4)==b'\xc3'+rom('ROM_MAINBOARD_PUT')
start=symbols['_diag0_irq']-0x40000  # fixed reviewed assembly-function extent
code=binary[start:start+91]
assert code.startswith(bytes.fromhex('f3 f5 c5 d5 e5 dd e5 fd e5 08 f5 d9 c5 d5 e5'))
assert code.endswith(bytes.fromhex('e1 d1 c1 d9 f1 08 fd e1 dd e1 e1 d1 c1 f1 fb 5b ed 4d'))
assert code.count(b'\xcd'+rom('ROM_UART0_RX'))==1 and code.count(b'\xcd'+rom('ROM_PROTOCOL'))==1
assert symbols['_diag0_left']>=0x40000 and symbols['_diag0_left']<0xB0000
result=dict(binding,bytes=len(binary),sha256=hashlib.sha256(binary).hexdigest(),linked_observer_checks=True)
(root/'build/artifact.json').write_text(json.dumps(result,indent=2)+'\n');print('Pinned observer ABI and register envelopes verified:',result['sha256'])
