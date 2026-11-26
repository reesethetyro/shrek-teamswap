# Historical reference; local input paths normalized for review. See README.md.
"""Port public Team Swap from Steam 26746348 to Steam 26880364.

Native original instructions/data are validated before writes. Added code is
relocated by symbols and control-flow traversal. No installed file is written.
"""
from pathlib import Path
from struct import unpack_from as U,pack as P,pack_into as W
from bisect import bisect_right
from hashlib import sha256
import sys,json,re,collections
R=Path(__file__).resolve().parent;S=Path(r'inputs/history/2026-11-16/we-x20/work/steam-update-2026-11-18')
sys.path.insert(0,str(S/'vendor'))
from capstone import *
from capstone.x86 import *
BASE=0x140000000
old=(S/'Shrek.clean.exe').read_bytes();mod=(S/'Shrek.team-swap-updated.exe').read_bytes();new=(R/'Shrek.clean.exe').read_bytes()
assert sha256(old).hexdigest()=='dfd2bef8fabea3c6cbd224ac8275773fa5b49a0c63ff5d9cd8a6932a5ef04673'
assert sha256(mod).hexdigest()=='dca90e19044eb69163ab788d54ff922c6ecbf14b59067f0135dd7df8db1be68b'
assert sha256(new).hexdigest()=='33f6f42cfbc087c55e2d6a15ef1f32ea42282937340a1639188096ae4c8da0eb'
def align(v,a):return(v+a-1)&-a
def sections(x):
 pe=U('<I',x,60)[0];t=pe+24+U('<H',x,pe+20)[0]
 return [(x[t+i*40:t+i*40+8].rstrip(b'\0').decode(),*U('<4I',x,t+i*40+8),t+i*40)for i in range(U('<H',x,pe+6)[0])]
os=sections(old);ms=sections(mod);ns=sections(new);custom=ms[len(os):]
SHIFT=align(max(s[2]+max(s[1],s[3])for s in ns),0x1000)-custom[0][2]
assert SHIFT%0x1000==0
def off(x,rva):
 for name,size,r,cap,raw,h in sections(x):
  if r<=rva<r+cap:return raw+rva-r
 raise ValueError(hex(rva))
def read(x,rva,n):p=off(x,rva);assert p+n<=len(x);return bytes(x[p:p+n])
def directory(x,i):return U('<II',x,U('<I',x,60)[0]+24+112+8*i)
def runtime(x):r,n=directory(x,3);p=off(x,r);return[U('<III',x,a)for a in range(p,p+n,12)]
def txtsyms(p):return{n:int(a,16)-BASE for a,n in(l.split(' ',1)for l in p.read_text().splitlines())}
def mapsyms(p):
 out={}
 for n,a in re.findall(r'^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{16})\b',p.read_text(),re.M):
  a=int(a,16)
  if BASE<=a<BASE+0x20000000:out[n]=a-BASE
 return out
a=txtsyms(S/'symbols.txt');b=txtsyms(R/'symbols.txt')
# The shipped MAP adds local code/data labels, when it matches this exact build.
sm=Path('inputs/game/Shrek.map')
if sm.exists():
 st=sm.read_text();stamp=re.search(r'Timestamp is ([0-9a-fA-F]+)',st)
 if stamp and int(stamp.group(1),16)==U('<I',old,U('<I',old,60)[0]+8)[0]:a.update(mapsyms(sm))
# Current official PDB symbols are extracted into symbols.txt.
pairs=sorted((v,b[k],k)for k,v in a.items()if k in b);keys=[x[0]for x in pairs];keyset=set(keys)
oldGC=a['?gGC@@3VGameController@@A'];newGC=b['?gGC@@3VGameController@@A']
def iscustom(v):return any(s[2]<=v<s[2]+max(s[1],s[3])for s in custom)
native_address_overrides={0x22e510:0x231210,0x22e515:0x231217,0x1da6f5:0x1dcae4,0x1da794:0x1dcb74}

