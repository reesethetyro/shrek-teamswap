from hashlib import sha256
from pathlib import Path
from struct import pack, unpack_from
import json
import re

work = Path('work')
image = (work / 'Shrek.weight-help.exe').read_bytes()
assert sha256(image).hexdigest() == 'd2b15b14c095e1db6efb16a38e97a58da968978d0f5089c855a3b479212ad8c7'
pe = unpack_from('<I', image, 0x3c)[0]
optional = pe + 24
section_count = unpack_from('<H', image, pe + 6)[0]
assert section_count == 9
next_header = optional + 0xf0 + section_count * 40
assert image[next_header:next_header + 40] == bytes(40)
roster_rva = unpack_from('<I', image, optional + 56)[0]
assert roster_rva % 0x1000 == 0

old_base = 0x1d0e800
controller = 0x1d0e7f0
old_controller_offset = 0x10
new_controller_offset = roster_rva - controller
pointer_count = 9
edits = []

def record(site, old, new, kind):
    pos = site - 0xc00
    assert image[pos:pos + len(old)] == old, hex(site)
    edits.append({'rva': hex(site), 'expected': old.hex(' '),
                  'replacement': new.hex(' '), 'kind': kind})

audit = (work / 'nine-slot-reference-audit.txt').read_text(encoding='utf-8').split('Party count')[0]
for line in audit.splitlines()[1:]:
    match = re.match(r'^([0-9a-f]{8})  ((?:[0-9a-f]{2} )+[0-9a-f]{2}) ', line)
    if not match:
        continue
    site = int(match[1], 16)
    old = bytes.fromhex(match[2])
    assert site + len(old) + unpack_from('<i', old, len(old) - 4)[0] == old_base
    new = old[:-4] + pack('<i', roster_rva - site - len(old))
    record(site, old, new, 'direct RIP-relative roster')

for line in (work / 'game-disassembly.txt').open(encoding='utf-8-sig'):
    match = re.match(r'^140([0-9a-f]{6}): ((?:[0-9a-f]{2} )+)\s+mov\s+.*\+ 0x1d0e808\]', line)
    if not match:
        continue
    site = int(match[1], 16)
    length = len(bytes.fromhex(match[2]))
    old = image[site - 0xc00:site - 0xc00 + length]
    assert old.endswith(pack('<I', old_base))
    record(site, old, old[:-4] + pack('<I', roster_rva), 'image-base indexed roster')

# Record only already-audited operands whose current meaning is known.
# The longer encodings require a relocation of whole native instructions.
controller_sites = {
    0x22b1e6: '49 8b 4c c7 10', 0x22b1fe: '48 89 6b 10',
    0x22b236: '4d 8d 77 10', 0x22b27d: '49 8b 4c df 10',
    0x22b2a0: '49 89 6c df 10', 0x22b318: '48 8b 4c ce 10',
    0x22b365: '4a 8b 74 f6 10', 0x22c3d5: '48 8b 7c d9 10',
    0x22c3fb: '48 8d 4e 10', 0x22c46a: '48 8d 41 10',
    0x22c52f: '48 8d 41 10', 0x22c96f: '48 8b 44 c1 10',
    0x22ec4a: '4f 8b 54 ce 10', 0x22ecfa: '4f 8b 54 ce 10',
    0x22ed9a: '4f 8b 54 ce 10', 0x22ef46: '4c 8d 43 10',
    0x22f114: '48 8d 41 10', 0x22f1e9: '48 8b 4c 2f 10',
    0x22f1fd: '4c 89 74 2f 10', 0x22f37e: '49 8b 4c d3 10',
    0x22f5bd: '49 8b 5c ee 10', 0x22f6cc: '49 8b 54 c6 10',
    0x22f7b9: '49 8b 4c fe 10', 0x22f92a: '49 8b 6c fe 10',
    0x232d98: '49 8b 4c c4 10', 0x232e65: '49 8b 4c c4 10',
    0x237a82: '4d 8b 54 c3 10',
}
for site, encoding in controller_sites.items():
    old = bytes.fromhex(encoding)
    assert old[2] >> 6 == 1 and old[-1] == old_controller_offset
    new = old[:2] + bytes((old[2] + 0x40,)) + old[3:-1] + pack('<i', new_controller_offset)
    record(site, old, new, 'controller-relative roster; needs trampoline')

assert len(edits) == 57
manifest = {
    'source_sha256': sha256(image).hexdigest(),
    'roster_rva': hex(roster_rva), 'pointer_count': pointer_count,
    'unchanged_count_rva': hex(controller + 0x54),
    'native_operands': edits,
    'additional_work': [
        'Rewrite constructor, nine-slot release loop, destructor and first-slot insertion.',
        'Move derived roster pointers and injected switch, quest and helper accesses.',
        'Expand AI combat data and temporary assigned flags to cover eight CPU characters.',
        'Verify lifecycle, switch wrap, and party count across Swamp and Cookie, Cookie.',
    ],
}
(work / 'nine-slot-relocation-plan.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
print('Verified', len(edits), 'source operands for separate nine-pointer storage at RVA', hex(roster_rva))
