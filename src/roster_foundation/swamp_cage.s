.intel_syntax noprefix
.text
cage_message:
    .byte 0x83, 0x3d
    .long 0x1d0e7f8 - (PAYLOAD_RVA + (. - cage_message) + 5)
    .byte 1
    jne original
    mov r9, [rcx + 0x10]
    .byte 0x4c, 0x8d, 0x15
    .long fairy_handler - (PAYLOAD_RVA + (. - cage_message) + 4)
    cmp r9, r10
    jne grounded
    cmp qword ptr [r8 + 8], 0
    je original
    xor eax, eax
    ret
grounded:
    .byte 0x4c, 0x8d, 0x15
    .long red_handler - (PAYLOAD_RVA + (. - cage_message) + 4)
    cmp r9, r10
    je extra
    .byte 0x4c, 0x8d, 0x15
    .long puss_handler - (PAYLOAD_RVA + (. - cage_message) + 4)
    cmp r9, r10
    je extra
    .byte 0x4c, 0x8d, 0x15
    .long wolf_handler - (PAYLOAD_RVA + (. - cage_message) + 4)
    cmp r9, r10
    je extra
    .byte 0x4c, 0x8d, 0x15
    .long duo_handler - (PAYLOAD_RVA + (. - cage_message) + 4)
    cmp r9, r10
    je extra
original:
    jmp rax
extra:
    jmp extra_message

extra_message:
    sub rsp, 0x48
    mov [rsp + 0x30], rcx
    mov r10, [r8 + 8]
    mov [rsp + 0x38], r10
    call rax
    mov [rsp + 0x40], rax
    mov rcx, [rsp + 0x30]
    cmp byte ptr [rcx + 0x1d], 0x17
    jne finish
    cmp qword ptr [rsp + 0x38], 0
    je release
    # The trapped mode freezes locomotion but does not choose a clip.
    mov rdx, [rcx + 0x4e0]
    test rdx, rdx
    jz finish
    # Use the held downed pose directly, skipping the fall-to-birds event.
    mov edx, [rdx + 4]
    test edx, edx
    js finish
    # Match the native downed-loop transition without rewriting frame state.
    mov rax, [rcx]
    xor r8d, r8d
    mov r9d, -1
    mov byte ptr [rsp + 0x20], 1
    mov byte ptr [rsp + 0x28], 0
    call qword ptr [rax + 0x50]
    jmp finish
release:
    # Recover through the base player's trapped-to-get-up transition.
    mov qword ptr [rcx + 0x6e8], 0
    mov edx, 3
    xor r8d, r8d
    .byte 0xe8
    .long init_player_mode - (PAYLOAD_RVA + (. - cage_message) + 4)
finish:
    mov rax, [rsp + 0x40]
    add rsp, 0x48
    ret
end_message:

.set fairy_handler, 0x107d40
.set red_handler, 0x196db0
.set puss_handler, 0x1cd1e0
.set wolf_handler, 0x06b1f0
.set duo_handler, 0x120ee0
.set init_player_mode, 0x1e0c20
.long extra_message - cage_message
.long end_message - cage_message

