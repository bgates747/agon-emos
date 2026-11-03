"""Freeze performance sources and the already-qualified firmware identities."""
from pathlib import Path
import hashlib,importlib.util,json,shutil,sys,zlib
root=Path(__file__).resolve().parents[2];out=(root/sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=False)
f=out/'fixture';shutil.copytree(root/'experiments/uart0-performance/src',f/'src');shutil.copyfile(root/'experiments/uart0-performance/Makefile',f/'Makefile')
spec=importlib.util.spec_from_file_location('packets',root/'experiments/uart0-minimal/packets.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);module.generate(f)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
mos=[]
expected=['2110243ec967b16426933f8ffbd411649e51eb47dea780a49f1644e5421e2a64','383167c7ae6220472378b366c63c81b9df5e3e63fe5d827233d4c2276c287965','7db662439e33badd561cefe26e7a7a643d9f932da1031bce20d60217cd1e03af']
for name,pin in zip(('baseline','scalar003','otirx003'),expected):
 p=root/'build/tm001'/name/'bin/mos.bin';data=p.read_bytes();assert h(p)==pin
 mos.append(dict(name=name,size=len(data),crc32=zlib.crc32(data),sha256=pin))
files={str(p.relative_to(f)):h(p) for p in sorted(f.rglob('*')) if p.is_file()}
build=hashlib.sha256(json.dumps(dict(source=files,mos=mos),sort_keys=True).encode()).hexdigest()[:16]
(f/'src/identity.h').write_text('#define TM_BUILD "'+build+'"\nstatic const unsigned mos_sizes[]={'+','.join(str(m['size']) for m in mos)+'};\nstatic const uint32_t mos_crcs[]={'+','.join(hex(m['crc32'])+'UL' for m in mos)+'};\n')
(out/'source.json').write_text(json.dumps(dict(build=build,source=files,mos=mos),indent=2)+'\n');print(build)
