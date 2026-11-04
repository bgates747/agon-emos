"""Oryx: freeze five no-argument apps; reuse all five MOS images unchanged."""
from pathlib import Path
import hashlib,importlib.util,json,shutil,sys,zlib
root=Path(__file__).resolve().parents[2];out=root/sys.argv[1];out.mkdir(parents=True,exist_ok=False)
old=root.parent/'agon-mos';appliance=old/'build/uart0-balls/run007/appliance'
cases=[
 ('stock','official-stock','Official stock MOS 3.0.2',0,appliance/'mos-stock.bin','d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae',root.parents[1]/'flexe-emulator/mos_platform.map','mos-stock.bin'),
 ('eirene','eirene-stock','Unmodified Eirene',0,root/'build/tm001/baseline/bin/mos.bin','2110243ec967b16426933f8ffbd411649e51eb47dea780a49f1644e5421e2a64',root/'build/tm001/baseline/bin/mos.map','mos-eirene.bin'),
 ('scalar','eirene-scalar','Minimal Eirene scalar async',1,root/'build/tm001/scalar003/bin/mos.bin','383167c7ae6220472378b366c63c81b9df5e3e63fe5d827233d4c2276c287965',root/'build/tm001/scalar003/bin/mos.map','mos-scalar.bin'),
 ('otirx','eirene-otirx','Minimal Eirene OTIRX async',1,root/'build/tm001/otirx003/bin/mos.bin','7db662439e33badd561cefe26e7a7a643d9f932da1031bce20d60217cd1e03af',root/'build/tm001/otirx003/bin/mos.map','mos-otirx.bin'),
 ('ours','previous-otirx','Previous full OTIRX borrowed/direct',2,appliance/'mos-candidate.bin','cb1d0f4e066395533a2f07c7454246fa3a1ed46134bf155b883bb8c6e8353128',appliance/'mos-candidate.map','mos-ours.bin')]
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');packets=importlib.util.module_from_spec(spec);spec.loader.exec_module(packets)
rows=[]
for command,case,name,transport,mos,pin,mos_map,firmware_name in cases:
 assert h(mos)==pin;data=mos.read_bytes();crc=zlib.crc32(data)
 f=out/command/'fixture';shutil.copytree(root/'experiments/uart0-performance2/src',f/'src');shutil.copyfile(root/'experiments/uart0-performance2/Makefile',f/'Makefile')
 (f/'Makefile').write_text((f/'Makefile').read_text().replace('NAME = test','NAME = '+command));packets.generate(f)
 if transport==2:
  extra=root/'experiments/uart0-performance/extra'
  shutil.copyfile(extra/'legacy.c',f/'src/zz_legacy.c');shutil.copyfile(extra/'legacy.src',f/'src/zz_gate.src')
  p=f/'src/api.src';asm=p.read_text();a=asm.index('_tm_write:');b=asm.index('.global _rom_byte0',a)
  p.write_text(asm[:a]+'.extern _legacy_write,_legacy_finish\n_tm_write:\n jp _legacy_write\n_tm_finish:\n jp _legacy_finish\n'+asm[b:])
 else:(f/'src/zz_legacy_stub.c').write_text('int legacy_init(void){return 1;}\nint legacy_cleanup(void){return 1;}\n')
 files={str(p.relative_to(f)):h(p) for p in sorted(f.rglob('*')) if p.is_file()}
 descriptor=dict(command=command,case=case,name=name,transport=transport,mos_bytes=len(data),mos_crc=crc,MOS_sha256=pin,map_sha256=h(mos_map),firmware=firmware_name)
 build=hashlib.sha256(json.dumps(dict(source=files,config=descriptor),sort_keys=True).encode()).hexdigest()[:16]
 (f/'src/zz_config.c').write_text('#include "config.h"\nconst struct FixtureConfig fixture={'+','.join(json.dumps(v) for v in (case,command,name,build))+','+str(transport)+','+str(len(data))+','+hex(crc)+'UL};\n')
 descriptor.update(build=build,source=files,config_sha256=h(f/'src/zz_config.c'),fixture=str(f),app=command+'.bin',mos=str(mos),mos_map=str(mos_map),work_steps=1,report_version=2)
 (out/command/'source.json').write_text(json.dumps(descriptor,indent=2)+'\n');shutil.copyfile(mos,out/command/'mos.bin');shutil.copyfile(mos_map,out/command/'mos.map');rows.append(descriptor)
(out/'cases.json').write_text(json.dumps(dict(schema='PERF2-five-1',reference='official-stock',cases=rows),indent=2)+'\n');print(json.dumps([dict(command=r['command'],build=r['build'],transport=r['transport']) for r in rows]))
