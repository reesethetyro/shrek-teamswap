# Historical reference; local input paths normalized for review. See README.md.
from pathlib import Path
from struct import unpack_from,pack,pack_into
from bisect import bisect_right
from collections import Counter
import sys,json
root=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'vendor'))
from capstone import *
from capstone.x86 import *
md=Cs(CS_ARCH_X86,CS_MODE_64);md.detail=True;md.skipdata=True
BASE=0x140000000
old=Path('inputs/game/Shrek.exe.steam-583916.bak').read_bytes()
mod=Path('inputs/history/2026-11-15/lo/work/Shrek.swap-team-menu.exe').read_bytes()
new=(root/'Shrek.clean.exe').read_bytes()
assert __import__('hashlib').sha256(new).hexdigest()=='dfd2bef8fabea3c6cbd224ac8275773fa5b49a0c63ff5d9cd8a6932a5ef04673'
assert __import__('hashlib').sha256(old).hexdigest()=='5c2eda7616b845e4deafe1387fe703fc75012e1854e759840e707c1fa5c25a29'
def sections(x):
 pe=unpack_from('<I',x,60)[0];t=pe+24+unpack_from('<H',x,pe+20)[0]
 return [(x[t+i*40:t+i*40+8].rstrip(b'\0').decode(),*unpack_from('<4I',x,t+i*40+8),t+i*40) for i in range(unpack_from('<H',x,pe+6)[0])]
os=sections(old);ns=sections(new);ms=sections(mod)
def off(x,rva):
 for name,size,r,cap,raw,h in sections(x):
  if r<=rva<r+cap:return raw+rva-r
 raise ValueError(hex(rva))
def read(x,a,n):return x[off(x,a):off(x,a)+n]
def syms(p):return {s:int(a,16)-BASE for a,s in (l.split(' ',1) for l in p.read_text().splitlines())}
a=syms(Path('inputs/history/2026-11-10/i-x20/work/symbols.txt'));b=syms(root/'symbols.txt')
pairs=sorted((v,b[k],k) for k,v in a.items() if k in b)
keys=[x[0] for x in pairs];keyset=set(keys)
def mapped(v):
 if v>=ms[7][2] and v<ms[-1][2]+0x40000:return v+0x3000
 if v==0:return 0
 i=bisect_right(keys,v)-1
 if i<0:return v
 st,dest,name=pairs[i]
 return dest+v-st

def relocate(ins,at):
 data=bytearray(ins.bytes);oldat=ins.address-BASE
 for op in ins.operands:
  if op.type==X86_OP_MEM:
   if op.mem.base==X86_REG_RIP:
    target=oldat+ins.size+ins.disp
    pack_into('<i',data,ins.disp_offset,mapped(target)-at-ins.size)
   elif op.mem.disp>=0x44c000 and op.mem.disp<0x19000000 and ins.disp_size==4:
    value=op.mem.disp
    if 0x17014800<=value<=0x17014900:value+=0x100 # expanded roster offset from relocated gGC
    else:value=mapped(value)
    pack_into('<i',data,ins.disp_offset,value)
  elif op.type==X86_OP_IMM:
   if ins.group(CS_GRP_JUMP) or ins.group(CS_GRP_CALL):
    target=op.imm-BASE
    value=mapped(target)-at-ins.size
    if ins.imm_size==1:pack_into('<b',data,ins.imm_offset,value)
    else:pack_into('<i',data,ins.imm_offset,value)
   elif 0x17014800<=op.imm<=0x17014900 and ins.imm_size==4:
    pack_into('<i',data,ins.imm_offset,op.imm+0x100)
   elif BASE<=op.imm<BASE+0x19000000 and ins.imm_size==8:
    pack_into('<Q',data,ins.imm_offset,BASE+mapped(op.imm-BASE))
 return bytes(data)

