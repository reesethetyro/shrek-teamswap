from hashlib import sha256
from pathlib import Path
from struct import pack, pack_into, unpack_from
import json
import re

work = Path('work')
manifest = json.loads((work / 'nine-slot-relocation-plan.json').read_text())
source = (work / 'Shrek.weight-help.exe').read_bytes()
assert sha256(source).hexdigest() == manifest['source_sha256']
image = bytearray(source)
roster = int(manifest['roster_rva'], 16)
delta = roster - 0x1d0e7f0
instructions = {}
targets = set()
for line in (work / 'game-disassembly.txt').open(encoding='utf-8-sig'):
    match = re.match(r'^140([0-9a-f]{6}): ((?:[0-9a-f]{2} )+)\s*(.*)', line)
    if match:
        rva = int(match[1], 16)
        instructions[rva] = (bytes.fromhex(match[2]), match[3])
        branch = re.match(r'(?:j\w+|call)\s+0x140([0-9a-f]{6})\b', match[3])
        if branch:
            targets.add(int(branch[1], 16))

def current(site, length):
    return bytes(image[site - 0xc00:site - 0xc00 + length])

def edit(site, old, new):
    assert len(old) == len(new) and current(site, len(old)) == old, hex(site)
    image[site - 0xc00:site - 0xc00 + len(old)] = new

pe = unpack_from('<I', image, 0x3c)[0]
opt = pe + 24
code_header, data_header = 0x388, 0x3b0
code_size, code_rva, code_capacity, code_raw = unpack_from('<4I', image, code_header + 8)
data_size, data_rva, data_capacity, data_raw = unpack_from('<4I', image, data_header + 8)
directory, directory_size = unpack_from('<II', image, opt + 112 + 3 * 8)
table_start = data_raw + directory - data_rva
records = [unpack_from('<III', image, pos) for pos in range(table_start, table_start + directory_size, 12)]
payload = bytearray(image[code_raw:code_raw + code_size])
new_records = []

def jump(site, target):
    return b'\xe9' + pack('<i', target - site - 5)

def frame(rva):
    for index in range(9):
        _, base, size, raw = unpack_from('<4I', source, opt + 0xf0 + index * 40 + 8)
        if base <= rva < base + size:
            return source[raw + rva - base + 3]
    raise ValueError(hex(rva))

def stub(site, width, assemble):
    assert width >= 5
    assert not any(address in targets for address in range(site + 1, site + width)), hex(site)
    parents = [record for record in records if record[0] <= site < record[1]]
    start = code_rva + len(payload)
    data = assemble(start)
    payload.extend(data)
    if parents:
        parent, = parents
        payload.extend(bytes(-len(payload) % 4))
        chain = code_rva + len(payload)
        payload.extend(bytes((0x21, 0, 0, frame(parent[2]))) + pack('<III', *parent))
        new_records.append((start, start + len(data), chain))
    branch = jump(site, start) + b'\x90' * (width - 5)
    if code_rva <= site < code_rva + code_size:
        payload[site-code_rva:site-code_rva+width] = branch
    else:
        old = current(site, width)
        edit(site, old, branch)

def linear(site, old, replacement):
    assert current(site, len(old)) == old
    if site in (0x22c52f, 0x22f114):
        prefix = current(site - 3, 3)
        assert prefix in (bytes.fromhex('45 8b c1'), bytes.fromhex('45 33 c9'))
        site -= 3
        old = prefix + old
        replacement = prefix + replacement
    width = len(old)
    suffix = b''
    while width < 5:
        follow, asm = instructions[site + width]
        assert 'rip ' not in asm and not re.match(r'(?:j\w+|call|ret)\s', asm), (hex(site), asm)
        suffix += current(site + width, len(follow))
        width += len(follow)
    stub(site, width, lambda start: replacement + suffix + jump(start + len(replacement) + len(suffix), site + width))

for entry in manifest['native_operands']:
    site = int(entry['rva'], 16)
    old = bytes.fromhex(entry['expected'])
    new = bytes.fromhex(entry['replacement'])
    if len(old) == len(new):
        edit(site, old, new)
    else:
        linear(site, old, new)

for site, encoding, field in [
    (0x22ea8b, '49 8d 7d 18', delta + 8),
    (0x22eb13, '49 8b 4c c5 10', delta),
    (0x22eb9a, '4f 8b 54 ce 10', delta),
    (0x22b717, '48 8d 7b 10', delta),
]:
    old = bytes.fromhex(encoding)
    new = old[:2] + bytes((old[2] + 0x40,)) + old[3:-1] + pack('<i', field)
    linear(site, old, new)
linear(0x22c56b, bytes.fromhex('48 83 c1 10'), b'\x48\x81\xc1' + pack('<i', delta))
edit(0x22b71e, b'\xbe\x06\0\0\0', b'\xbe\x09\0\0\0')