def mapped(v):
 if v==0:return 0
 if v in native_address_overrides:return native_address_overrides[v]
 if iscustom(v):return v+SHIFT
 exact=bisect_right(keys,v)-1
 if exact>=0 and keys[exact]==v:return pairs[exact][1]
 if 'aligned_keys' in globals():
  j=bisect_right(aligned_keys,v)-1
  if j>=0:
   st,dest,n=aligned_spans[j]
   if st<=v<st+n:return dest+v-st
 i=bisect_right(keys,v)-1
 if i<0:raise ValueError('No symbol preceding '+hex(v))
 start,dest,name=pairs[i];return dest+v-start
def gc_offset(v):return mapped(oldGC+v)-newGC if v>=0x10000000 and iscustom(oldGC+v)else None
md=Cs(CS_ARCH_X86,CS_MODE_64);md.detail=True;md.skipdata=True
def relocate(ins,at):
 data=bytearray(ins.bytes);origin=ins.address-BASE
 for op in ins.operands:
  if op.type==X86_OP_MEM:
   if op.mem.base==X86_REG_RIP:W('<i',data,ins.disp_offset,mapped(origin+ins.size+ins.disp)-at-ins.size)
   elif ins.disp_size==4 and gc_offset(op.mem.disp)is not None:W('<i',data,ins.disp_offset,gc_offset(op.mem.disp))
   elif os[1][2]<=op.mem.disp<custom[-1][2]+custom[-1][1]and ins.disp_size==4:
    W('<I',data,ins.disp_offset,mapped(op.mem.disp))
  elif op.type==X86_OP_IMM:
   if ins.group(CS_GRP_JUMP)or ins.group(CS_GRP_CALL):
    value=mapped(op.imm-BASE)-at-ins.size;W('<b'if ins.imm_size==1 else'<i',data,ins.imm_offset,value)
   elif gc_offset(op.imm)is not None and ins.imm_size==4:W('<I',data,ins.imm_offset,gc_offset(op.imm))
   elif BASE<=op.imm<BASE+custom[-1][2]+custom[-1][1]and ins.imm_size==8:W('<Q',data,ins.imm_offset,BASE+mapped(op.imm-BASE))
 return bytes(data)

from address_alignment import make_alignment
aligned_spans=make_alignment(old,mod,new,a,b,md,sections,read,runtime,directory)
aligned_keys=[s[0] for s in aligned_spans]

from respawn_adapter import plan_adapter,apply_adapter
respawn_plan,_respawn_changes=plan_adapter(old,mod,new,mapped,read)
from gc_iteration_adapter import plan_adapter as plan_gc_adapter,apply_adapter as apply_gc_adapter
gc_plan,_gc_changes=plan_gc_adapter(old,mod,new,mapped,read)
gc_owned_sites={e['old_site'] for e in gc_plan['sites']}
for e in gc_plan['sites']:
 native_address_overrides[e['old_site']]=e['new_site']
 if 'old_continuation' in e:native_address_overrides[e['old_continuation']]=e['new_continuation']
victory_owned_sites={0x2300de,0x2300f8,0x230116}
handled_custom_ranges=[(respawn_plan['old_helper'],36)]+[(e['old_helper'],e['helper_bytes'])for e in gc_plan['sites']if 'old_helper' in e]
omitted_native_sites={0x1452bf:'Official update replaced actor-count loop with online lobby-count logic',0x14555b:'Official update replaced actor-count loop with online lobby-count logic'}
rip_register_sites={0x1da6f5:0x1dcae4,0x1da794:0x1dcb74}
def current_register_patch(at,dest):
 original=next(md.disasm(read(old,at,15),BASE+at))
 source=next(md.disasm(read(mod,at,15),BASE+at))
 target=next(md.disasm(read(new,dest,15),BASE+dest))
 assert original.id and source.id and target.id
 assert original.mnemonic==source.mnemonic==target.mnemonic=='cmp'
 assert original.size==source.size==target.size and original.disp_size==source.disp_size==target.disp_size==4
 before=bytearray(original.bytes);after=bytearray(source.bytes)
 before[original.disp_offset:original.disp_offset+4]=bytes(4)
 after[source.disp_offset:source.disp_offset+4]=bytes(4)
 assert before==after,'Old patch changed more than its count reference'
 old_target=at+original.size+original.disp
 source_target=at+source.size+source.disp
 actual_target=dest+target.size+target.disp
 assert mapped(old_target)==actual_target==newGC+0x38
 assert source_target==oldGC+0x54
 result=bytearray(target.bytes);W('<i',result,target.disp_offset,mapped(source_target)-dest-target.size)
 return bytes(result)