# Compare whole original instructions touched by the mod, retaining new game changes.
mod_hash=__import__('hashlib').sha256(mod).hexdigest()
if not (root/'native-patch-audit.json').exists() or json.loads((root/'native-patch-audit.json').read_text()).get('mod_hash')!=mod_hash:
 patches=[];conflicts=[]
 text=os[0]
 for ins in md.disasm(old[text[4]:text[4]+text[1]],BASE+text[2]):
  if not ins.id:continue
  at=ins.address-BASE
  changed=read(mod,at,ins.size)!=ins.bytes
  if not changed:continue
  dest=mapped(at)
  expected=relocate(ins,dest)
  actual=read(new,dest,ins.size)
  patches.append((at,dest,ins.size))
  if expected!=actual:conflicts.append((hex(at),hex(dest),ins.mnemonic+' '+ins.op_str,expected.hex(),actual.hex()))
 (root/'native-patch-audit.json').write_text(json.dumps({'instructions':patches,'conflicts':conflicts,'mod_hash':mod_hash},indent=2))
 print('Native modified instructions',len(patches),'conflicts',len(conflicts));print(*conflicts[:30],sep='\n')
else:
 audit=json.loads((root/'native-patch-audit.json').read_text());patches=audit['instructions'];conflicts=audit['conflicts']

def directory(x,index):
 pe=unpack_from('<I',x,60)[0];return unpack_from('<II',x,pe+24+112+8*index)
def runtime(x):
 r,n=directory(x,3);p=off(x,r);return [unpack_from('<III',x,i) for i in range(p,p+n,12)]
print('Exception dirs',*[tuple(hex(n) for n in directory(x,3)) for x in (old,mod,new)])
print('Added runtime',len([e for e in runtime(mod) if e[0]>=ms[7][2]]))
print('Native data diffs',[(sec[0],sum(u!=v for u,v in zip(old[sec[4]:sec[4]+sec[3]],read(mod,sec[2],sec[3])))) for sec in os[1:]])

assert not conflicts
out=bytearray(new[:0x400]+bytes(0x200)+new[0x400:])
for sec in ns:
 pack_into('<I',out,sec[5]+20,sec[4]+0x200)
pe=unpack_from('<I',new,60)[0];opt=pe+24
pack_into('<I',out,opt+60,0x600)
dbg,dbg_size=directory(new,6)
if dbg:
 for p in range(off(out,dbg),off(out,dbg)+dbg_size,28):
  pointer=unpack_from('<I',out,p+24)[0]
  if pointer:pack_into('<I',out,p+24,pointer+0x200)
cert,cert_size=directory(new,4)
if cert:pack_into('<I',out,opt+112+32,cert+0x200)
# Copy the three mod-owned sections above the new image, keeping native bytes.
pe=unpack_from('<I',new,60)[0];opt=pe+24;headers=opt+unpack_from('<H',new,pe+20)[0]
for index,sec in enumerate(ms[7:],7):
 name,size,r,cap,raw,h=sec
 dest_raw=(len(out)+511)&~511
 out.extend(bytes(dest_raw-len(out)))
 out.extend(mod[raw:raw+cap])
 nh=headers+40*index
 out[nh:nh+40]=mod[h:h+40]
 pack_into('<I',out,nh+12,r+0x3000);pack_into('<I',out,nh+20,dest_raw)
pack_into('<H',out,pe+6,10)
def write(r,data):
 p=off(out,r);out[p:p+len(data)]=data
# Native code replacement ranges follow original instruction boundaries.
ranges=[]
for at,dest,n in patches:
 if ranges and ranges[-1][0]+ranges[-1][2]==at:
  x,y,z=ranges[-1];ranges[-1]=(x,y,z+n)
 else:ranges.append((at,dest,n))
roots=set();native_count=0
for at,dest,n in ranges:
 data=read(mod,at,n);done=0
 for ins in md.disasm(data,BASE+at):
  assert ins.id,('Undecoded patched native bytes',hex(at),data.hex())
  pos=ins.address-BASE
  replacement=relocate(ins,dest+pos-at)
  write(dest+pos-at,replacement);done+=ins.size;native_count+=1
  for op in ins.operands:
   if op.type==X86_OP_IMM and (ins.group(CS_GRP_JUMP) or ins.group(CS_GRP_CALL)):
    target=op.imm-BASE
    if target>=ms[7][2]:roots.add(target)
 assert done==n,(hex(at),done,n)
