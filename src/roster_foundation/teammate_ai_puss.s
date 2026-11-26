.intel_syntax noprefix
.text
.globl teammate_ai
.macro ref target
.long \target - (PAYLOAD_RVA + (. - teammate_ai) + 4)
.endm
.macro address_r10 target
.byte 0x4c,0x8d,0x15
ref \target
.endm
teammate_ai:
    cmp qword ptr [rdi + 0x490], 0
    jne done
    .byte 0x48,0x8b,0x05
    ref 0x1d0e980
    test rax, rax
    jz done
    address_r10 0x48cc90
    cmp [rax], r10
    jne done
    mov rax, [rdi]
    lea r10, [rip + class_table]
    xor r9d, r9d
find_class:
    .byte 0x48,0x8d,0x15
    ref 0
    movsxd rcx, dword ptr [r10 + r9*4]
    add rcx, rdx
    cmp rax, rcx
    je found
    inc r9d
    cmp r9d, 9
    jb find_class
    jmp done
found:
    address_r10 STATE_RVA
    mov eax, r9d
    shl eax, 6
    add r10, rax
    .byte 0x4c,0x8b,0x1d
    ref 0x1d10d00
    cmp [r10], rdi
    jne reset
    cmp r11, [r10 + 8]
    jae update
reset:
    mov [r10], rdi
    xor eax, eax
    mov [r10 + 16], rax
    mov [r10 + 24], rax
    mov [r10 + 32], rax
update:
    mov [r10 + 8], r11
    # Finish Shrek's native held-enemy throw before considering new moves.
    cmp r9d, 1
    jne target
    cmp dword ptr [rdi + 0x760], 1
    jne target
    mov byte ptr [rdi + 0x3a2], 1
    jmp done
target:
    movzx ecx, byte ptr [rdi + 0x1d]
    mov rdx, [rdi + 0x250]
    test rdx, rdx
    jz cancel_jump
    cmp byte ptr [rdx + 0x1c], 1
    jne cancel_jump
    test dword ptr [rdx + 0x364], 0x800
    jz cancel_jump
    cmp dword ptr [rdx + 0x18], 0
    jl cancel_jump
    cmp qword ptr [r10 + 24], 0
    je grounded
    cmp r11, [r10 + 24]
    jae cancel_jump
    cmp ecx, 9
    je airborne
    cmp ecx, 10
    jne done
airborne:
    cmp r9d, 6
    je fairy_orbs
    mov byte ptr [rdi + 0x3a2], 1
    jmp done
fairy_orbs:
    mov qword ptr [r10 + 24], 0
    mov rax, [rdi]
    mov rcx, rdi
    mov edx, 0x11
    xor r8d, r8d
    call qword ptr [rax + 0x58]
    jmp done
cancel_jump:
    mov qword ptr [r10 + 24], 0
    jmp done
grounded:
    cmp ecx, 1
    jb done
    cmp ecx, 3
    ja done
    cmp r11, [r10 + 16]
    jb done
    movups xmm0, [rdi + 0xc0]
    movups xmm1, [rdx + 0xc0]
    subps xmm0, xmm1
    dpps xmm0, xmm0, 0x71
    jp done
    lea rax, [rip + max_distance_squared]
    ucomiss xmm0, dword ptr [rax + r9*4]
    ja done
    cmp r9d, 0
    je close_stun
    cmp r9d, 1
    je close_stun
    cmp r9d, 6
    je fairy_choice
    cmp byte ptr [rdi + 0x3a2], 0
    je done
    cmp r9d, 5
    je alternate_special
    cmp r9d, 7
    je alternate_special
    jmp alternate_jump
close_stun:
    cmp byte ptr [rdx + 0x1d], 0xb
    jne basic
    mov eax, 0x3fa1eb85
    movd xmm1, eax
    ucomiss xmm0, xmm1
    jbe special
basic:
    cmp byte ptr [rdi + 0x3a2], 0
    je done
    jmp alternate_jump
fairy_choice:
    xor byte ptr [r10 + 32], 1
    jz fairy_float
    jmp jump_attack
alternate_special:
    xor byte ptr [r10 + 32], 1
    jz special
    jmp jump_attack
alternate_jump:
    xor byte ptr [r10 + 32], 1
    jz regular
jump_attack:
    mov byte ptr [rdi + 0x3a2], 0
    mov byte ptr [rdi + 0x3a0], 1
    lea rax, [r11 + 12288]
    mov [r10 + 24], rax
    jmp cooldown
fairy_float:
    mov byte ptr [rdi + 0x3a3], 0
    mov byte ptr [rdi + 0x3a6], 1
    jmp cooldown
special:
    mov byte ptr [rdi + 0x3a2], 0
    mov byte ptr [rdi + 0x3a5], 1
cooldown:
    lea rax, [r11 + 61440]
    mov [r10 + 16], rax
    jmp done
regular:
    lea rax, [r11 + 15360]
    mov [r10 + 16], rax
done:
    mov rax, [rdi]
    mov rcx, rdi
    .byte 0xe9
    ref 0x1e4eae
.p2align 2
class_table:
.long 0x462918,0x4cc398,0x470640,0x4738e8,0x486ff0,0x454638,0x46fb48,0x474458,0x496468
max_distance_squared:
.float 4,4,4,16,4,4,36,9,4