cache=R/'native-patch-audit.json'
if cache.exists():
 report=json.loads(cache.read_text());patches=report['instructions'];conflicts=[]
 for pi,(at,dest,n) in enumerate(patches):
  dest=mapped(at);patches[pi]=(at,dest,n)
  ins=next(md.disasm(read(old,at,n),BASE+at));assert ins.id and ins.size==n
  expected=relocate(ins,dest);actual=read(new,dest,n)
  if actual!=expected and at not in omitted_native_sites and at!=0x22e510 and at not in rip_register_sites and at not in gc_owned_sites and at not in victory_owned_sites:conflicts.append(dict(source=hex(at),epic=hex(dest),instruction=ins.mnemonic+' '+ins.op_str,expected=expected.hex(),actual=actual.hex()))
 report['instructions']=patches;report['conflicts']=conflicts;cache.write_text(json.dumps(report,indent=2))
else:
 patches=[];conflicts=[]
 for ins in md.disasm(old[os[0][4]:os[0][4]+os[0][1]],BASE+os[0][2]):
  if not ins.id:continue
  at=ins.address-BASE
  if read(mod,at,ins.size)==ins.bytes:continue
  dest=mapped(at);expected=relocate(ins,dest);actual=read(new,dest,ins.size)
  patches.append((at,dest,ins.size))
  if actual!=expected and at not in omitted_native_sites and at!=0x22e510 and at not in rip_register_sites and at not in gc_owned_sites and at not in victory_owned_sites:conflicts.append(dict(source=hex(at),epic=hex(dest),instruction=ins.mnemonic+' '+ins.op_str,expected=expected.hex(),actual=actual.hex()))
 report=dict(instructions=patches,conflicts=conflicts,custom_shift=SHIFT,steam_symbols=len(a),epic_symbols=len(b));cache.write_text(json.dumps(report,indent=2))
print('Native patch instructions',len(patches),'conflicts',len(conflicts),'custom shift',hex(SHIFT));print(*conflicts[:16],sep='\n')
assert not conflicts,'Native instruction conflicts require investigation'
for at,dest in rip_register_sites.items():current_register_patch(at,dest)
report['omitted_native_sites']={hex(k):v for k,v in omitted_native_sites.items()}
report['register_preserving_sites']={hex(k):hex(v)for k,v in rip_register_sites.items()}
report['respawn_adapter']=respawn_plan
cache.write_text(json.dumps(report,indent=2))
patches=[p for p in patches if p[0] not in omitted_native_sites and p[0] not in gc_owned_sites and p[0] not in victory_owned_sites]

pe=U('<I',new,60)[0];opt=pe+24;headers=opt+U('<H',new,pe+20)[0];oldheaders=U('<I',new,opt+60)[0]
growth=max(0,align(headers+40*len(ms),0x200)-oldheaders)
out=bytearray(new[:oldheaders]+bytes(growth)+new[oldheaders:])
for sec in ns:W('<I',out,sec[5]+20,sec[4]+growth)
W('<I',out,opt+60,oldheaders+growth)
dbg,dbgsize=directory(new,6)
if dbg:
 for p in range(off(out,dbg),off(out,dbg)+dbgsize,28):
  pointer=U('<I',out,p+24)[0]
  if pointer:W('<I',out,p+24,pointer+growth)
cert,n=directory(new,4)
if cert:W('<I',out,opt+112+32,cert+growth)
coff=U('<I',out,pe+12)[0]
if coff:W('<I',out,pe+12,coff+growth)
for index,sec in enumerate(custom,len(ns)):
 name,size,r,cap,raw,h=sec;dest=align(len(out),0x200);out.extend(bytes(dest-len(out)));out.extend(mod[raw:raw+cap]);nh=headers+40*index
 out[nh:nh+40]=mod[h:h+40];W('<I',out,nh+12,r+SHIFT);W('<I',out,nh+20,dest)
