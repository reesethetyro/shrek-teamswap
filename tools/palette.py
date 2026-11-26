from pathlib import Path
from struct import unpack_from
from hashlib import sha256
import json

def generate(repo, game_root):
    import texture2ddecoder
    meta=json.loads((repo/'review/palette.json').read_text())
    data=(Path(game_root)/meta['source_relative_path']).read_bytes()
    if sha256(data).hexdigest()!=meta['source_sha256']:raise ValueError('Unsupported health-bar source texture')
    if data[:12]!=bytes.fromhex('ab4b5458203230bb0d0a1a0a'):raise ValueError('Expected KTX2 data')
    width,height=unpack_from('<II',data,20);offset,length=unpack_from('<QQ',data,80)
    bgra=texture2ddecoder.decode_bc7(data[offset:offset+length],width,height)
    rgba=bytearray(len(bgra))
    for i in range(0,len(bgra),4):
        blue,green,red,alpha=bgra[i:i+4]
        rgba[i]=red;rgba[i+1]=min(255,round((blue+.85*(green-blue))*1.12));rgba[i+2]=min(255,round(green*1.12));rgba[i+3]=alpha
    if sha256(rgba).hexdigest()!=meta['output_sha256']:raise ValueError('Palette output does not match the release')
    return bytes(rgba)