# Two native vtable bytes point into the mod's compatibility handler.
for sec in os[1:]:
 if sec[0]=='.pdata':continue
 for i in range(0,sec[3],8):
  at=sec[2]+i;before=read(old,at,8);after=read(mod,at,8)
  if before==after:continue
  assert len(before)==len(after)==8
  value=unpack_from('<Q',before)[0];changed=unpack_from('<Q',after)[0]
  if not (BASE<=value<BASE+0x19000000 and BASE<=changed<BASE+0x19000000):
   assert read(new,mapped(at),8)==before,(hex(at),before.hex(),read(new,mapped(at),8).hex())
   write(mapped(at),after);continue
  dest=mapped(at)
  assert read(new,dest,8)==pack('<Q',BASE+mapped(value-BASE))
  write(dest,pack('<Q',BASE+mapped(changed-BASE)))
  if changed-BASE>=ms[7][2]:roots.add(changed-BASE)
# Reach every hook and helper, including registered unwind ranges.
custom_runtime=[e for e in runtime(mod) if e[0]>=ms[7][2]]
for start,end,unwind in custom_runtime:roots.add(start)
code={};pending=list(roots)
while pending:
 at=pending.pop()
 if at in code or at<ms[7][2]:continue
 section=next((s for s in ms[7:] if s[2]<=at<s[2]+s[1]),None)
 assert section and section[0] in ('.fight','.party5'),('Non-code target',hex(at))
 for ins in md.disasm(read(mod,at,min(8192,section[2]+section[1]-at)),BASE+at):
  pos=ins.address-BASE
  if pos in code:break
  assert ins.id,('Undecoded added code',hex(pos))
  code[pos]=ins
  for op in ins.operands:
   if op.type==X86_OP_IMM and (ins.group(CS_GRP_JUMP) or ins.group(CS_GRP_CALL)):
    target=op.imm-BASE
    if target>=ms[7][2]:pending.append(target)
  if ins.group(CS_GRP_RET) or ins.mnemonic in ('jmp','int3','ud2'):break
for at,ins in code.items():write(mapped(at),relocate(ins,mapped(at)))
covered=set(p for at,ins in code.items() for p in range(at,at+ins.size))
# Data tables contain native RVA pairs and chained runtime records.
old_records=set(runtime(mod))
table_changes=0
for sec in ms[7:]:
 name,size,r,cap,raw,h=sec
 at=r
 while at+4<=r+cap:
  if at%4:at+=1;continue
  if any(p in covered for p in range(at,at+4)):
   at+=1;continue
  if at+12<=r+cap:
   entry=unpack_from('<III',mod,raw+at-r)
   if entry in old_records:
    write(mapped(at),pack('<III',*(mapped(v) for v in entry)));table_changes+=1;at+=12;continue
  value=unpack_from('<I',mod,raw+at-r)[0]
  # Symbol RVAs are used in character-file and handler tables.
  if value in keyset and value>=0x45000 and value<os[-1][2]+os[-1][1]:
   write(mapped(at),pack('<I',mapped(value)));table_changes+=1
  at+=4
# Assembly RVA tables need not start on a four-byte boundary. Follow actual
# RIP-relative data references instead of relying on file alignment.
unaligned_tables={}
for at,ins in code.items():
 for op in ins.operands:
  if op.type!=X86_OP_MEM or op.mem.base!=X86_REG_RIP:continue
  table=at+ins.size+ins.disp
  if table<ms[7][2] or table in covered:continue
  entries=[]
  for delta in range(0,256,4):
   pos=table+delta
   if any(p in covered for p in range(pos,pos+4)):break
   try:value=unpack_from('<I',read(mod,pos,4))[0]
   except ValueError:break
   if value==0:continue
   if value not in keyset or value<0x45000:break
   write(mapped(pos),pack('<I',mapped(value)))
   entries.append((hex(pos),hex(value),hex(mapped(value))))
  if entries:unaligned_tables[hex(table)]=entries
