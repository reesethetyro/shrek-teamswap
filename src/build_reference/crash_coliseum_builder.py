# Reference builder; historical inputs are not bundled. Use root BUILDING.md for tested commands.
from pathlib import Path
import importlib.util,struct,json,gzip,io,re,subprocess,shutil
R=Path('work/crash-coliseum');L=Path('inputs/history/2026-11-23/this-x20/work');spec=importlib.util.spec_from_file_location('h',L/'team-swap-update-2026-11-24/package_current.py');H=importlib.util.module_from_spec(spec);spec.loader.exec_module(H)
pkg=R/'package/Team Swap';groups=H.parse_payload((pkg/'TeamSwap.patch').read_bytes());reports=json.loads((R/'variants.json').read_text());encoded=[]
for report,(name,rows)in zip(reports,groups):
 d=R/str(report['index']);a=(d/'clean.exe').read_bytes();old=(d/'before.exe').read_bytes();b=bytearray(old);at=report['mask_offsets'][0];assert len(report['mask_offsets'])==1 and b[at-9:at-6]==bytes.fromhex('83f80f')
 b[at-7]=24;struct.pack_into('<I',b,at+2,0x100e8ee);b=bytes(b);assert len([i for i,(x,y)in enumerate(zip(old,b))if x!=y])==2
 (d/'after.exe').write_bytes(b);report.update(new_sha=H.digest(b),chapter_cap_offset=at-7,new_mask='0x100e8ee',changed_bytes=2)
 encoded.append((name,[H.entry(row['name'],a,b)if row['name']=='Shrek.exe'else row['raw']for row in rows]))
raw=[b'TSW2',struct.pack('<i',len(encoded))]
for name,rows in encoded:
 name=name.encode('ascii');raw.extend([struct.pack('<i',len(name)),name,struct.pack('<i',len(rows)),*rows])
out=io.BytesIO()
with gzip.GzipFile(fileobj=out,mode='wb',compresslevel=9,mtime=0)as z:z.write(b''.join(raw))
payload=out.getvalue();(pkg/'TeamSwap.patch').write_bytes(payload)
s=(pkg/'Source/Installer.cs').read_text(encoding='utf-8-sig');s=re.sub(r'private const string PatchHash = "[0-9a-f]{64}";', 'private const string PatchHash = "'+H.digest(payload)+'";',s);(pkg/'Source/Installer.cs').write_text(s,encoding='utf-8-sig')
for report,folder in zip(reports,['Steam','Epic','Steam26880364','Epic593174']):
 dst=pkg/'Source'/folder;shutil.copy2(R/'0/after.exe',R/'0/after.exe.tmp')if False else None
 (dst/'crash-coliseum-update.json').write_text(json.dumps(report,indent=2))
 # Keep original feature manifests as archived; append explicit post-build patcher.
 (dst/'enable_crash_coliseum.py').write_text("from pathlib import Path\nfrom hashlib import sha256\np=Path(__import__('sys').argv[1]);b=bytearray(p.read_bytes());assert sha256(b).hexdigest()=='"+report['mod_sha']+"'\nb["+str(report['chapter_cap_offset'])+"]=24\nb["+str(report['mask_offsets'][0]+2)+":"+str(report['mask_offsets'][0]+6)+"]=(0x100e8ee).to_bytes(4,'little')\nassert sha256(b).hexdigest()=='"+report['new_sha']+"'\np.write_bytes(b)\n")
readme=(pkg/'README.txt').read_text();readme=readme.replace('NEW IN THIS UPDATE','NEW IN THIS UPDATE\n- Team Swap selections now apply in Crash Coliseum (Bonus menu, level 24). Other bonus-mode and scripted-sequence restrictions are unchanged. In-game Crash Coliseum validation is pending.');(pkg/'README.txt').write_text(readme)
(R/'variants.json').write_text(json.dumps(reports,indent=2))
compiler=['C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe','/nologo','/codepage:65001','/platform:x64','/optimize+','/r:System.Web.Extensions.dll'];subprocess.check_call(compiler+['/target:winexe','/out:'+str(pkg/'Team Swap Installer.exe'),str(pkg/'Source/Installer.cs')])
for tool in ['InstallerCommand','InstallerTests']:
 source=L/'team-swap-latest-2026-11-24'/(tool+'.cs');shutil.copy2(source,R/source.name);subprocess.check_call(compiler+['/target:exe','/main:'+tool,'/out:'+str(R/(tool+'.exe')),str(pkg/'Source/Installer.cs'),str(R/source.name)])
print('Updated four supported platform payloads; two guarded bytes per executable; installer rebuilt.')