# The prototype is intentionally never emitted as an executable until the
# injected accesses and every lifecycle consumer can pass the same audit.
print(f'Staged {len(new_records)} executable relocations for {roster:#x}.')



def retarget_other_slots():
    for site, (original, asm) in instructions.items():
        match = re.search(r'\[rip ([+-]) 0x([0-9a-f]+)\]', asm)
        if not match:
            continue
        value = int(match[2], 16) * (1 if match[1] == '+' else -1)
        field = original.find(pack('<i', value))
        if field < 0:
            continue
        old = current(site, len(original))
        if old[0] in (0xe8, 0xe9) and old[0] != original[0]:
            continue
        target = site + len(old) + unpack_from('<i', old, field)[0]
        if target not in range(0x1d0e808, 0x1d0e829, 8):
            continue
        if not re.match(r'(?:lea|mov|cmp)\s', asm):
            raise ValueError((hex(site), asm))
        changed = bytearray(old)
        pack_into('<i', changed, field, roster + target - 0x1d0e800 - site - len(old))
        print('Retarget remaining fixed roster access', hex(site), hex(target))
        edit(site, old, changed)
retarget_other_slots()

# Relocate the three roster operands in previously installed hooks.
for encoding in ('4d 8b 44 c3 10', '49 8b 44 cb 10', '4e 8b 5c c8 10'):
    old = bytes.fromhex(encoding)
    locations = [match.start() for match in re.finditer(re.escape(old), source[code_raw:code_raw + code_size])]
    assert len(locations) == (2 if encoding == '4d 8b 44 c3 10' else 1)
    for location in locations:
        site = code_rva + location
        new = old[:2] + bytes((old[2] + 0x40,)) + old[3:-1] + pack('<i', delta)
        stub(site, 5, lambda start: new + jump(start + len(new), site + 5))

def destroy(start):
    # RBX and RDI are saved by the native destructor's frame.
    code = bytearray(b'\x48\x8d\x1d' + pack('<i', roster - start - 7))
    code += bytes.fromhex('bf 09 00 00 00')
    loop = len(code)
    code += bytes.fromhex('48 8b 0b 48 85 c9 74 08')
    code += b'\xe8' + pack('<i', 0x2adb00 - (start + len(code) + 5))
    code += bytes.fromhex('48 89 2b 48 83 c3 08 ff cf 75')
    code += pack('<b', loop - len(code) - 1)
    code += bytes.fromhex('48 8b 5c 24 30')
    code += jump(start + len(code), 0x22aa18)
    return code
stub(0x22aa13, 5, destroy)

def replace_immediate(site, before, after, width=4):
    old = current(site, len(instructions[site][0]))
    needle = before.to_bytes(width, 'little', signed=before < 0)
    replacement = after.to_bytes(width, 'little', signed=after < 0)
    assert old.count(needle) == 1, (hex(site), old.hex())
    edit(site, old, old.replace(needle, replacement))

replace_immediate(0x5ba5b, -0x510, -0x710)
for site in (0x5ba63, 0x5bba5):
    replace_immediate(site, 0x610, 0x810)
for site in (0x5ae16, 0x5aec1, 0x5afa5, 0x5b0cf, 0x5b31b, 0x5b378, 0x5b3e0,
             0x5b3f2, 0x5b4c9, 0x5b597, 0x5b689, 0x5b733, 0x5b76c):
    replace_immediate(site, 0x4e8, 0x668)
replace_immediate(0x5bb1e, 0x418, 0x598)
for site in (0x5b45e, 0x5b882):
    replace_immediate(site, 0x50, 0x68, 1)
for sites, old, new in [
    ((0x5b448, 0x5b8a7), 0x560, 0x710),
    ((0x5adea, 0x5ae86, 0x5b1d3, 0x5b1e0, 0x5b43e, 0x5b522, 0x5b901), 0x578, 0x738),
    ((0x5b6da, 0x5b858, 0x5b941), 0x5a8, 0x780),
    ((0x5adcb, 0x5b8b7, 0x5b8f2), 0x5d8, 0x7c8),
]:
    for site in sites:
        replace_immediate(site, old, new)

combat_stub = 0x5bb80 + unpack_from('<i', current(0x5bb7b, 5), 1)[0]
offset = combat_stub - code_rva + 9
assert payload[offset:offset + 8] == bytes.fromhex('48 8d 9c 24 a8 05 00 00')
payload[offset + 4:offset + 8] = pack('<I', 0x768)

def mapped_raw(rva):
    for index in range(9):
        _, base, size, raw = unpack_from('<4I', image, opt + 0xf0 + index * 40 + 8)
        if base <= rva < base + size:
            return raw + rva - base
    raise ValueError(hex(rva))

_, _, unwind = next(record for record in records if record[0] == 0x5ba50)
offset = mapped_raw(unwind)
unwind_length = 4 + 2 * image[offset + 2]
old = image[offset:offset + unwind_length]
assert old.count(bytes.fromhex('1a 01 c2 00')) == 1
image[offset:offset + unwind_length] = old.replace(bytes.fromhex('1a 01 c2 00'), bytes.fromhex('1a 01 02 01'))
print('Expanded combat storage and destructor to nine records.')


