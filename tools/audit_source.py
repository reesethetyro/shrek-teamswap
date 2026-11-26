"""Audit only the publishable source tree; ignored local build/input directories are skipped."""
from pathlib import Path
from hashlib import sha256
import re,json
repo=Path(__file__).resolve().parents[1];banned={'.exe','.dll','.pdb','.obj','.rgba','.dds','.bin','.patch','.bak','.zip','.sav','.png','.jpg','.wav','.mp3'};issues=[];files=[]
for p in sorted(repo.rglob('*')):
    rel=p.relative_to(repo)
    if not p.is_file()or rel.parts[0]in {'build','inputs','.git','.venv'}or '__pycache__'in rel.parts:continue
    files.append(p)
    if p.suffix.lower()in banned:issues.append(str(rel)+': binary/private-input extension')
    text=p.read_text(encoding='utf-8-sig')
    if re.search(r'[A-Za-z]:[/\\]Users[/\\]|D:/Steam[L]ibrary|gh[pousr]_[A-Za-z0-9]{20,}|sk-[A-Za-z0-9]{24,}|AKIA[A-Z0-9]{16}|BEGIN [A-Z ]*PRIVATE KEY',text):issues.append(str(rel)+': personal path or credential pattern')
    if rel.parts[0]=='src'and re.search('cindy|cinderella',text,re.I):issues.append(str(rel)+': excluded experimental-character source')
assert not issues,issues
print('PASS source-only audit:',len(files),'files; no forbidden binaries, personal paths, credential patterns or excluded character source')
