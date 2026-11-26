# Reference builder; historical inputs are not bundled. Use root BUILDING.md for tested commands.
from pathlib import Path
from struct import unpack_from as U,pack_into as W,pack as P
import json,subprocess,hashlib
R=Path('work/crash-clock');reports=[];clang='inputs/toolchain/llvm/clang.exe'
for i in range(4):
 d=R/str(i);b=bytearray((d/'before.exe').read_bytes());pe=U('<I',b,60)[0];opt=pe+24;st=opt+U('<H',b,pe+20)[0];sections=[U('<4I',b,st+40*j+8)for j in range(U('<H',b,pe+6)[0])]
 def off(rv):
  for sz,v,cap,raw in sections:
   if v<=rv<v+cap:return raw+rv-v
  raise ValueError(hex(rv))
 def rva(raw):
  for sz,v,cap,at in sections:
   if at<=raw<at+cap:return v+raw-at
  raise ValueError(hex(raw))
 pat=bytes.fromhex('baeee800000fa3c2');assert b.count(pat)==1;hp=b.index(pat)-19;helper=rva(hp);lookup=helper-0x85;site=helper-0x72
 assert b[off(site)]==0xe8 and site+5+U('<i',b,off(site)+1)[0]==helper
 gc=helper+11+U('<i',b,hp+7)[0];network=helper+34+U('<i',b,hp+30)[0];fiona=lookup+0x28+U('<i',b,off(lookup)+0x24)[0]
 oldtable,size=U('<II',b,opt+112+24);old=b[off(oldtable):off(oldtable)+size];start=(oldtable+size+31)&~15
 text=f'''.intel_syntax noprefix
.text
.globl clock_gate
clock_gate:
sub rsp,0x28
.byte 0x48,0x8d,0x0d
.long {gc}-(START+(. - clock_gate)+4)
cmp dword ptr [rcx+8],24
jne normal
mov rdx,[rsp+0x50]
.byte 0x48,0x8d,0x05
.long {fiona}-(START+(. - clock_gate)+4)
cmp rdx,rax
jne denied
.byte 0xe8
.long {network}-(START+(. - clock_gate)+4)
movzx eax,al
xor al,1
jmp done
normal:
.byte 0xe8
.long {helper}-(START+(. - clock_gate)+4)
jmp done
denied:
xor eax,eax
done:
add rsp,0x28
ret
'''
 (d/'clock_gate.s').write_text(text);subprocess.check_call([clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp','-DSTART='+str(start),'-c',str(d/'clock_gate.s'),'-o',str(d/'clock_gate.obj')]);obj=(d/'clock_gate.obj').read_bytes();n,raw=U('<II',obj,36);assert U('<H',obj,52)[0]==0;code=obj[raw:raw+n]
 unwind=(start+len(code)+3)&~3;newtable=unwind+8;records=[U('<III',old,j)for j in range(0,len(old),12)];assert max(v[1]for v in records)<start
 records.append((start,start+len(code),unwind));table=b''.join(P('<III',*v)for v in records)
 end=newtable+len(table);state=json.loads((Path('work/crash-coliseum/portrait-package/Team Swap/Source')/['Steam','Epic','Steam26880364','Epic593174'][i]/'manifest.json').read_text())['state'];assert end<state
 assert b[off(start):off(end)]==bytes(end-start)
 b[off(start):off(start)+len(code)]=code;b[off(unwind):off(unwind)+8]=bytes.fromhex('0104010004420000');b[off(newtable):off(newtable)+len(table)]=table
 W('<II',b,opt+112+24,newtable,len(table));b[off(site):off(site)+5]=b'\xe8'+P('<i',start-site-5)
 (d/'after.exe').write_bytes(b);reports.append(dict(index=i,before_sha=hashlib.sha256((d/'before.exe').read_bytes()).hexdigest(),after_sha=hashlib.sha256(b).hexdigest(),gate=start,gate_size=len(code),lookup=lookup,callsite=site,helper=helper,gc=gc,network=network,fiona=fiona,unwind=unwind,exception_table=newtable,exception_table_size=len(table),reserved_state=state))
(R/'patches.json').write_text(json.dumps(reports,indent=2));print('Built Fiona-only offline Crash Coliseum clock gate for all four variants; unwind data and bank boundaries verified.')