# The movement manager has separate lists for special movement states.
# Keep writes inside their original storage even with more registered players.
for site, capacity_limit, full_target in [(0x5e25f, 6, 0x5e285), (0x5e31b, 5, 0x5e341)]:
    old = instructions[site][0]
    assert current(site, len(old)) == old
    def guard(start):
        data = old + b'\x83\xf8' + bytes((capacity_limit,))
        data += b'\x0f\x83' + pack('<i', full_target - start - len(data) - 6)
        return data + jump(start + len(data), site + len(old))
    stub(site, len(old), guard)

# Apply the existing overflow-follow path to parties of six or more.
site = 0x5e56f
follow_rva = site + 5 + unpack_from('<i', current(site, 5), 1)[0]
follow_offset = follow_rva - code_rva
assert payload[follow_offset + 0x11:follow_offset + 0x13] == b'\x83\x3d'
assert payload[follow_offset + 0x17:follow_offset + 0x1a] == b'\x06\x0f\x85'
payload[follow_offset + 0x19] = 0x82

import subprocess
clang = Path('work/build_mongo_boarding.py').read_text().split("clang = '")[1].split("'")[0]

def assemble(name, entry):
    path = work / (name + '.obj')
    subprocess.run([clang, '-target', 'x86_64-pc-windows-msvc', '-x', 'assembler-with-cpp',
                    f'-DPAYLOAD_RVA={entry}', '-c', str(work / (name + '.s')), '-o', str(path)], check=True)
    obj = path.read_bytes()
    length, raw = unpack_from('<II', obj, 36)
    assert unpack_from('<H', obj, 52)[0] == 0
    return obj[raw:raw + length]

spawn = code_rva + len(payload)
spawn_code = assemble('swamp_nine_spawn', spawn)
assert spawn_code[:4] == bytes.fromhex('48 83 ec 58')
payload.extend(spawn_code)
payload.extend(bytes(-len(payload) % 4))
unwind = code_rva + len(payload)
payload.extend(bytes.fromhex('01 04 01 00 04 a2 00 00'))
new_records.append((spawn, spawn + len(spawn_code), unwind))
site = 0x1e7214
old = current(site, 5)
assert old == b'\xe8' + pack('<i', 0x18cbd000 - site - 5)
edit(site, old, b'\xe8' + pack('<i', spawn - site - 5))

# HUD positions are recalculated by Player::UpdateHud as the roster fills.
hud = code_rva + len(payload)
hud_code = assemble('swamp_nine_hud', hud)
old = current(0x12a200, 6)
previous_hud = 0x12a205 + unpack_from('<i', old, 1)[0]
assert previous_hud == 0x18cbd350
dispatch = b'\x83\x3d' + pack('<i', 0x1d0e844 - hud - 7) + b'\x06'
dispatch += b'\x0f\x86' + pack('<i', previous_hud - hud - len(dispatch) - 6)
payload.extend(dispatch + hud_code)
edit(0x12a200, old, jump(0x12a200, hud) + b'\x90')

# Add the zero-initialized roster to the writable data section, then move
# the exception directory past it. Existing state RVAs stay stable.
assert roster >= data_rva + data_size
roster_offset = roster - data_rva
data = bytearray(source[data_raw:data_raw + data_size])
data.extend(bytes(roster_offset + 9 * 8 - len(data)))
table_offset = (len(data) + 15) & ~15
data.extend(bytes(table_offset - len(data)))
table = sorted(records + new_records)
assert all(a[1] <= b[0] for a, b in zip(table, table[1:]))
data.extend(b''.join(pack('<III', *entry) for entry in table))
code_padded = (len(payload) + 0x1ff) & ~0x1ff
data_padded = (len(data) + 0x1ff) & ~0x1ff
assert code_rva + code_padded <= data_rva, (hex(len(payload)), hex(data_rva - code_rva))
pack_into('<I', image, code_header + 8, len(payload))
pack_into('<I', image, code_header + 16, code_padded)
pack_into('<IIII', image, data_header + 8, len(data), data_rva, data_padded, code_raw + code_padded)
pack_into('<I', image, opt + 4, unpack_from('<I', image, opt + 4)[0] + code_padded - code_capacity)
pack_into('<I', image, opt + 8, unpack_from('<I', image, opt + 8)[0] + data_padded - data_capacity)
pack_into('<I', image, opt + 56, (data_rva + data_padded + 0xfff) & ~0xfff)
pack_into('<II', image, opt + 112 + 3 * 8, data_rva + table_offset, 12 * len(table))
image[code_raw:] = payload + bytes(code_padded - len(payload)) + data + bytes(data_padded - len(data))
(work / 'Shrek.swamp-nine-candidate.exe').write_bytes(image)
print('Candidate SHA256', sha256(image).hexdigest())

