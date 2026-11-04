"""Oryx: freeze/build measured trim steps, functional fixture and matched PERF2 apps."""
from pathlib import Path
import hashlib,importlib.util,io,json,os,re,shutil,subprocess,sys,tarfile,zlib
root=Path(__file__).resolve().parents[2];out=root/sys.argv[1];tool=Path(os.environ['AGONDEV_TOOLCHAIN']).resolve();out.mkdir(parents=True,exist_ok=False)
assert not subprocess.check_output(['git','status','--porcelain'],cwd=root).strip()
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip();archive=subprocess.check_output(['git','archive',commit],cwd=root)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def extract(dst):
 dst.mkdir()
 with tarfile.open(fileobj=io.BytesIO(archive)) as t:t.extractall(dst,filter='data')
def make(dst,log):
 with log.open('w') as f:subprocess.run(['make','AGONDEV_TOOLCHAIN='+str(tool)],cwd=dst,stdout=f,stderr=subprocess.STDOUT,check=True)
def sizes(obj):
 s=subprocess.check_output([str(tool/'bin/ez80-none-elf-objdump'),'-h',str(obj)],text=True)
 return {m[1]:int(m[2],16) for m in re.finditer(r'^\s*\d+\s+(\S+)\s+([0-9a-f]+)',s,re.M)}
rc1root=root/'build/tm001/rc1-003';rc1=rc1root/'firmware';old=rc1/'bin/mos.bin';assert h(old)=='49873fb817f47cc76f50b8cec1c3d9cde7b252ac2ec844e08e745bdaf1c5eeab'
original=subprocess.check_output(['git','show','47dd561:src/uart0_async.asm'],cwd=root,text=True)
rows=[];first=out/'branding';extract(first);(first/'src/uart0_async.asm').write_text(original);make(first,out/'branding.log')
for name in ('branding','checks','wrappers','trim'):
 dst=out/name
 if name!='branding':
  shutil.copytree(first,dst);(dst/'src/uart0_async.asm').write_bytes((root/'experiments/uart0-trim/variants'/(name+'.asm')).read_bytes());make(dst,out/(name+'.log'))
 for file in (dst/'src').rglob('*'):
  if file.is_file() and file.name not in ('version.h','uart0_async.asm'):assert h(file)==h(rc1/'src'/file.relative_to(dst/'src')),file
 for file in (dst/'obj').glob('*.o'):
  if file.name not in ('main.o','uart0_async.o'):assert h(file)==h(rc1/'obj'/file.name),file
 section=sizes(dst/'obj/uart0_async.o');rows.append(dict(name=name,text=section['.text'],bss=section['.bss'],image_bytes=(dst/'bin/mos.bin').stat().st_size,image_sha256=h(dst/'bin/mos.bin'),module_sha256=h(dst/'obj/uart0_async.o')))
 assert h(dst/'obj/main.o')==h(first/'obj/main.o')
repeat=out/'repeat';extract(repeat);make(repeat,out/'repeat.log');assert h(repeat/'bin/mos.bin')==h(out/'trim/bin/mos.bin')
info=dict(source_commit=commit,release_source='47dd56129c0043931bb32f7d329c0392a0b8a809',baseline=dict(text=476,bss=11,image_bytes=101140,image_sha256=h(old)),steps=rows,clean_rebuild_identical=True,toolchain=dict(path=str(tool),tools={n:h(tool/'bin'/n) for n in ('ez80-none-elf-clang','ez80-none-elf-as','ez80-none-elf-ld')},libagon=h(tool/'lib/libagon.a')))
packet_spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');packets=importlib.util.module_from_spec(packet_spec);packet_spec.loader.exec_module(packets)
gate=(root/'experiments/uart0-trim/callback_gate.inc').read_text()
def bridge(text):
 start=text.index('_tm_write:');end=text.index('_tm_finish:',start)
 text=text[:start]+'_tm_write:\n ; Fixture has one known C callback. NULL remains a raw NULL callback.\n ld hl,(_tm_callback)\n ld de,0\n or a\n sbc hl,de\n jr z,tm_args_ready\n ld de,tm_completion_gate\ntm_args_ready:\n ld hl,(_tm_source)\n ld bc,(_tm_length)\n ld a,042h\n rst.lil 08h\n ret\n'+text[end:]
 text=text.replace(',_tm_masked_probe','');text=re.sub(r'_tm_masked_probe:\n di\n call _tm_write\n ei\n ret\n','',text)
 return text+'\ninclude "src/callback_gate.inc"\n.extern _completion\nprotect_callback tm_completion_gate,_completion\n'
def identity(app,mos,descriptor):
 data=mos.read_bytes();crc=zlib.crc32(data);files={str(f.relative_to(app)):h(f) for f in sorted(app.rglob('*')) if f.is_file()};build=hashlib.sha256(json.dumps(dict(files=files,config=descriptor,MOS=h(mos)),sort_keys=True).encode()).hexdigest()[:16]
 (app/'src/identity.h').write_text('#define TM_BUILD "'+build+'"\n#define RC_MOS_BYTES '+str(len(data))+'\n#define RC_MOS_CRC '+hex(crc)+'UL\n')
 return dict(build=build,source=files,MOS_bytes=len(data),MOS_crc32=f'{crc:08X}',MOS_sha256=h(mos))
