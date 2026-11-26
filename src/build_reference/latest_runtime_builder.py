# Reference builder; historical inputs are not bundled. Use root BUILDING.md for tested commands.
from pathlib import Path
from struct import unpack_from as u,pack_into as w,pack
import json,re,subprocess,hashlib
R=Path('work/quick-shrek-carry');cfg=json.loads(Path('work/crash-ammo/symbol-config.json').read_text());old=json.loads(Path('work/crash-ammo/patches.json').read_text());crash=json.loads(Path('work/crash-coliseum/variants.json').read_text());clang='inputs/toolchain/llvm/clang.exe';results=[]
for i in range(4):
 d=R/str(i);d.mkdir(exist_ok=True);b=bytearray(Path('work/crash-ammo/{}/after.exe'.format(i)).read_bytes());before=bytes(b);pe=u('<I',b,60)[0];opt=pe+24;st=opt+u('<H',b,pe+20)[0];ss=[u('<4I',b,st+j*40+8)for j in range(u('<H',b,pe+6)[0])]
 def off(rv):
  for sz,v,cap,raw in ss:
   if v<=rv<v+cap:return raw+rv-v
  raise ValueError(hex(rv))
 def read(rv,n):return bytes(b[off(rv):off(rv)+n])
 sy={line.split(' ',1)[1].strip():int(line.split(' ',1)[0],16)-0x140000000 for line in Path(cfg['paths'][i]).read_text().splitlines()if line.startswith('14')}
 def addr(prefix):return next(v for k,v in sy.items()if k.startswith(prefix))
 vals={k:addr(v)for k,v in dict(WORLD='?world@@',GC='?gGC@@',ONLINE='?hasOnlineLobby@GameController@@',GET_PLAYER='?GetPlayer@GameController@@QEAAPEAVPlayer@@H@Z',INPUT='?gLuxInputManager@InputManager@Lux@@',IS_PAUSED='?IsPaused@@',READ_BOOL='?ReadBoolOption@GameOptionFile@@',WRITE_BOOL='?WriteBoolOption@GameOptionFile@@',SAVE_OPTIONS='?SaveOptions@GameOption@@',OPTIONS='?gGameOptions@@',HASH='?crc32lcase@@',BREAK_LOCK='?BreakLock@Player@@',REMOVE_OBJECT='?Delete@Object@@',DAMAGE_MULT='?damageMultiplier@Player@@',DEFENSE_MH='?DefensePowerup_mh@@',POTION_FX='?AttackPowerupFX_mh@@',FIND_CHILD='?FindObject@@',DETACH='?Detach@Object@@',RELEASE='?Release@Object@@',HUD_CALLBACK='?HudCallBack@Player@@').items()}
 manifest=json.loads((R/'package/Team Swap/Source'/['Steam','Epic','Steam26880364','Epic593174'][i]/'manifest.json').read_text());origin=manifest['code'];ex=manifest['exports'];state=manifest['state']
 for key,name in dict(OLD_GET='settings_get_hook',OLD_SET='settings_set_hook',OLD_INIT='settings_init',OLD_LOAD='settings_load',OLD_SAVE='settings_save',OLD_STRING='fairy_table_string',OLD_HUD_RENDER='fairy_hud_render').items():vals[key]=origin+ex[name]
 rawmask=crash[i]['mask_offsets'][0]
 for sz,v,cap,raw in ss:
  if raw<=rawmask<raw+cap:mask=v+rawmask-raw
 part=read(mask-250,250);ap=part.rfind(bytes.fromhex('4881ec98000000'));assert ap>=0;apply=mask-250+ap;vals['APPLY']=apply;assert read(apply+14,3)==bytes.fromhex('488d15');vals['DRAFT']=apply+21+u('<i',read(apply+17,4))[0]
 world_hook=addr('?UpdateAll@ShrekWorld@@');assert read(world_hook,7)==bytes.fromhex('488bc448895810')
 # Locate every native read of the shared attack-potion multiplier in its owner method.
 contexts=[('?Attack@Character@@','rdi','eax'),('?Collision@Projectile@@','rsi','eax'),('?AttackCollision@DragonPlayer@@','rbp','eax'),('?mh@CandyCane@@','[rbx+0x1e8]','eax'),('?onPacket@PBBoss@@','rax','ebx'),('?sendRhythmStartSectionRequest@PBBoss@@','ebx','edi')];damage_sites=[]
 for j,(method,actor,dest)in enumerate(contexts):
  start=addr(method);body=read(start,0x1000);hits=[start+k for k in range(len(body)-7)if body[k:k+3]in [bytes.fromhex('0fb605'),bytes.fromhex('0fb61d'),bytes.fromhex('0fb63d')]and start+k+7+u('<i',body,k+3)[0]==vals['DAMAGE_MULT']];assert hits;damage_sites.append((hits[0],actor,dest))
 attack_pickup=addr('?PickUp@AttackPowerup@@');assert read(attack_pickup,5)==bytes.fromhex('48895c2410')
 table,size=u('<II',b,opt+136);code=(table+size+31)&~15;state=old[i]['state'];defs=['-D{}={}'.format(k,v)for k,v in vals.items()]
 subprocess.check_call([clang,'-target','x86_64-pc-windows-msvc','-x','c++','-std=c++17','-O2','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-builtin','-mno-stack-arg-probe','-funwind-tables','-S','-masm=intel']+defs+[str(R/'quick.cpp'),'-o',str(d/'raw.s')]);s=(d/'raw.s').read_text();s=re.sub(r'^\s*\.section\s+[^\n]+','.text',s,flags=re.M);(d/'compiled.s').write_text(s)
 prefix=f''' .intel_syntax noprefix
.text
.globl quick_start
quick_start:
.globl quick_base
quick_base:
.byte 0x48,0x8d,0x05
.long -(CODE_RVA+(. - quick_start)+4)
ret
.globl quick_state
quick_state:
.byte 0x48,0x8d,0x05
.long STATE_RVA-(CODE_RVA+(. - quick_start)+4)
ret
.globl quick_world_hook
.seh_proc quick_world_hook
quick_world_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
mov [rsp+0x28],rdx
call quick_tick
mov rcx,[rsp+0x20]
mov rdx,[rsp+0x28]
add rsp,0x38
jmp quick_world_original
.seh_endproc
.globl quick_world_original
.seh_proc quick_world_original
quick_world_original:
mov rax,rsp
mov [rax+0x10],rbx
.seh_savereg rbx,0x10
.seh_endprologue
.byte 0xe9
.long {world_hook+7}-(CODE_RVA+(. - quick_start)+4)
.seh_endproc
''' 
 prefix+=f'''
.globl attack_pickup_original
.seh_proc attack_pickup_original
attack_pickup_original:
mov [rsp+0x10],rbx
.seh_savereg rbx,0x10
.seh_endprologue
.byte 0xe9
.long {attack_pickup+5}-(CODE_RVA+(. - quick_start)+4)
.seh_endproc
'''
 for j,(site,actor,dest)in enumerate(damage_sites):
  label='potion_damage_'+str(j);setup=('mov ecx,ebx'if j==5 else 'mov rcx,'+actor);callee='quick_damage_slot'if j==5 else 'quick_damage'
  prefix+=f'''
.p2align 4
.globl {label}
.seh_proc {label}
{label}:
pushfq
.seh_stackalloc 8
sub rsp,0xd0
.seh_stackalloc 0xd0
.seh_endprologue
'''
  for reg,offset in [('rcx',0x20),('rdx',0x28),('r8',0x30),('r9',0x38),('r10',0x40),('r11',0x48),('rax',0x50)]:prefix+='mov [rsp+{}],{}\n'.format(offset,reg)
  for reg in range(6):prefix+='movups [rsp+{}],xmm{}\n'.format(0x60+16*reg,reg)
  prefix+=setup+'\ncall '+callee+'\nmov [rsp+0xc0],eax\n'
  for reg,offset in [('rcx',0x20),('rdx',0x28),('r8',0x30),('r9',0x38),('r10',0x40),('r11',0x48),('rax',0x50)]:prefix+='mov {},[rsp+{}]\n'.format(reg,offset)
  for reg in range(6):prefix+='movups xmm{},[rsp+{}]\n'.format(reg,0x60+16*reg)
  prefix+='mov {},[rsp+0xc0]\nadd rsp,0xd0\npopfq\nret\n.seh_endproc\n'.format(dest)
 (d/'all.s').write_text(prefix+s)
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
 patches=[]
 def patch(rv,data):
  previous=read(rv,len(data));b[off(rv):off(rv)+len(data)]=data;patches.append(dict(rva=rv,before=previous.hex(),after=data.hex()))
 def jump(rv,label,n=5):patch(rv,b'\xe9'+pack('<i',code+exports[label]-rv-5)+b'\x90'*(n-5))
 jump(world_hook,'quick_world_hook',7)
 for prefix,label,oldkey in [('?GetBoolOption@GameState@@','quick_get','OLD_GET'),('?SetBoolOption@GameState@@','quick_set','OLD_SET')]:
  site=addr(prefix);assert read(site,1)==b'\xe9' and site+5+u('<i',read(site+1,4))[0]==vals[oldkey];jump(site,label)
 for oldname,hookname,label in [('OLD_INIT','settings_ctor_hook','quick_init'),('OLD_LOAD','settings_load_hook','quick_load'),('OLD_SAVE','settings_save_hook','quick_save')]:
  h=origin+ex[hookname];body=read(h,60);sites=[h+j for j in range(len(body)-5)if body[j]==0xe8 and h+j+5+u('<i',body,j+1)[0]==vals[oldname]];assert len(sites)==1;site=sites[0];patch(site,b'\xe8'+pack('<i',code+exports[label]-site-5))
 # Preserve the existing dynamic Fairy description handler behind the new label.
 for patchrow in manifest['patches']:
  site=patchrow['rva'];data=read(site,5)
  if data[0]==0xe9 and site+5+u('<i',data,1)[0]==vals['OLD_STRING']:jump(site,'quick_string');break
 else:raise AssertionError('table string hook not found')
 for patchrow in manifest['patches']:
  site=patchrow['rva'];data=read(site,5)
  if data[0]==0xe8 and site+5+u('<i',data,1)[0]==vals['OLD_HUD_RENDER']:
   patch(site,b'\xe8'+pack('<i',code+exports['pickup_hud_render']-site-5));break
 else:raise AssertionError('HUD render callback call not found')
 jump(attack_pickup,'quick_attack_pickup')
 for j,(site,actor,dest)in enumerate(damage_sites):patch(site,b'\xe8'+pack('<i',code+exports['potion_damage_'+str(j)]-site-5)+b'\x90\x90')
 # Only the committed outgoing-player retirement call, not staging-failure cleanup.
 body=read(apply,0x640);sites=[apply+j for j in range(len(body)-5)if body[j]==0xe8 and apply+j+5+u('<i',body,j+1)[0]==vals['REMOVE_OBJECT']];assert len(sites)>=1
 site=sites[0];assert read(site-3,3)==bytes.fromhex('4889d1');patch(site,b'\xe8'+pack('<i',code+exports['swap_retire']-site-5))
 (d/'after.exe').write_bytes(b);results.append(dict(index=i,code=code,length=len(blob),state=state,hook=world_hook,patches=patches,exports=exports,addresses=vals,damage_sites=damage_sites,attack_pickup=attack_pickup,new_records=[list(u('<III',pd,j))for j in range(0,len(pd),12)],before_sha=hashlib.sha256(before).hexdigest(),after_sha=hashlib.sha256(b).hexdigest()))
(R/'patches.json').write_text(json.dumps(results,indent=2));print('Built live Quick Swap plus saved setting for all four builds; existing feature bytes and unwind data preserved.')
