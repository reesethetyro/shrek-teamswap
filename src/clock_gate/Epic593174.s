.intel_syntax noprefix
.text
.globl clock_gate
clock_gate:
sub rsp,0x28
.byte 0x48,0x8d,0x0d
.long 30484928-(START+(. - clock_gate)+4)
cmp dword ptr [rcx+8],24
jne normal
mov rdx,[rsp+0x50]
.byte 0x48,0x8d,0x05
.long 1086352-(START+(. - clock_gate)+4)
cmp rdx,rax
# Final ammo stage: permit all three native personal-ammo owner lookups.
nop
nop
.byte 0xe8
.long 2317744-(START+(. - clock_gate)+4)
movzx eax,al
xor al,1
jmp done
normal:
.byte 0xe8
.long 416662641-(START+(. - clock_gate)+4)
jmp done
denied:
xor eax,eax
done:
add rsp,0x28
ret
