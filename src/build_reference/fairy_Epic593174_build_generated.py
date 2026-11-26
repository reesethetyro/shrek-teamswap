# Reference builder; historical inputs are not bundled. Use root BUILDING.md for tested commands.
from pathlib import Path
from struct import unpack_from as u,pack_into as w,pack
from hashlib import sha256
import subprocess,re,json
r=Path(__file__).resolve().parent
b=bytearray((r/'Shrek.before-fairy.exe').read_bytes());baseline=sha256(b).hexdigest();assert baseline=='47e15ca767690aca7ad466cff47dc85d1533c58f7517d78395feeab199e8b874'
b=ensure_section_header_room(b)
pe=u('<I',b,60)[0];opt=pe+24;st=opt+u('<H',b,pe+20)[0];n=u('<H',b,pe+6)[0];ss=[u('<4I',b,st+40*i+8)for i in range(n)]
def align(v,a):return (v+a-1)&-a
def off(rv):
 for sz,v,cap,raw in ss:
  if v<=rv<v+cap:return raw+rv-v
 raise ValueError(hex(rv))
def read(rv,k):return image_read(old_public,rv,k)
code=align(max(v+max(sz,cap)for sz,v,cap,raw in ss),4096);state=code+0x100000
llvm=Path('inputs/toolchain/llvm');clang=str(llvm/'clang.exe')
subprocess.run([clang,'-target','x86_64-pc-windows-msvc','-x','c++','-std=c++17','-O2','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-builtin','-mno-stack-arg-probe','-funwind-tables','-S','-masm=intel',str(r/'fairy.cpp'),'-o',str(r/'fairy_raw.s')],check=True)
s=(r/'fairy_raw.s').read_text();s=re.sub(r'^\s*\.section\s+[^\n]+','.text',s,flags=re.M);(r/'fairy_compiled.s').write_text(s)
subprocess.run([clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp',f'-DCODE_RVA={code}',f'-DSTATE_RVA={state}',f'-DOLD_DRAW_RVA={mapped(0x259b80+u("<i",read(0x259b7c,4))[0])}',f'-DOLD_BACK_RVA={mapped(0x25b1c5+u("<i",read(0x25b1c1,4))[0])}',f'-DOLD_STYLE_RVA={mapped(0x259d72+u("<i",read(0x259d6e,4))[0])}','-c',str(r/'fairy.s'),'-o',str(r/'fairy.obj')],check=True)
obj=(r/'fairy.obj').read_bytes();ns=u('<H',obj,2)[0];symtab,num=u('<II',obj,8);strings=symtab+18*num;syms={};si=0
while si<num:
 a=symtab+si*18;name=obj[a:a+8]
 if name[:4]==bytes(4):q=strings+u('<I',name,4)[0];name=obj[q:obj.index(0,q)]
 else:name=name.split(b'\0')[0]
 value,sec=u('<Ih',obj,a+8);syms[si]=(name.decode(),value,sec);si+=1+obj[a+17]
coff=[]
for i in range(ns):
 a=20+40*i;size,raw,rel=u('<III',obj,a+16);nr=u('<H',obj,a+32)[0];coff.append(dict(name=obj[a:a+8].split(b'\0')[0].decode(),size=size,raw=raw,rel=rel,nrel=nr))
textsec=coff[0];assert textsec['name']=='.text';blob=bytearray(obj[textsec['raw']:textsec['raw']+textsec['size']]);exports={name:value for name,value,sec in syms.values()if sec==1}
for i in range(textsec['nrel']):
 pos,idx,ty=u('<IIH',obj,textsec['rel']+10*i);name,value,sec=syms[idx];assert ty==4 and sec==1,(name,sec,ty);w('<i',blob,pos,value+u('<i',blob,pos)[0]-pos-4)
assert len(blob)<0xc0000
oldtab,oldsize=u('<II',b,opt+112+24);records=[u('<III',b,off(oldtab)+i)for i in range(0,oldsize,12)]
xsec=next(i+1 for i,s in enumerate(coff)if s['name']=='.xdata');xs=coff[xsec-1];ps=next(s for s in coff if s['name']=='.pdata');assert xs['nrel']==0
xb=align(code+len(blob),4);blob+=bytes(xb-code-len(blob));blob+=obj[xs['raw']:xs['raw']+xs['size']];pd=bytearray(obj[ps['raw']:ps['raw']+ps['size']])
for i in range(ps['nrel']):
 pos,idx,ty=u('<IIH',obj,ps['rel']+10*i);name,value,sec=syms[idx];assert ty==3 and sec in(1,xsec);w('<I',pd,pos,u('<I',pd,pos)[0]+value+(code if sec==1 else xb))
newrecords=[u('<III',pd,i)for i in range(0,len(pd),12)];records+=newrecords
# Native bodies reached from entry trampolines retain their native unwind state.
for start,end,trampoline in [(0x1e0a86,0x1e0b64,'fairy_update_original'),(0x241a95,0x241c00,'fairy_clear_original')]:
 start=mapped(start);end=mapped(end)
 parent=next(v for v in newrecords if v[0]==code+exports[trampoline]);assert parent
 # Original native runtime entries already cover these continuation addresses.
 assert any(a<=start<z for a,z,_ in records)
records.sort();tb=align(code+len(blob),4);blob+=bytes(tb-code-len(blob));blob+=b''.join(pack('<III',*row)for row in records);assert code+len(blob)<state
blob+=bytes(state+0x4000-code-len(blob))
raw=align(len(b),512);b+=bytes(raw-len(b));capacity=align(len(blob),512);b+=blob+bytes(capacity-len(blob));assert b[st+n*40:st+n*40+40]==bytes(40)
b[st+n*40:st+n*40+40]=pack('<8s6I2HI',b'.fairy\0\0',len(blob),code,capacity,raw,0,0,0,0,0xe0000060)
w('<H',b,pe+6,n+1);w('<I',b,opt+56,align(code+len(blob),4096));w('<II',b,opt+112+24,tb,len(records)*12)
patches=[]
def patch(site,expected,data):
 assert read(site,len(expected))==expected,('Old source changed',hex(site),read(site,len(expected)).hex(),expected.hex())
 newsite=mapped(site);current_expected=relocate_expected(expected,site,newsite)
 current=image_read(bytes(b),newsite,len(current_expected))
 assert current==current_expected,('Current native hook mismatch',hex(site),hex(newsite),current_expected.hex(),current.hex())
 replacement=patch_data(data,site,newsite)
 b[off(newsite):off(newsite)+len(replacement)]=replacement
 patches.append(dict(rva=newsite,old_rva=site,before=current_expected.hex(),after=replacement.hex()))
patch(0x103c1e,bytes.fromhex('4c8d442450ba0b000000ffd0'),bytes.fromhex('4c8d442450')+b'\xe8'+pack('<i',code+exports['fairy_hit']-0x103c28)+b'\x90\x90')
patch(0x1e0a80,bytes.fromhex('40534883ec30'),b'\xe9'+pack('<i',code+exports['fairy_update_hook']-0x1e0a85)+b'\x90')
patch(0x241a90,bytes.fromhex('48895c2410'),b'\xe9'+pack('<i',code+exports['fairy_clear_hook']-0x241a95))
patch(0x1fcf00,bytes.fromhex('405553574154'),b'\xe9'+pack('<i',code+exports['fairy_bridge_update']-0x1fcf05)+b'\x90')
patch(0x1fc440,bytes.fromhex('48896c2418'),b'\xe9'+pack('<i',code+exports['fairy_bridge_trigger']-0x1fc445))
patch(0x1fc486,bytes.fromhex('0f840d010000'),bytes.fromhex('909090909090'))
patch(0x1df010,bytes.fromhex('40574883ec30'),b'\xe9'+pack('<i',code+exports['fairy_action_check']-0x1df015)+b'\x90')
patch(0x202d50,bytes.fromhex('40555356488d6c24d0'),b'\xe9'+pack('<i',code+exports['fairy_simple_message']-0x202d55)+b'\x90'*4)
patch(0x1fd067,bytes.fromhex('f680b0010000010f847e010000'),b'\xe9'+pack('<i',code+exports['fairy_bridge_ground']-0x1fd06c)+b'\x90'*8)
patch(0x202f67,bytes.fromhex('f680b0010000010f8480010000'),b'\xe9'+pack('<i',code+exports['fairy_simple_ground']-0x202f6c)+b'\x90'*8)
for site in [0x1fd088,0x202f88]:
 patch(site,b'\xe8'+pack('<i',0x94770-site-5),b'\xe8'+pack('<i',code+exports['fairy_lever_height']-site-5))
for site,expected,label in [(0x23b250,'488d0d49843800','settings_get_hook'),(0x23d850,'488d0d495e3800','settings_set_hook'),(0x24e8b0,'4533c0c701ffffffff','settings_ctor_hook'),(0x1bc660,'48895c2408','settings_load_hook'),(0x1bc7b0,'48895c2408','settings_save_hook')]:
 patch(site,bytes.fromhex(expected),b'\xe9'+pack('<i',code+exports[label]-site-5)+b'\x90'*(len(bytes.fromhex(expected))-5))
for site,label in [(0x259b7b,'settings_draw_hook'),(0x259d6d,'settings_style_hook')]:
 old=read(site,5);assert old[0]==0xe9;patch(site,old,b'\xe9'+pack('<i',code+exports[label]-site-5))
patch(0x127d80,bytes.fromhex('e85b270000'),b'\xe8'+pack('<i',code+exports['fairy_hud_render']-0x127d85))
patch(0x202050,bytes.fromhex('48895c2408'),b'\xe9'+pack('<i',code+exports['fairy_exit_message']-0x202055))
patch(0x25e6d0,bytes.fromhex('48895c2408'),b'\xe9'+pack('<i',code+exports['fairy_table_string']-0x25e6d5))
old=read(0x25b1c0,5);assert old[0]==0xe9;patch(0x25b1c0,old,b'\xe9'+pack('<i',code+exports['swap_back_hook']-0x25b1c5))
old=read(0x18d59430,7);assert old==bytes.fromhex('83bf6001000001');patch(0x18d59430,old,b'\xe9'+pack('<i',code+exports['swap_preview_hook']-0x18d59435)+b'\x90\x90')
for site in [0x12a6d9,0x12a84a,0x12a950]:
 old=read(site,5)
 assert old[0]==0xe8 and site+5+u('<i',old,1)[0]==0x2d2af0
 patch(site,old,b'\xe8'+pack('<i',code+exports['shield_set_texture']-site-5))
patch(0x127b80,bytes.fromhex('488bc448895808'),b'\xe9'+pack('<i',code+exports['portrait_draw']-0x127b85)+b'\x90\x90')
patch(0x1e90a0,bytes.fromhex('48895c2410'),b'\xe9'+pack('<i',code+exports['fairy_potion_pickup']-0x1e90a5))
out=r/'Shrek.Fairy-current.exe'  ;out.write_bytes(b)
m=dict(version='Team Swap Fairy update for Epic CL593174',steam_only=False,platform='Epic CL593174',base_sha256=baseline,sha256=sha256(b).hexdigest(),code=code,state=state,section_raw=raw,exports=exports,patches=patches,unwind_records=len(newrecords),native_duration_offset='0x5a0',preserves='Current Epic CL593174 Team Swap baseline; complete authoritative Fairy/settings features')
(r/'manifest.json').write_text(json.dumps(m,indent=2));print(json.dumps({k:v for k,v in m.items()if k not in ['exports','patches']},indent=2))