(root/'referenced-table-audit.json').write_text(json.dumps(unaligned_tables,indent=2))
# Check all character-file and handler entries, including the unavailable slots.
character_signature=pack('<4I',0x5e0680,0x23f520,0x5ac580,0xb2620)
character_raw=mod.index(character_signature)
character_section=next(sec for sec in ms if sec[4]<=character_raw<sec[4]+sec[3])
character_rva=character_section[2]+character_raw-character_section[4]
for i in range(20):
 value=unpack_from('<I',mod,character_raw+i*4)[0]
 assert read(out,mapped(character_rva)+i*4,4)==pack('<I',mapped(value) if value else 0)
print('Validated all 20 character-file and handler table entries, including unaligned data.')
# Preserve this update's unwind table; retain the mod's native unwind overrides.
base_records={e[0]:e for e in runtime(old)}
overrides={e[0]:e for e in runtime(mod) if e[0] in base_records and e!=base_records[e[0]]}
new_records=[]
for entry in runtime(new):
 old_start=next((s for s in overrides if mapped(s)==entry[0]),None)
 if old_start is not None:
  replacement=overrides[old_start]
  entry=tuple(mapped(v) for v in replacement)
 new_records.append(entry)
new_records.extend(tuple(mapped(v) for v in e) for e in custom_runtime)
new_records.sort()
assert all(x[1]<=y[0] for x,y in zip(new_records,new_records[1:]))
# Relocate chained unwind trailers and custom exception handlers.
for start,end,unwind in custom_runtime:
 flags=read(mod,unwind,4);n=flags[2];trailer=unwind+4+((n+1)&~1)*2
 if flags[0]>>3 & 4:
  chain=unpack_from('<III',read(mod,trailer,12));write(mapped(trailer),pack('<III',*(mapped(v) for v in chain)))
 elif flags[0]>>3 & 3:
  handler=unpack_from('<I',read(mod,trailer,4))[0];write(mapped(trailer),pack('<I',mapped(handler)))
