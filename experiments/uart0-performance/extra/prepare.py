"""Two added cases using the frozen physical PERF1 workload; no MOS rebuild."""
from pathlib import Path
import hashlib,json,shutil,sys,zlib
root=Path(__file__).resolve().parents[3];out=root/sys.argv[1];out.mkdir(parents=True,exist_ok=False)
base=root/'build/tm001/perf003/fixture';extra=Path(__file__).resolve().parent
old=root.parent/'agon-mos/build/uart0-balls/run007/appliance'
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
rows=[]
for name,mode,fw,pin in (
 ('stock',0,'mos-stock.bin','d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae'),
 ('ours',2,'mos-candidate.bin','cb1d0f4e066395533a2f07c7454246fa3a1ed46134bf155b883bb8c6e8353128')):
 f=out/name/'fixture';shutil.copytree(base/'src',f/'src');shutil.copyfile(base/'Makefile',f/'Makefile');shutil.copyfile(base/'checks.bin',f/'checks.bin')
 (f/'Makefile').write_text((f/'Makefile').read_text().replace('NAME = test','NAME = '+name))
 main=(f/'src/main.c').read_text()
 main=main.replace('EIRENE UART PERFORMANCE COMPARISON','UART PERFORMANCE COMPARISON - '+name.upper())
 main=main.replace('#define FRAMES 32','extern int case_argument_ok(const char*);\n#define FRAMES 32')
 main=main.replace(" mode=(unsigned)(argv[1][0]-'0');", " if(!case_argument_ok(argv[1]))return 1;\n mode=(unsigned)(argv[1][0]-'0');")
 (f/'src/zz_argument.c').write_text("int case_argument_ok(const char*s){return s[0]=='"+str(mode)+"';}\n")
 main=main.replace('RUN . 0=pristine Eirene, 1=scalar control, 2=OTIRX candidate. No firmware is flashed.',('RUN . 0 = official stock MOS 3.0.2.' if name=='stock' else 'RUN . 2 = previous full OTIRX MOS; borrowed/direct callback.')+' No firmware is flashed.')
 if name=='ours':
  main=main.replace('#define FRAMES 32','extern int legacy_init(void),legacy_cleanup(void);\n#define FRAMES 32')
  main=main.replace('if(ok){clock=clocks_start();','if(ok&&!legacy_init()){ok=0;record("FAIL: required old UTX1 interface absent");}\n if(ok){clock=clocks_start();')
  main=main.replace('if(clock)clocks_stop();','if(!legacy_cleanup())for(;;){}\n if(clock)clocks_stop();')
  shutil.copyfile(extra/'legacy.c',f/'src/zz_legacy.c');shutil.copyfile(extra/'legacy.src',f/'src/zz_gate.src')
  asm=(f/'src/api.src').read_text();start=asm.index('_tm_write:');end=asm.index('.global _rom_byte0',start)
  asm=asm[:start]+'.extern _legacy_write,_legacy_finish\n_tm_write:\n jp _legacy_write\n_tm_finish:\n jp _legacy_finish\n'+asm[end:];(f/'src/api.src').write_text(asm)
 (f/'src/main.c').write_text(main)
 # Assert exact source identity for every timed/helper function and PRT code.
 original=(base/'src/main.c').read_text()
 for a,b in [('static int drain(void)','int main(')]:assert main[main.index(a):main.index(b)]==original[original.index(a):original.index(b)]
 assert (f/'src/platform.c').read_bytes()==(base/'src/platform.c').read_bytes()
 for n in ('platform.h','run_counter.inc','packets.h'):assert (f/'src'/n).read_bytes()==(base/'src'/n).read_bytes()
 assert (f/'src/api.src').read_text().split('.global _rom_byte0',1)[1]==(base/'src/api.src').read_text().split('.global _rom_byte0',1)[1]
 data=(old/fw).read_bytes();assert hashlib.sha256(data).hexdigest()==pin
 mos=[dict(name=name,size=len(data),crc32=zlib.crc32(data),sha256=pin) for _ in range(3)]
 files={str(p.relative_to(f)):h(p) for p in sorted(f.rglob('*')) if p.is_file() and p.name!='identity.h'}
 build=hashlib.sha256(json.dumps(dict(source=files,mos=mos),sort_keys=True).encode()).hexdigest()[:16]
 (f/'src/identity.h').write_text('#define TM_BUILD "'+build+'"\nstatic const unsigned mos_sizes[]={'+','.join(str(m['size']) for m in mos)+'};\nstatic const uint32_t mos_crcs[]={'+','.join(hex(m['crc32'])+'UL' for m in mos)+'};\n')
 manifest=dict(build=build,source=files,mos=mos,case=name,mode=mode,parent_build='1ad95f651c6e017d',measured_C_helpers_unchanged=True,PRT_code_unchanged=True)
 (out/name/'source.json').write_text(json.dumps(manifest,indent=2)+'\n');shutil.copyfile(old/fw,out/name/'mos.bin')
 rows.append(dict(case=name,mode=mode,build=build,fixture=str(f),MOS_sha256=pin))
(out/'cases.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows))
