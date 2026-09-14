"""Pin the private diagnostic ABI to a verified ROM/ELF, never guessed addresses."""
from pathlib import Path
import argparse,hashlib,json,subprocess,zlib,datetime
p=argparse.ArgumentParser();p.add_argument('--image',type=Path,required=True);p.add_argument('--elf',type=Path,required=True);p.add_argument('--nm',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parent;out=root/'build';out.mkdir(exist_ok=True)
image=a.image.read_bytes();symbols={}
for line in subprocess.check_output([str(a.nm),'-a',str(a.elf)],text=True).splitlines():
 parts=line.split()
 if len(parts)==3:
  try:symbols[parts[2]]=int(parts[0],16)
  except ValueError:pass
names={'ROM_MAINBOARD_PUT':'_emos_mainboard_put','ROM_UART0_IRQ':'_uart0_handler','ROM_UART1_IRQ':'_emos_keyboard_irq_entry','ROM_UART0_RX':'UART0_serial_RX','ROM_PROTOCOL':'vdp_protocol','ROM_PROTOCOL_DATA':'_vdp_protocol_data'}
values={key:symbols[name] for key,name in names.items()};values.update(ROM_VECTOR0=symbols['__2nd_jump_table']+0x31,ROM_VECTOR1=symbols['__2nd_jump_table']+0x35)
# Bind the UART0 copied envelope to actual original linked bytes. The observer
# adds counters/raw recording only; calls this ROM's read and parser routines.
save=bytes.fromhex('f3 f5 c5 d5 e5 dd e5 fd e5 08 f5 d9 c5 d5 e5')
restore=bytes.fromhex('e1 d1 c1 d9 f1 08 fd e1 dd e1 e1 d1 c1 f1 fb 5b ed 4d')
expected=save+b'\xcd'+values['ROM_UART0_RX'].to_bytes(3,'little')+b'\x4f\x21'+values['ROM_PROTOCOL_DATA'].to_bytes(3,'little')+b'\xcd'+values['ROM_PROTOCOL'].to_bytes(3,'little')+restore
start=values['ROM_UART0_IRQ'];assert image[start:start+len(expected)]==expected,'Unexpected UART0 envelope'
assert len(image)<=131072
manifest=json.loads(a.image.with_name('manifest.json').read_text())
assert manifest['sha256']==hashlib.sha256(image).hexdigest()
build='integ014-dual-uart-r01-b'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%d-%H-%M-%SZ')
(out/'firmware_abi.inc').write_text('\n'.join(f'.equ {key},0x{value:06x}' for key,value in values.items())+'\n')
header=['/* Generated private diagnostic binding. Not a public firmware ABI. */',f'#define PROBE_BUILD_ID "{build}"',f'#define ROM_BUILD_ID "{manifest["identity"]}"',f'#define ROM_BYTES {len(image)}U',f'#define ROM_FIRST {image[0]}U',f'#define ROM_CRC32_FROM1 0x{zlib.crc32(image[1:]):08x}UL']+[f'#define {key} 0x{value:06x}U' for key,value in values.items()]
(out/'firmware_abi.h').write_text('\n'.join(header)+'\n')
(out/'binding.json').write_text(json.dumps(dict(build=build,rom_sha256=manifest['sha256'],rom_identity=manifest['identity'],rom_bytes=len(image),symbols=values,original_uart0_envelope_verified=True),indent=2)+'\n')
print(build)
