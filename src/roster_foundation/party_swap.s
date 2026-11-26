.intel_syntax noprefix
.text
.globl party_swap
.set payload_rva, 0x44b9a0
.macro target rva
    .long \rva - (payload_rva + (. - party_swap) + 4)
.endm
party_swap:
    .byte 0x48, 0x8d, 0x05
    target 0x1d0e7f0
    cmp dword ptr [rax + 8], 1
    jne gingy
    mov rax, [rax + 0x190]
    test rax, rax
    je gingy
    .byte 0x4c, 0x8d, 0x15
    target 0x48cc90
    cmp [rax], r10
    jne gingy
    .byte 0xe9
    target 0x1e7116
gingy:
    .byte 0x48, 0x8d, 0x1d
    target 0x5ad460
    .byte 0xe9
    target 0x1e70fa