trim=out/'trim/bin/mos.bin';functional=out/'functional';shutil.copytree(root/'experiments/uart0-release/src',functional/'src');shutil.copyfile(root/'experiments/uart0-release/Makefile',functional/'Makefile');packets.generate(functional)
s=(functional/'src/main.c').read_text().replace('static void completion(void)','void completion(void)').replace('RC1','TRIM1')
s=s.replace('tm_write(void),tm_finish(void),tm_masked_probe(void)','tm_write(void),tm_finish(void)')
s=s.replace('tm_source=zeros;tm_length=16;if(tm_masked_probe()!=3||!finish())ok=0;', 'if(!finish())ok=0; /* Context is now a caller precondition. */')
s=s.replace('Zero-length/masked-context refusal PASS','Zero-length refusal; context is a caller precondition PASS')
(functional/'src/main.c').write_text(s);(functional/'src/api.src').write_text(bridge((functional/'src/api.src').read_text()));(functional/'src/callback_gate.inc').write_text(gate)
functional_info=identity(functional,trim,dict(kind='functional'));make(functional,out/'functional.log');functional_info['app_sha256']=h(functional/'bin/test.bin')
example=out/'example';shutil.copytree(root/'examples/uart0-async/src',example/'src');shutil.copyfile(root/'examples/uart0-async/Makefile',example/'Makefile');shutil.copyfile(root/'experiments/uart0-release/src/rom_crc.h',example/'src/rom_crc.h')
example_info=identity(example,trim,dict(kind='example'));make(example,out/'example.log');example_info['app_sha256']=h(example/'bin/example.bin')
# Match all measured C code across official stock, published RC1 and trial.
# Only export the completion symbol so the fixed-target app wrapper can call it.
stock=root.parent/'agon-mos/build/uart0-balls/run007/appliance/mos-stock.bin';assert h(stock)=='d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae'
cases=[]
for command,case,name,transport,mos,mosmap in (
 ('stock','official-stock','Official stock MOS3.0.2',0,stock,root.parents[1]/'flexe-emulator/mos_platform.map'),
 ('rc1','published-rc1','Published Eirene UART0 RC1',1,old,rc1/'bin/mos.map'),
 ('trim','trim-trial1','Unpublished Eirene UART0 TRIM Trial1',1,trim,out/'trim/bin/mos.map')):
 app=out/'perf'/command/'fixture';shutil.copytree(root/'experiments/uart0-performance2/src',app/'src');shutil.copyfile(root/'experiments/uart0-performance2/Makefile',app/'Makefile');(app/'Makefile').write_text((app/'Makefile').read_text().replace('NAME = test','NAME = '+command));packets.generate(app)
 f=app/'src/main.c';f.write_text(f.read_text().replace('static void completion(void)','void completion(void)'))
 (app/'src/zz_legacy_stub.c').write_text('int legacy_init(void){return 1;}\nint legacy_cleanup(void){return 1;}\n')
 if command=='trim':(app/'src/api.src').write_text(bridge((app/'src/api.src').read_text()));(app/'src/callback_gate.inc').write_text(gate)
 data=mos.read_bytes();descriptor=dict(command=command,case=case,name=name,transport=transport,mos_bytes=len(data),mos_crc=zlib.crc32(data),MOS_sha256=h(mos),map_sha256=h(mosmap),firmware='mos-'+command+'.bin')
 files={str(f.relative_to(app)):h(f) for f in sorted(app.rglob('*')) if f.is_file()};build=hashlib.sha256(json.dumps(dict(source=files,config=descriptor),sort_keys=True).encode()).hexdigest()[:16]
 (app/'src/zz_config.c').write_text('#include "config.h"\nconst struct FixtureConfig fixture={'+','.join(json.dumps(v) for v in (case,command,name,build))+','+str(transport)+','+str(len(data))+','+hex(zlib.crc32(data))+'UL};\n')
 make(app,out/(command+'-perf.log'));shutil.copyfile(mos,app.parent/'mos.bin');shutil.copyfile(mosmap,app.parent/'mos.map')
 descriptor.update(build=build,source=files,config_sha256=h(app/'src/zz_config.c'),app=command+'.bin',app_sha256=h(app/'bin'/(command+'.bin')),fixture=str(app.parent),work_steps=1,report_version=2);(app.parent/'source.json').write_text(json.dumps(descriptor,indent=2)+'\n');cases.append(descriptor)
objects={command:{n:h(out/'perf'/command/'fixture/obj'/n) for n in ('main.o','platform.o')} for command in ('stock','rc1','trim')};assert len({tuple(sorted(v.items())) for v in objects.values()})==1
info.update(functional=functional_info,example=example_info,common_measurement_objects=objects,comparison_note='All three new apps share identical C measurement objects; callbacks have external linkage only. Trial includes its required app wrapper cost.')
(out/'build-info.json').write_text(json.dumps(info,indent=2)+'\n');(out/'perf/cases.json').write_text(json.dumps(dict(schema='PERF2-trim-1',reference='official-stock',cases=cases),indent=2)+'\n');print(json.dumps(dict(steps=rows,functional=functional_info['build'],common_core_identical=True,source=commit)))
