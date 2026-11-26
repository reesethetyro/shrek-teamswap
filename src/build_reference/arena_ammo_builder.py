# Reference builder; historical inputs are not bundled. Use root BUILDING.md for tested commands.
from pathlib import Path
from struct import unpack_from as u,pack_into as w,pack
import json,re,subprocess,hashlib
R=Path('work/crash-ammo');cfg=json.loads((R/'symbol-config.json').read_text());old=json.loads(Path('work/crash-clock/patches.json').read_text());clang='inputs/toolchain/llvm/clang.exe';results=[]
for i in range(4):
 d=R/str(i);d.mkdir(exist_ok=True);b=bytearray(Path('work/crash-clock/{}/after.exe'.format(i)).read_bytes());before=bytes(b);pe=u('<I',b,60)[0];opt=pe+24;st=opt+u('<H',b,pe+20)[0];ss=[u('<4I',b,st+j*40+8)for j in range(u('<H',b,pe+6)[0])]
 def off(rv):
  for sz,v,cap,raw in ss:
   if v<=rv<v+cap:return raw+rv-v
  raise ValueError(hex(rv))
 def read(rv,n):return bytes(b[off(rv):off(rv)+n])
 sy={line.split(' ',1)[1].strip():int(line.split(' ',1)[0],16)-0x140000000 for line in Path(cfg['paths'][i]).read_text().splitlines()if line.startswith('14')}
 def addr(prefix):return next(v for k,v in sy.items()if k.startswith(prefix))
 vals={k:addr(v)for k,v in dict(LOAD='?Load@CharacterFile@@',CREATE='?CreateRootObject@ShrekWorld@@',WORLD='?world@@',GC='?gGC@@',GAMEPLAY='?gpGameplayMF@@',ONLINE='?hasOnlineLobby@GameController@@',ORIGINAL='?mh@FionaPickup@@',APPLE='?ApplePickup_mh@@',COOKIE='?CookiePickup_mh@@',MOVE='?MoveObject@QuadTree@@').items()};vals['TREE']=vals['WORLD']+0x61d60
 # Verify the spatial tree against native shadow update's RIP-relative reference.
 sh=addr('?UpdateGround@PickupShadow@@');body=read(sh,0x600);assert any(sh+j+7+u('<i',body,j+3)[0]==vals['TREE'] for j in range(len(body)-7)if body[j:j+3]==bytes.fromhex('488d0d'))
 table,size=u('<II',b,opt+136);code=(table+size+31)&~15;state=old[i]['reserved_state'];defs=['-D{}={}'.format(k,v)for k,v in vals.items()]
 subprocess.check_call([clang,'-target','x86_64-pc-windows-msvc','-x','c++','-std=c++17','-O2','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-builtin','-mno-stack-arg-probe','-funwind-tables','-S','-masm=intel']+defs+[str(R/'ammo.cpp'),'-o',str(d/'raw.s')]);s=(d/'raw.s').read_text();s=re.sub(r'^\s*\.section\s+[^\n]+','.text',s,flags=re.M);(d/'compiled.s').write_text(s)
 prefix='''.intel_syntax noprefix
.text
.globl ammo_start
ammo_start:
.globl ammo_base
ammo_base:
.byte 0x48,0x8d,0x05
.long -(CODE_RVA+(. - ammo_start)+4)
ret
.globl ammo_state
ammo_state:
.byte 0x48,0x8d,0x05
.long STATE_RVA+0x3c00-(CODE_RVA+(. - ammo_start)+4)
ret
''';(d/'all.s').write_text(prefix+s)
 subprocess.check_call([clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp','-DCODE_RVA='+str(code),'-DSTATE_RVA='+str(state),'-c',str(d/'all.s'),'-o',str(d/'ammo.obj')]);obj=(d/'ammo.obj').read_bytes();ns=u('<H',obj,2)[0];symtab,num=u('<II',obj,8);strings=symtab+18*num;syms={};si=0
 while si<num:
  a=symtab+si*18;name=obj[a:a+8]
  if name[:4]==bytes(4):q=strings+u('<I',name,4)[0];name=obj[q:obj.index(0,q)]
  else:name=name.split(b'\0')[0]
  value,sec=u('<Ih',obj,a+8);syms[si]=(name.decode(),value,sec);si+=1+obj[a+17]
 coff=[]
 for j in range(ns):
  a=20+40*j;size2,raw,rel=u('<III',obj,a+16);nr=u('<H',obj,a+32)[0];coff.append(dict(name=obj[a:a+8].split(b'\0')[0].decode(),size=size2,raw=raw,rel=rel,nrel=nr))
 ts=coff[0];blob=bytearray(obj[ts['raw']:ts['raw']+ts['size']]);exports={n:v for n,v,sec in syms.values()if sec==1}
 for j in range(ts['nrel']):
  pos,idx,ty=u('<IIH',obj,ts['rel']+10*j);name,value,sec=syms[idx];assert ty==4 and sec==1,(name,sec,ty);w('<i',blob,pos,value+u('<i',blob,pos)[0]-pos-4)
 xsec=next(j+1 for j,s in enumerate(coff)if s['name']=='.xdata');xs=coff[xsec-1];ps=next(s for s in coff if s['name']=='.pdata');assert xs['nrel']==0
 xb=(code+len(blob)+3)&~3;blob+=bytes(xb-code-len(blob));blob+=obj[xs['raw']:xs['raw']+xs['size']];pd=bytearray(obj[ps['raw']:ps['raw']+ps['size']])
 for j in range(ps['nrel']):
  pos,idx,ty=u('<IIH',obj,ps['rel']+10*j);name,value,sec=syms[idx];assert ty==3 and sec in(1,xsec);w('<I',pd,pos,u('<I',pd,pos)[0]+value+(code if sec==1 else xb))
 records=[u('<III',b,off(table)+j)for j in range(0,size,12)]+[u('<III',pd,j)for j in range(0,len(pd),12)];records.sort();tb=(code+len(blob)+3)&~3;blob+=bytes(tb-code-len(blob));blob+=b''.join(pack('<III',*v)for v in records);assert code+len(blob)<state;assert read(code,len(blob))==bytes(len(blob));b[off(code):off(code)+len(blob)]=blob;w('<II',b,opt+136,tb,len(records)*12)
 hook=addr('?FionaPickup_mh@@');assert read(hook,1)==b'\xe9';assert hook+5+u('<i',read(hook+1,4))[0]==vals['ORIGINAL'];b[off(hook):off(hook)+5]=b'\xe9'+pack('<i',code+exports['arena_clock']-hook-5)
 gate=old[i]['gate'];gatebody=read(gate,old[i]['gate_size']);pat=bytes.fromhex('4839c275');assert gatebody.count(pat)==1;at=gate+gatebody.index(pat)+3;assert read(at,1)==b'\x75';b[off(at):off(at)+2]=b'\x90\x90'
 (d/'after.exe').write_bytes(b);results.append(dict(index=i,code=code,length=len(blob),state=state,hook=hook,exports=exports,addresses=vals,gate_branch=at,new_records=[list(u('<III',pd,j))for j in range(0,len(pd),12)],before_sha=hashlib.sha256(before).hexdigest(),after_sha=hashlib.sha256(b).hexdigest()))
(R/'patches.json').write_text(json.dumps(results,indent=2));print('Built arena ammo spawner, verified tree addresses and unwind records for all four builds.')
