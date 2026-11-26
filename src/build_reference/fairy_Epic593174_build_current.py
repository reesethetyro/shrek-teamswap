# Reference builder; historical inputs are not bundled. Use root BUILDING.md for tested commands.
"""Recompile the byte-reproduced latest Team Swap extension for EpicCL593174."""
from pathlib import Path
from struct import unpack_from as U,pack_into as W,pack as P
from bisect import bisect_right
from hashlib import sha256
import sys,json,re,subprocess,shutil

R=Path(__file__).resolve().parent
SOURCE=R.parents[1]/'team-swap-latest-2026-11-24/input/Source/Steam'
PUBLIC=R.parent
OLD=Path(r'inputs/history/2026-11-16/we-x20/work/steam-update-2026-11-18')
sys.path.insert(0,str(OLD/'vendor'))
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_GRP_CALL,CS_GRP_JUMP
from capstone.x86 import X86_OP_MEM,X86_OP_IMM,X86_REG_RIP
MD=Cs(CS_ARCH_X86,CS_MODE_64);MD.detail=True
BASE=0x140000000
old_public=(OLD/'Shrek.team-swap-updated.exe').read_bytes()
current_public=(PUBLIC/'Shrek.Epic.team-swap.exe').read_bytes()
assert sha256(old_public).hexdigest()=='dca90e19044eb69163ab788d54ff922c6ecbf14b59067f0135dd7df8db1be68b'
assert sha256(current_public).hexdigest()=='47e15ca767690aca7ad466cff47dc85d1533c58f7517d78395feeab199e8b874'

def symbols(path):return {n:int(a,16)-BASE for a,n in (l.split(' ',1) for l in path.read_text().splitlines())}
a,b=symbols(OLD/'symbols.txt'),symbols(PUBLIC/'symbols.txt')
pairs=sorted((v,b[n],n)for n,v in a.items()if n in b);keys=[p[0]for p in pairs]
exact_symbols={start:dest for start,dest,name in pairs}
# Extra alignment below maps authoritative old Steam source directly to current Epic.
alignment=[]
if (R/'extra-alignment.json').is_file():
    extra=json.loads((R/'extra-alignment.json').read_text())
    if isinstance(extra,dict):extra=extra.get('spans',[])
    alignment.extend(extra)
spans={old:(new,size)for old,new,size in alignment};span_keys=sorted(spans)
def first_custom(image):
 pe=U('<I',image,60)[0];table=pe+24+U('<H',image,pe+20)[0];return U('<I',image,table+7*40+12)[0]
custom_shift=first_custom(current_public)-first_custom(old_public)
assert custom_shift==0x2000
remaps={}

def mapped(value):
    if value==0:return 0
    if 0x18cc0000<=value<0x18dc0000:result=value+custom_shift
    elif value in exact_symbols:result=exact_symbols[value]
    else:
        i=bisect_right(span_keys,value)-1
        if i>=0 and value<span_keys[i]+spans[span_keys[i]][1]:
            start=span_keys[i];result=spans[start][0]+value-start
        else:
            i=bisect_right(keys,value)-1
            assert i>=0,hex(value)
            start,dest,name=pairs[i];result=dest+value-start
    remaps[value]=result
    return result

def image_read(image,rva,size):
    pe=U('<I',image,60)[0];table=pe+24+U('<H',image,pe+20)[0]
    for i in range(U('<H',image,pe+6)[0]):
        length,start,capacity,raw=U('<4I',image,table+40*i+8)
        if start<=rva<start+capacity:return image[raw+rva-start:raw+rva-start+size]
    raise ValueError(hex(rva))

def ensure_section_header_room(image):
    """Make room for the11thsection without changing any virtual address."""
    pe=U('<I',image,60)[0];opt=pe+24;table=opt+U('<H',image,pe+20)[0]
    count=U('<H',image,pe+6)[0];oldheaders=U('<I',image,opt+60)[0]
    required=(table+(count+1)*40+511)&-512
    growth=max(0,required-oldheaders)
    if not growth:return image
    image=bytearray(image[:oldheaders]+bytes(growth)+image[oldheaders:])
    for i in range(count):
        p=table+i*40+20
        W('<I',image,p,U('<I',image,p)[0]+growth)
    W('<I',image,opt+60,required)
    debug,size=U('<II',image,opt+112+6*8)
    if debug:
        # Locate the relocated debug directory and update raw-file pointers.
        for i in range(count):
            length,start,capacity,raw=U('<4I',image,table+i*40+8)
            if start<=debug<start+capacity:
                pos=raw+debug-start
                break
        else:raise ValueError('Debug directory outside image')
        for offset in range(pos,pos+size,28):
            pointer=U('<I',image,offset+24)[0]
            if pointer:W('<I',image,offset+24,pointer+growth)
    certificate=U('<I',image,opt+112+4*8)[0]
    if certificate:W('<I',image,opt+112+4*8,certificate+growth)
    return image