W('<H',out,pe+6,len(ms))
def write(r,data):p=off(out,r);out[p:p+len(data)]=data

ranges=[]
for at,dest,n in patches:
 if ranges and ranges[-1][0]+ranges[-1][2]==at and ranges[-1][1]+ranges[-1][2]==dest:
  x,y,z=ranges[-1];ranges[-1]=(x,y,z+n)
 else:ranges.append((at,dest,n))
roots=set()
for at,dest,n in ranges:
 done=0
 for ins in md.disasm(read(mod,at,n),BASE+at):
  assert ins.id,('Invalid native replacement',hex(at));pos=ins.address-BASE
  replacement=current_register_patch(pos,dest+pos-at) if pos in rip_register_sites else relocate(ins,dest+pos-at)
  write(dest+pos-at,replacement);done+=ins.size
  for op in ins.operands:
   if op.type==X86_OP_IMM and(ins.group(CS_GRP_JUMP)or ins.group(CS_GRP_CALL))and iscustom(op.imm-BASE):roots.add(op.imm-BASE)
 assert done==n,(hex(at),done,n)

data_changes=[]
source_dbg,source_dbgsize=directory(old,6)
structural_raw_pointers={source_dbg+j+24 for j in range(0,source_dbgsize,28)}if source_dbg else set()
for sec in os[1:]:
 if sec[0]=='.pdata':continue
 for i in range(0,sec[3],8):
  at=sec[2]+i;before=read(old,at,8);after=read(mod,at,8)
  if before==after:continue
  dest=mapped(at);value=U('<Q',before)[0];changed=U('<Q',after)[0]
  if BASE<=value<BASE+custom[-1][2]+custom[-1][1]and BASE<=changed<BASE+custom[-1][2]+custom[-1][1]:
   assert read(new,dest,8)==P('<Q',BASE+mapped(value-BASE)),('Native pointer mismatch',hex(at),hex(dest))
   write(dest,P('<Q',BASE+mapped(changed-BASE)))
   if iscustom(changed-BASE):roots.add(changed-BASE)
  else:
   actual=read(new,dest,8);replacement=bytearray(read(out,dest,8))
   for k in(0,4):
    if at+k in structural_raw_pointers:continue
    bv=U('<I',before,k)[0];av=U('<I',after,k)[0];ev=U('<I',actual,k)[0]
    if bv==av:continue # Preserve untouched official metadata/pointers in this half-record.
    is_rva=bv>=0x1000 and bv<custom[0][2] and (bv in keyset or ev!=bv)
    if is_rva and ev==mapped(bv):W('<I',replacement,k,mapped(av)if av else 0)
    else:
     assert ev==bv,('Native scalar/RVA mismatch',hex(at+k),hex(dest+k),hex(bv),hex(ev))
     W('<I',replacement,k,mapped(av)if bv==0 and iscustom(av)else av)
   write(dest,replacement)
  data_changes.append((at,dest,before.hex(),after.hex()))

custom_runtime=[e for e in runtime(mod)if iscustom(e[0])]
for start,end,unwind in custom_runtime:roots.add(start)
code={};pending=list(roots)
while pending:
 at=pending.pop()
 if at in code or not iscustom(at):continue
 sec=next(s for s in custom if s[2]<=at<s[2]+s[1]);assert sec[0]in('.party5','.fight'),('Non-code target',hex(at))
 for ins in md.disasm(read(mod,at,min(16384,sec[2]+sec[1]-at)),BASE+at):
  pos=ins.address-BASE
  if pos in code:break
  assert ins.id,('Invalid custom code',hex(pos))
  code[pos]=ins
  for op in ins.operands:
   if op.type==X86_OP_IMM and(ins.group(CS_GRP_JUMP)or ins.group(CS_GRP_CALL))and iscustom(op.imm-BASE):pending.append(op.imm-BASE)
  if ins.group(CS_GRP_RET)or ins.mnemonic in('jmp','int3','ud2'):break
for at,ins in code.items():write(mapped(at),relocate(ins,mapped(at)))
covered={p for at,ins in code.items()for p in range(at,at+ins.size)}

