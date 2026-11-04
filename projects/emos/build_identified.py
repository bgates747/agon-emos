#!/usr/bin/env python3
"""Native EMOS identified build; never installs, flashes or selects production."""
from pathlib import Path
import argparse,datetime,hashlib,json,re,shutil,subprocess

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);p.add_argument('--toolchain',required=True,type=Path);p.add_argument('--python',required=True,type=Path);a=p.parse_args()
 root=Path(__file__).resolve().parents[2];commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
 if subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True):raise SystemExit('Refuse identified build from dirty native source')
 identity=json.loads((root/'port/eirene-identity.json').read_text());assert identity['status']=='experimental'
 stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%d-%H-%M-%SZ');build=identity['source_identity']+'-b'+stamp
 assert re.fullmatch(r'agon-emos-eirene-r01-b\d{4}(?:-\d{2}){5}Z',build)
 out=a.output.resolve();out.mkdir(exist_ok=False);header=out/'identity.h';header.write_text('/* Generated native EMOS identity; do not edit. */\n'+''.join('#define '+k+' '+json.dumps(v)+'\n' for k,v in [('EMOS_SOURCE_IDENTITY',identity['source_identity']),('EMOS_BUILD_ID',build),('EMOS_ARTIFACT_STATUS',identity['status'])]))
 cmd=['make','-j1','firmware-check','AGONDEV_TOOLCHAIN='+str(a.toolchain.resolve()),'PYTHON='+str(a.python.resolve()),'EMOS_IDENTITY_HEADER='+str(header)]
 (out/'command.json').write_text(json.dumps(cmd,indent=2)+'\n')
 start=datetime.datetime.now(datetime.timezone.utc)
 with (out/'build.log').open('x') as log:r=subprocess.run(cmd,cwd=root,stdout=log,stderr=subprocess.STDOUT,timeout=180)
 (out/'step.json').write_text(json.dumps({'start':start.isoformat(),'end':datetime.datetime.now(datetime.timezone.utc).isoformat(),'returncode':r.returncode},indent=2)+'\n')
 if r.returncode:raise SystemExit(r.returncode)
 if subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True):raise SystemExit('Source changed during identified build')
 for name in ['mos.bin','mos.elf','mos.map','mos.elf.map']:shutil.copyfile(root/'bin'/name,out/name)
 image=(out/'mos.bin').read_bytes();assert len(image)<=131072
 for value in [identity['source_identity'],build,identity['status']]:assert value.encode()+b'\0' in image,value
 (out/'ROM.bin').write_bytes(image+b'\xff'*(131072-len(image)))
 (out/(build+'.bin')).write_bytes(image)
 m={'build_id':build,'source_identity':identity['source_identity'],'status':identity['status'],'source_commit':commit,'rom_used':len(image),'rom_free':131072-len(image),'scope':'identified native wrapper/all4mandatorylinkedchecks; no installation','outputs':{f:{'bytes':(out/f).stat().st_size,'sha256':hashlib.sha256((out/f).read_bytes()).hexdigest()} for f in ['mos.bin','mos.elf','mos.map','mos.elf.map','ROM.bin']}}
 (out/'manifest.json').write_text(json.dumps(m,indent=2)+'\n');print(json.dumps(m,indent=2))
if __name__=='__main__':main()
