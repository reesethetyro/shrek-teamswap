from pathlib import Path
from hashlib import sha256
import argparse,json,subprocess
from compile_sources import linked_text
ap=argparse.ArgumentParser();ap.add_argument('--clang',default='clang');args=ap.parse_args();repo=Path(__file__).resolve().parents[1];out=repo/'build/gates';out.mkdir(parents=True,exist_ok=True);results=[]
for cfg in json.loads((repo/'review/clock-config.json').read_text()):
    obj=out/(cfg['platform']+'.obj');subprocess.check_call([args.clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp','-DSTART='+str(cfg['start']),'-c',str(repo/'src/clock_gate'/(cfg['platform']+'.s')),'-o',str(obj)])
    text=linked_text(obj.read_bytes());assert len(text)==cfg['length']and sha256(text).hexdigest()==cfg['sha256'];results.append(dict(platform=cfg['platform'],matches_release=True));print(cfg['platform'],'MATCH')
(repo/'build/gate-results.json').write_text(json.dumps(results,indent=2))
