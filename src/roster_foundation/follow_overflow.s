.intel_syntax noprefix
.text
.globl follow_overflow
follow_overflow:
    movsxd rax, dword ptr [rdi + 0x324]
    cmp eax, 4
    jae overflow
    .byte 0xe9
    .long 0x5e576 - (PAYLOAD_RVA + (. - follow_overflow) + 4)
overflow:
    .byte 0x83, 0x3d
    .long 0x1d0e844 - (PAYLOAD_RVA + (. - follow_overflow) + 5)
    .byte 6
    jne done
    cmp dword ptr [rdi + 0x320], 1
    jne done
    # Slot zero of the human-player position table is the active player.
    movups xmm0, [rdi + 0xa0]
    movaps [rsp + 0x60], xmm0
    lea rcx, [rsp + 0x60]
    lea rdx, [rbx + 0x240]
    mov r8b, 1
    .byte 0xe8
    .long 0x5a440 - (PAYLOAD_RVA + (. - follow_overflow) + 4)
    lea rdx, [rsp + 0x50]
    mov rcx, rbx
    .byte 0xe8
    .long 0x2aeef0 - (PAYLOAD_RVA + (. - follow_overflow) + 4)
    movaps xmm0, [rsp + 0x60]
    movups xmm1, [rax]
    subps xmm1, xmm0
    mulps xmm1, xmm1
    movaps xmm2, xmm1
    shufps xmm2, xmm2, 0x55
    addss xmm2, xmm1
    shufps xmm1, xmm1, 0xaa
    addss xmm2, xmm1
    sqrtss xmm2, xmm2
    # Stop at a comfortable distance rather than walking into the leader.
    mov ecx, 0x3f800000
    movd xmm3, ecx
    ucomiss xmm2, xmm3
    ja set_target
    movups xmm0, [rax]
    xorps xmm2, xmm2
set_target:
    movups [rbx + 0x1e0], xmm0
    movups [rbx + 0x1d0], xmm0
    movss [rbx + 0x230], xmm2
    and dword ptr [rbx + 0x200], -5
    cmp dword ptr [rbx + 0x248], 0
    jne done
    mov dword ptr [rbx + 0x248], 1
done:
    .byte 0xe9
    .long 0x5e620 - (PAYLOAD_RVA + (. - follow_overflow) + 4)

