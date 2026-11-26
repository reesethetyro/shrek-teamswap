"""Recreate text assets from exact local clean inputs. Outputs only into build/assets."""
from pathlib import Path
from hashlib import sha256
import json,argparse

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--platform',required=True,choices=['Steam','Epic','Steam26880364','Epic593174']);ap.add_argument('--game-root',required=True);args=ap.parse_args();repo=Path(__file__).resolve().parents[1];root=Path(args.game_root).resolve();out=(repo/'build/assets'/args.platform).resolve()
    if out==root or root in out.parents:raise SystemExit('Output must not be inside the game directory')
    for recipe in json.loads((repo/'src/assets/text-edits.json').read_text()):
        if recipe['platform']!=args.platform:continue
        path=Path(recipe['path']);assert not path.is_absolute()and '..'not in path.parts
        data=(root/path).read_bytes()
        if sha256(data).hexdigest()!=recipe['clean_sha256']:raise ValueError('Expected clean input: '+str(path))
        for edit in reversed(recipe['changes']):data=data[:edit['offset']]+edit['text'].encode('utf-8')+data[edit['offset']+edit['remove']:]
        assert sha256(data).hexdigest()==recipe['mod_sha256'];dest=out/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
        print(path,'MATCH')

if __name__=='__main__':main()
