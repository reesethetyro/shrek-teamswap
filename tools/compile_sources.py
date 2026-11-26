"""Compile review objects only. Does not modify any installed game files."""
from pathlib import Path
from struct import unpack_from as U, pack_into as W
from hashlib import sha256
import argparse,json,re,subprocess,shutil
from palette import generate

def linked_text(obj):
    symtab,num=U('<II',obj,8);strings=symtab+18*num;syms={};i=0
    while i<num:
        a=symtab+i*18;name=obj[a:a+8]
        if name[:4]==bytes(4):q=strings+U('<I',name,4)[0];name=obj[q:obj.index(0,q)]
        value,section=U('<Ih',obj,a+8);syms[i]=(value,section);i+=1+obj[a+17]
    size,raw,rel=U('<III',obj,36);nr=U('<H',obj,52)[0];blob=bytearray(obj[raw:raw+size])
    for i in range(nr):
        pos,index,kind=U('<IIH',obj,rel+i*10);value,section=syms[index]
        if kind!=4 or section!=1:raise ValueError('Unexpected text relocation')
        W('<i',blob,pos,value+U('<i',blob,pos)[0]-pos-4)
    return bytes(blob)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--clang',default='clang');ap.add_argument('--game-root',required=True,help='Local game directory; read-only, for its original health-bar texture');args=ap.parse_args()
    repo=Path(__file__).resolve().parents[1];output=repo/'build/objects';output.mkdir(parents=True,exist_ok=True);palette=generate(repo,args.game_root);results=[]
    configs=json.loads((repo/'review/compile-config.json').read_text());expected=json.loads((repo/'review/expected-object-text.json').read_text())
    for cfg in configs:
        key=cfg['layer']+'/'+cfg['platform'];d=output/cfg['layer']/cfg['platform'];d.mkdir(parents=True,exist_ok=True);defs=['-D{}={}'.format(k,v)for k,v in cfg['definitions'].items()]
        subprocess.check_call([args.clang,'-target','x86_64-pc-windows-msvc','-x','c++','-std=c++17','-O2','-ffreestanding','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-builtin','-mno-stack-arg-probe','-funwind-tables','-S','-masm=intel',*defs,str(repo/cfg['cpp']),'-o',str(d/'raw.s')])
        compiled=re.sub(r'^\s*\.section\s+[^\n]+','.text',(d/'raw.s').read_text(),flags=re.M)
        entry=(repo/cfg['entry']).read_text()
        if cfg['include']:
            (d/cfg['include']).write_text(compiled);(d/'shield-bar.rgba').write_bytes(palette);allasm=entry
        else:allasm=entry+compiled
        (d/'all.s').write_text(allasm)
        subprocess.check_call([args.clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp',*defs,'-c','all.s','-o','runtime.obj'],cwd=str(d))
        text=linked_text((d/'runtime.obj').read_bytes());digest=sha256(text).hexdigest();match=digest==expected[key]['sha256'] and len(text)==expected[key]['length']
        results.append(dict(target=key,text_sha256=digest,length=len(text),matches_release_layer=match));print(key, 'MATCH' if match else 'DIFFERENT')
    (repo/'build/compile-results.json').write_text(json.dumps(results,indent=2))
    if not all(r['matches_release_layer']for r in results):raise SystemExit('Compiled text differs; inspect build/compile-results.json and compiler version')

if __name__=='__main__':main()