steam_runtime={e[0]:e for e in runtime(old)};epic_runtime={e[0]:e for e in runtime(new)};source_records=set(runtime(mod))
merged_native_unwind_scopes=[]
def runtime_mapping(entry):
 if iscustom(entry[0]):return tuple(mapped(v)for v in entry)
 target_start=mapped(entry[0])
 if target_start in epic_runtime:actual=epic_runtime[target_start]
 else:
  candidates=[e for e in epic_runtime.values()if e[0]<=target_start<e[1]]
  assert len(candidates)==1,('Native unwind scope no longer resolves',entry,target_start)
  actual=candidates[0]
  merged_native_unwind_scopes.append(dict(source=entry,mapped_source=target_start,current_scope=actual))
 if entry[0]in steam_runtime and entry[2]!=steam_runtime[entry[0]][2]:return(actual[0],actual[1],mapped(entry[2]))
 return actual
table_changes=[];unused_runtime_rows=[]
for sec in custom:
 name,size,r,cap,raw,h=sec;at=r
 while at+4<=r+cap:
  if at%4 or any(p in covered for p in range(at,at+4)):at+=1;continue
  if at+12<=r+cap:
   entry=U('<III',mod,raw+at-r)
   if entry in source_records:
    if not iscustom(entry[0])and mapped(entry[0])not in epic_runtime:
     unused_runtime_rows.append((at,entry));at+=12;continue
    write(mapped(at),P('<III',*runtime_mapping(entry)));table_changes.append((at,'runtime'));at+=12;continue
  if at+8<=r+cap:
   value=U('<Q',mod,raw+at-r)[0]
   if BASE<=value<BASE+custom[-1][2]+custom[-1][1]and (value-BASE in keyset or iscustom(value-BASE)):
    write(mapped(at),P('<Q',BASE+mapped(value-BASE)));table_changes.append((at,'VA'));at+=8;continue
  value=U('<I',mod,raw+at-r)[0]
  if value in keyset and 0x1000<=value<os[-1][2]+os[-1][1]:write(mapped(at),P('<I',mapped(value)));table_changes.append((at,'RVA'))
  at+=4
referenced={}
for at,ins in code.items():
 for op in ins.operands:
  if op.type!=X86_OP_MEM or op.mem.base!=X86_REG_RIP:continue
  table=at+ins.size+ins.disp
  if not iscustom(table)or table in covered:continue
  entries=[]
  for delta in range(0,256,4):
   pos=table+delta
   if any(p in covered for p in range(pos,pos+4)):break
   try:value=U('<I',read(mod,pos,4))[0]
   except ValueError:break
   if value==0:continue
   if value not in keyset or value<0x1000:break
   write(mapped(pos),P('<I',mapped(value)));entries.append((hex(pos),hex(value),hex(mapped(value))))
  if entries:referenced[hex(table)]=entries
for at,ins in code.items():
 for op in ins.operands:
  if op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP:
   target=at+ins.size+ins.disp
   assert not any(p<=target<p+12 for p,row in unused_runtime_rows),('Reference to unported historical runtime row',hex(at),hex(target))

overrides={e[0]:e for e in runtime(mod)if e[0]in steam_runtime and e!=steam_runtime[e[0]]}
override_epic={mapped(k):runtime_mapping(v)for k,v in overrides.items()}
records=[override_epic.get(e[0],e)for e in runtime(new)]+[runtime_mapping(e)for e in custom_runtime]
records.sort();assert all(x[1]<=y[0]for x,y in zip(records,records[1:])), 'Overlapping unwind records'
for start,end,unwind in custom_runtime:
 flags=read(mod,unwind,4);trailer=unwind+4+((flags[2]+1)&~1)*2
 if flags[0]>>3&4:
  chain=U('<III',read(mod,trailer,12));write(mapped(trailer),P('<III',*runtime_mapping(chain)))
 elif flags[0]>>3&3:write(mapped(trailer),P('<I',mapped(U('<I',read(mod,trailer,4))[0])))
