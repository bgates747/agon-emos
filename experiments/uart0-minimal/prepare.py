"""Freeze the first finite app from reviewed inputs; no emulator or flashing."""
from pathlib import Path
import hashlib,json,shutil,sys
root=Path(__file__).resolve().parents[2];out=(root/sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=False)
f=out/'fixture';shutil.copytree(root/'experiments/uart0-minimal/src',f/'src');shutil.copyfile(root/'experiments/uart0-minimal/Makefile',f/'Makefile')
from packets import generate
generate(f)
h=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
files={str(p.relative_to(f)):h(p) for p in sorted(f.rglob('*')) if p.is_file()}
build=hashlib.sha256(json.dumps(files,sort_keys=True).encode()).hexdigest()[:16];(f/'src/identity.h').write_text('#define TM_BUILD "'+build+'"\n')
(out/'source.json').write_text(json.dumps(dict(build=build,source=files),indent=2)+'\n');print(build)