# Append authoritative current-build unwind data after the copied payload.
last=sections(out)[-1]
helper_offset=(last[1]+15)&~15
helper_rva=last[2]+helper_offset
from subprocess import run
clang='C:/Program Files/Unity/Hub/Editor/6000.0.43f1/Editor/Data/PlaybackEngines/WebGLSupport/BuildTools/Emscripten/llvm/clang.exe'
run([clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp','-DHELPER_RVA='+str(helper_rva),'-c',str(root/'egg_counter_alignment.s'),'-o',str(root/'egg_counter_alignment.obj')],check=True)
obj=(root/'egg_counter_alignment.obj').read_bytes();length,raw_obj=unpack_from('<II',obj,36)
helper=obj[raw_obj:raw_obj+length]
assert unpack_from('<H',obj,52)[0]==0
unwind_offset=(helper_offset+len(helper)+3)&~3
parent=next(e for e in runtime(new) if e[0]<=0x12a5ca<e[1])
chain=bytes((0x21,0,0,read(new,parent[2]+3,1)[0]))+pack('<III',*parent)
new_records.append((helper_rva,helper_rva+len(helper),last[2]+unwind_offset));new_records.sort()
mongo_offset=(unwind_offset+len(chain)+15)&~15
mongo_rva=last[2]+mongo_offset
run([clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp','-DMONGO_RVA='+str(mongo_rva),'-c',str(root/'mongo_party_boarding.s'),'-o',str(root/'mongo_party_boarding.obj')],check=True)
mongo_obj=(root/'mongo_party_boarding.obj').read_bytes();mongo_size,mongo_raw=unpack_from('<II',mongo_obj,36)
assert unpack_from('<H',mongo_obj,52)[0]==0
mongo_code=mongo_obj[mongo_raw:mongo_raw+mongo_size-4]
mongo_slot=unpack_from('<I',mongo_obj,mongo_raw+mongo_size-4)[0]
mongo_unwind_offset=(mongo_offset+len(mongo_code)+3)&~3
mongo_parent=next(e for e in runtime(new) if e[0]<=0x1a6e53<e[1])
mongo_chain=bytes((0x21,0,0,read(new,mongo_parent[2]+3,1)[0]))+pack('<III',*mongo_parent)
new_records.append((mongo_rva,mongo_rva+len(mongo_code),last[2]+mongo_unwind_offset));new_records.sort()
write(0x1a6e53,b'\xe9'+pack('<i',mongo_rva-0x1a6e53-5))
write(0x1a6f40,b'\xe9'+pack('<i',mongo_rva+mongo_slot-0x1a6f40-5))
table_at=(mongo_unwind_offset+len(mongo_chain)+15)&~15
assert read(new,0x12a5ca,8)==bytes.fromhex('f64704104c8b4738')
write(0x12a5ca,b'\xe9'+pack('<i',helper_rva-0x12a5ca-5)+b'\x90'*3)
payload=b''.join(pack('<III',*e) for e in new_records)
need=table_at+len(payload);capacity=(need+511)&~511
out.extend(bytes(last[4]+capacity-len(out)))
out[last[4]+helper_offset:last[4]+helper_offset+len(helper)]=helper
out[last[4]+unwind_offset:last[4]+unwind_offset+len(chain)]=chain
out[last[4]+mongo_offset:last[4]+mongo_offset+len(mongo_code)]=mongo_code
out[last[4]+mongo_unwind_offset:last[4]+mongo_unwind_offset+len(mongo_chain)]=mongo_chain
out[last[4]+table_at:last[4]+table_at+len(payload)]=payload
pack_into('<I',out,last[5]+8,need);pack_into('<I',out,last[5]+16,capacity)
pack_into('<II',out,opt+112+24,last[2]+table_at,len(payload))
pack_into('<I',out,opt+56,(last[2]+need+4095)&~4095)
pack_into('<I',out,opt+64,0)
(root/'Shrek.team-swap-updated.exe').write_bytes(out)
print('Ported',len(ranges),'native edit ranges,',len(code),'added instructions,',table_changes,'table records;',len(overrides),'native unwind overrides')
from hashlib import sha256
print('New mod SHA256',sha256(out).hexdigest())

# Audit coverage and native continuation addresses independently of emission.
gaps=[]
for start,end,u in custom_runtime:
 missing=[p for p in range(start,end) if p not in covered and read(mod,p,1) not in (b'\x90',b'\xcc',b'\x00')]
 if missing and (start,end) not in ((0x18cbde80,0x18cbe104),(0x18cbec80,0x18cbee10)):gaps.append((hex(start),hex(end),len(missing),hex(missing[0])))
print('Uncovered custom runtime bytes',gaps)
continuations=[]
for at,ins in code.items():
 for op in ins.operands:
  if op.type!=X86_OP_IMM or not(ins.group(CS_GRP_JUMP) or ins.group(CS_GRP_CALL)):continue
  t=op.imm-BASE
  if not(0x1000<=t<os[0][2]+os[0][1]) or t in keyset:continue
  oi=next(md.disasm(read(old,t,15),BASE+t),None)
  if oi and oi.id and relocate(oi,mapped(t))!=read(new,mapped(t),oi.size):
   continuations.append((hex(at),hex(t),hex(mapped(t)),oi.mnemonic,oi.op_str))
print('Native continuation mismatches',continuations)
assert not gaps and not continuations



# Verify today's native .text is unchanged outside Team Swap's audited edits.
allowed=set(p for at,dest,n in ranges for p in range(dest,dest+n))|set(range(0x12a5ca,0x12a5d2))
sec=ns[0]
actual_changes=[sec[2]+i for i,(x,y) in enumerate(zip(new[sec[4]:sec[4]+sec[1]],read(out,sec[2],sec[1]))) if x!=y]
assert all(p in allowed for p in actual_changes)
assert read(out,0x135720,5)==read(new,0x135720,5)
assert read(out,0x22bf98,6)==read(new,0x22bf98,6)
print('Preserved all updated native text outside audited mod edits; temporary shortcut sites are native.')