last=sections(out)[-1];tableoff=align(last[1],16);table_rva=last[2]+tableoff;payload=b''.join(P('<III',*e)for e in records)
need=tableoff+len(payload);capacity=align(need,0x200);out.extend(bytes(last[4]+capacity-len(out)));write(table_rva,payload)
W('<I',out,last[5]+8,need);W('<I',out,last[5]+16,capacity);W('<II',out,opt+112+24,table_rva,len(payload));W('<I',out,opt+56,align(last[2]+need,0x1000));W('<I',out,opt+64,0)

gaps=[]
# These two original scopes include trailing literal pools after an unconditional
# jump/return. They are data (pointer/float tables), not unvisited instructions.
literal_pools=[(0x18cc10ba,0x18cc1104),(0x18cc1db0,0x18cc1e10)]
assert all(not any(p in covered for p in range(lo,hi))for lo,hi in literal_pools)
for start,end,unwind in custom_runtime:
 missing=[p for p in range(start,end)if p not in covered and not any(lo<=p<hi for lo,hi in literal_pools)and read(mod,p,1)not in(b'\x90',b'\xcc',b'\0')]
 if missing:gaps.append((hex(start),hex(end),len(missing),hex(missing[0])))
continuations=[]
for at,ins in code.items():
 if any(st<=at<st+n for st,n in handled_custom_ranges):continue
 for op in ins.operands:
  if op.type!=X86_OP_IMM or not(ins.group(CS_GRP_JUMP)or ins.group(CS_GRP_CALL)):continue
  target=op.imm-BASE
  if not(0x1000<=target<os[0][2]+os[0][1])or target in keyset:continue
  first=next(md.disasm(read(old,target,15),BASE+target),None)
  if first and first.id and relocate(first,mapped(target))!=read(new,mapped(target),first.size):continuations.append((hex(at),hex(target),hex(mapped(target)),first.mnemonic,first.op_str))
print('Uncovered runtime ranges:',gaps);print('Native continuation mismatches:',continuations)
(R/'coverage-audit.json').write_text(json.dumps(dict(gaps=gaps,continuations=continuations),indent=2))
assert not gaps and not continuations
adapter=apply_adapter(out,old,mod,new,mapped,read,write,runtime)
gc_adapter=apply_gc_adapter(out,old,mod,new,mapped,read,write,runtime)
allowed={p for at,dest,n in ranges for p in range(dest,dest+n)}|set(range(adapter['new_entry'],adapter['new_entry']+7))
allowed.update(p for e in gc_adapter['sites'] for p in range(e['new_site'],e['new_site']+e['native_overwrite_bytes']))
actual_text_changes=[ns[0][2]+i for i,(u,v)in enumerate(zip(new[ns[0][4]:ns[0][4]+ns[0][1]],read(out,ns[0][2],ns[0][1])))if u!=v]
assert all(p in allowed for p in actual_text_changes),'Unintended official native code change'
outpath=R/'Shrek.team-swap-updated.exe';outpath.write_bytes(out)
(R/'validation-address-overrides.json').write_text(json.dumps({hex(k):hex(v)for k,v in native_address_overrides.items()},indent=2))
report=dict(steam_clean_sha256=sha256(old).hexdigest(),steam_mod_sha256=sha256(mod).hexdigest(),current_clean_sha256=sha256(new).hexdigest(),current_mod_sha256=sha256(out).hexdigest(),custom_shift=SHIFT,native_ranges=ranges,native_data_changes=data_changes,custom_instructions=len(code),custom_runtime_count=len(custom_runtime),table_changes=len(table_changes),referenced_tables=referenced,native_overrides=len(overrides),old_gc=oldGC,new_gc=newGC,header_growth=growth,unused_historical_runtime_rows=unused_runtime_rows,respawn_adapter=adapter,gc_iteration_adapter=gc_adapter,omitted_native_sites=report['omitted_native_sites'],register_preserving_sites=report['register_preserving_sites'],updated_native_text_preserved=True,merged_native_unwind_scopes=merged_native_unwind_scopes)
(R/'port-manifest.json').write_text(json.dumps(report,indent=2))
print('Built',outpath.name,sha256(out).hexdigest(),'native ranges',len(ranges),'custom instructions',len(code),'runtime',len(custom_runtime))
