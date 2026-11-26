.intel_syntax noprefix
.text
.globl position_six_portraits
.set payload_rva, 0x18cbd350
.macro reference target
    .long \target - (payload_rva + (. - position_six_portraits) + 4)
.endm
position_six_portraits:
    .byte 0x48, 0x8d, 0x05
    reference 0x1d0e7f0
    cmp dword ptr [rax + 0x54], 6
    je six
    cmp dword ptr [rax + 0x54], 5
    jne original
    mov eax, dword ptr [rdx + 0x604]
    lea ecx, [rax - 1]
    cmp ecx, 4
    ja original
    sub eax, 3
    cvtsi2ss xmm0, eax
    .byte 0xf3, 0x0f, 0x59, 0x05
    reference 0x466264
    ret
six:
    mov eax, dword ptr [rdx + 0x604]
    lea ecx, [rax - 1]
    cmp ecx, 5
    ja original
    lea eax, [rax + rax - 7]
    cvtsi2ss xmm0, eax
    mov eax, 0x42200000
    movd xmm1, eax
    mulss xmm0, xmm1
    ret
original:
    mov eax, dword ptr [rdx + 0x604]
    .byte 0xe9
    reference 0x12a206