def relocate_expected(data,origin,dest):
    """Strict byteproof for the original native instructions being replaced."""
    result=bytearray(data);decoded=0
    for ins in MD.disasm(data,BASE+origin):
        at=ins.address-BASE;offset=at-origin;decoded+=ins.size
        for op in ins.operands:
            if op.type==X86_OP_MEM and op.mem.base==X86_REG_RIP:
                target=at+ins.size+ins.disp
                W('<i',result,offset+ins.disp_offset,mapped(target)-(dest+offset+ins.size))
            elif op.type==X86_OP_IMM and(ins.group(CS_GRP_CALL)or ins.group(CS_GRP_JUMP)):
                target=mapped(op.imm-BASE)
                W('<b'if ins.imm_size==1 else'<i',result,offset+ins.imm_offset,target-(dest+offset+ins.size))
    assert decoded==len(data),(hex(origin),data.hex(),decoded)
    return bytes(result)

def patch_data(data,origin,dest):
    """The supplied builder already targets the newly compiled CODE_RVA."""
    result=bytearray(data);decoded=0
    for ins in MD.disasm(data,BASE+origin):
        at=ins.address-BASE;offset=at-origin;decoded+=ins.size
        for op in ins.operands:
            if op.type==X86_OP_IMM and(ins.group(CS_GRP_CALL)or ins.group(CS_GRP_JUMP)):
                target=op.imm-BASE
                W('<b'if ins.imm_size==1 else'<i',result,offset+ins.imm_offset,target-(dest+offset+ins.size))
    assert decoded==len(data),(hex(origin),data.hex(),decoded)
    return bytes(result)

def translate_source(text):
    def replace(m):
        v=int(m[0],16)
        return hex(mapped(v))if 0x70000<=v<0x18dc0000 else m[0]
    return re.sub(r'0x[0-9a-fA-F]+',replace,text)

(R/'fairy.cpp').write_text(translate_source((SOURCE/'fairy.cpp').read_text()))
asm=translate_source((SOURCE/'fairy.s').read_text())
for value in (1820261,1820597):asm=asm.replace(str(value),str(mapped(value)))
asm=asm.replace('work/fairy-exits/shield-bar.rgba',(R/'shield-bar.rgba').as_posix())
(R/'fairy.s').write_text(asm)
shutil.copy2(SOURCE/'shield-bar.rgba',R/'shield-bar.rgba')
shutil.copy2(PUBLIC/'Shrek.Epic.team-swap.exe',R/'Shrek.before-fairy.exe')

# Retain the known-reproducing compile/COFF/unwind/PE workflow. Keep supplied
# source addresses in its patch calls, translate each site at the write boundary,
# and validate both the authoritative old baseline and current public image.
builder=(SOURCE/'build.py').read_text()
builder=builder.replace("assert baseline=='dca90e19044eb69163ab788d54ff922c6ecbf14b59067f0135dd7df8db1be68b'",
                        "assert baseline=='47e15ca767690aca7ad466cff47dc85d1533c58f7517d78395feeab199e8b874'")
builder=builder.replace("pe=u('<I',b,60)[0];opt=pe+24;", "b=ensure_section_header_room(b)\npe=u('<I',b,60)[0];opt=pe+24;",1)
builder=builder.replace('def read(rv,k):return bytes(b[off(rv):off(rv)+k])',
                        'def read(rv,k):return image_read(old_public,rv,k)')
for expression in ['0x259b80+u("<i",read(0x259b7c,4))[0]',
                   '0x25b1c5+u("<i",read(0x25b1c1,4))[0]',
                   '0x259d72+u("<i",read(0x259d6e,4))[0]']:
    builder=builder.replace('{'+expression+'}','{mapped('+expression+')}')
builder=builder.replace('parent=next(v for v in newrecords',
                        'start=mapped(start);end=mapped(end)\n parent=next(v for v in newrecords')
before="""def patch(site,expected,data):
 assert read(site,len(expected))==expected,(hex(site),read(site,len(expected)).hex());b[off(site):off(site)+len(data)]=data;patches.append(dict(rva=site,before=expected.hex(),after=data.hex()))"""
after="""def patch(site,expected,data):
 assert read(site,len(expected))==expected,('Old source changed',hex(site),read(site,len(expected)).hex(),expected.hex())
 newsite=mapped(site);current_expected=relocate_expected(expected,site,newsite)
 current=image_read(bytes(b),newsite,len(current_expected))
 assert current==current_expected,('Current native hook mismatch',hex(site),hex(newsite),current_expected.hex(),current.hex())
 replacement=patch_data(data,site,newsite)
 b[off(newsite):off(newsite)+len(replacement)]=replacement
 patches.append(dict(rva=newsite,old_rva=site,before=current_expected.hex(),after=replacement.hex()))"""
assert before in builder
builder=builder.replace(before,after)
builder=builder.replace("out=r/'Shrek.Fairy-v7.exe'","out=r/'Shrek.Fairy-current.exe'")
builder=builder.replace("version='Team Swap next update - Fairy exits and KO countdown'",
                        "version='Team Swap Fairy update for Epic CL593174'")
builder=builder.replace('steam_only=True',"steam_only=False,platform='Epic CL593174'")
builder=builder.replace("preserves='Public nine-character Steam Team Swap baseline; Fairy changes included; release not published'","preserves='Current Epic CL593174 Team Swap baseline; complete authoritative Fairy/settings features'")
(R/'build_generated.py').write_text(builder)
context=dict(globals());context['__file__']=str(R/'build_generated.py')
try:
    exec(compile(builder,str(R/'build_generated.py'),'exec'),context)
finally:
    (R/'mapping.json').write_text(json.dumps({hex(k):hex(v)for k,v in sorted(remaps.items())},indent=2))
