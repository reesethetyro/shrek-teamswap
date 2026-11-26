.intel_syntax noprefix
.text
.macro ref target, tail=0
.long \target - (PAYLOAD_RVA + (. - START) + 4 + \tail)
.endm
.macro jump target
.byte 0xe9
ref \target
.endm
.macro invoke target
.byte 0xe8
ref \target
.endm
.macro address_rax target
.byte 0x48,0x8d,0x05
ref \target
.endm
.macro six_party no
.byte 0x83,0x3d
ref 0x1d0e7f8,1
.byte 14
jne \no
.byte 0x83,0x3d
ref 0x1d0e844,1
.byte 6
jne \no
.endm
.macro compare_boarded
    mov r10d, 0xf
    six_party 9f
    mov r10d, 0x3f
9:
    cmp eax, r10d
.endm
START:
.if KIND == 0
    six_party original
    address_rax 0x196db0
    cmp rcx, rax
    je extra
    address_rax 0x107d40
    cmp rcx, rax
    jne original
extra:
    jump 0x1a6efd
original:
    lea rcx, [rbp - 0x49]
    invoke 0x089a80
    jump 0x1a6e5c
.elseif KIND == 1
    mov eax, [rdi + 0x654]
    compare_boarded
    jump 0x1a6f06
.elseif KIND == 2
    address_rax 0x196db0
    cmp rcx, rax
    je red
    address_rax 0x107d40
    cmp rcx, rax
    je fairy
    mov eax, [rdi + 0x654]
    cmp rcx, r9
    jne native
    jump 0x1a6f45
native:
    jump 0x1a6f4c
red:
    mov esi, 0x10
    mov edx, 2
    jmp check
fairy:
    mov esi, 0x20
    mov edx, 5
check:
    mov eax, [rdi + 0x654]
    jump 0x1a6f6d
.elseif KIND == 3
    mov eax, [rdi + 0x654]
    compare_boarded
    jump 0x1a6fe0
.elseif KIND == 4
    # Run the original fourth rider's message first.
    mov r9, [rax + 0x10]
    test r9, r9
    jz extras
    lea r8, [rsp + 0x40]
    mov edx, 0xb
    mov rcx, rax
    call r9
extras:
    six_party finish
    mov edx, 2
    mov rcx, rbx
    invoke 0xb9e90
    test rax, rax
    jz fairy_transfer
    mov [rsp + 0x50], rax
    mov edx, 4
    .byte 0x48,0x8d,0x0d
    ref 0x1d0e7f0
    invoke 0x22c8e0
    test rax, rax
    jz fairy_transfer
    mov r9, [rax + 0x10]
    test r9, r9
    jz fairy_transfer
    lea r8, [rsp + 0x40]
    mov edx, 0xb
    mov rcx, rax
    call r9
fairy_transfer:
    mov edx, 5
    mov rcx, rbx
    invoke 0xb9e90
    test rax, rax
    jz finish
    mov [rsp + 0x50], rax
    mov edx, 5
    .byte 0x48,0x8d,0x0d
    ref 0x1d0e7f0
    invoke 0x22c8e0
    test rax, rax
    jz finish
    mov r9, [rax + 0x10]
    test r9, r9
    jz finish
    lea r8, [rsp + 0x40]
    mov edx, 0xb
    mov rcx, rax
    call r9
finish:
    jump 0x1a7b5b
.endif
