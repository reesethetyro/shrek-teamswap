"""Compile the historical public nine-character menu/swap core for inspection."""
from pathlib import Path
from hashlib import sha256
import json,argparse,subprocess
from compile_sources import linked_text
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--clang',default='clang');args=ap.parse_args();repo=Path(__file__).resolve().parents[1];cfg=json.loads((repo/'review/legacy-core-config.json').read_text());out=repo/'build/core';out.mkdir(parents=True,exist_ok=True)
command=[args.clang,'-target','x86_64-pc-windows-msvc','-x','assembler-with-cpp',*['-D{}={}'.format(k,v)for k,v in cfg['definitions'].items()],'-c',str(repo/cfg['entry']),'-o',str(out/'core.obj')]
subprocess.check_call(command,cwd=str((repo/cfg['entry']).parent));text=linked_text((out/'core.obj').read_bytes());result=dict(text_sha256=sha256(text).hexdigest(),length=len(text),scope=cfg['status']);assert result['text_sha256']==cfg['expected_text_sha256'] and result['length']==cfg['expected_length']
(out/'result.json').write_text(json.dumps(result,indent=2));print(result)
