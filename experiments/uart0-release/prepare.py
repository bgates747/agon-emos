"""Build frozen RC1 firmware twice and its matching original applications on Oryx."""
from pathlib import Path
import hashlib,importlib.util,io,json,os,shutil,subprocess,sys,tarfile,zlib
root=Path(__file__).resolve().parents[2];out=root/sys.argv[1];tool=Path(os.environ['AGONDEV_TOOLCHAIN']).resolve()
assert not subprocess.check_output(['git','status','--porcelain'],cwd=root).strip(),'Commit source before freezing'
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip();out.mkdir(parents=True,exist_ok=False)
archive=subprocess.check_output(['git','archive',commit],cwd=root)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def extract(dst):
 dst.mkdir()
 with tarfile.open(fileobj=io.BytesIO(archive)) as t:t.extractall(dst,filter='data')
def make(dst,log):
 with log.open('w') as f:subprocess.run(['make','-j4','AGONDEV_TOOLCHAIN='+str(tool)],cwd=dst,stdout=f,stderr=subprocess.STDOUT,check=True)
firmware=out/'firmware';repeat=out/'firmware-repeat';extract(firmware);extract(repeat)
make(firmware,out/'firmware-build.log');make(repeat,out/'repeat-build.log');assert h(firmware/'bin/mos.bin')==h(repeat/'bin/mos.bin')
prior=root/'build/tm001/otirx003';sources={str(f.relative_to(firmware/'src')):h(f) for f in sorted((firmware/'src').rglob('*')) if f.is_file()}
changed=[name for name in sources if not (prior/'src'/name).exists() or h(prior/'src'/name)!=sources[name]];assert changed==['version.h'],changed
objects={str(f.relative_to(firmware)):h(f) for f in sorted((firmware/'obj').glob('*.o'))}
changed_objects=[name for name in objects if h(prior/name)!=objects[name]];assert changed_objects==['obj/main.o'],changed_objects
assert h(firmware/'obj/uart0_async.o')==h(prior/'obj/uart0_async.o')
rom=(firmware/'bin/mos.bin').read_bytes();crc=zlib.crc32(rom)
assert b'Eirene UART0\x00' in rom and b'RC \x00' in rom
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');packets=importlib.util.module_from_spec(spec);spec.loader.exec_module(packets)
app_sources={}
for name,src in (('fixture',root/'experiments/uart0-release'),('example',root/'examples/uart0-async')):
 app=out/name;shutil.copytree(src/'src',app/'src');shutil.copyfile(src/'Makefile',app/'Makefile')
 if name=='fixture':packets.generate(app)
 else:shutil.copyfile(root/'experiments/uart0-release/src/rom_crc.h',app/'src/rom_crc.h')
 pins={str(f.relative_to(app)):h(f) for f in sorted(app.rglob('*')) if f.is_file()};build=hashlib.sha256(json.dumps(dict(source=pins,MOS=h(firmware/'bin/mos.bin')),sort_keys=True).encode()).hexdigest()[:16]
 (app/'src/identity.h').write_text('#define TM_BUILD "'+build+'"\n#define RC_MOS_BYTES '+str(len(rom))+'\n#define RC_MOS_CRC '+hex(crc)+'UL\n')
 make(app,out/(name+'-build.log'));app_sources[name]=dict(build=build,files=pins,identity_sha256=h(app/'src/identity.h'))
tool_names=('ez80-none-elf-clang','ez80-none-elf-as','ez80-none-elf-ld','ez80-none-elf-objdump')
info=dict(source_commit=commit,upstream_commit='86b694260e3be3b391284ce4aa8ecc758f7f34e9',tag='3.1.0_uart0_rc_1',title='Eirene MOS 3.1.0 UART0 RC1',banner='Eirene UART0 MOS Version 3.1.0 RC 1',MOS_bytes=len(rom),MOS_crc32=f'{crc:08X}',MOS_sha256=h(firmware/'bin/mos.bin'),map_sha256=h(firmware/'bin/mos.map'),test_sha256=h(out/'fixture/bin/test.bin'),example_sha256=h(out/'example/bin/example.bin'),checks_sha256=h(out/'fixture/checks.bin'),firmware_rebuild_identical=True,firmware_source_changes_from_qualified_OTIRX=changed,changed_objects=changed_objects,sender_object_identical=True,firmware_source_hashes=sources,toolchain=dict(path=str(tool),tools={n:h(tool/'bin'/n) for n in tool_names},libagon_sha256=h(tool/'lib/libagon.a'),compiler_version=subprocess.check_output([str(tool/'bin/ez80-none-elf-clang'),'--version'],text=True)),applications=app_sources)
(out/'build-info.json').write_text(json.dumps(info,indent=2)+'\n');print(json.dumps({k:info[k] for k in ('source_commit','MOS_bytes','MOS_crc32','MOS_sha256','firmware_rebuild_identical','firmware_source_changes_from_qualified_OTIRX','changed_objects','sender_object_identical')}))
