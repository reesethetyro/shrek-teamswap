# An absent pickup owner is represented by a token only between the
# pickup's owner lookup and its AddAmmo call. No game object uses the token.
lookup_pickup_owner:
    sub rsp, 0x38
    mov [rsp + 0x20], rdx
    .byte 0xe8
    rip_ref 0x22c560
    test rax, rax
    jnz finish_pickup_lookup
    call bench_pickups_allowed
    test al, al
    jz missing_pickup_owner
    mov rdx, [rsp + 0x20]
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x109000
    cmp rdx, rax
    mov eax, 1
    je finish_pickup_lookup
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x11ce50
    cmp rdx, rax
    mov eax, 2
    je finish_pickup_lookup
    mov eax, 3
    jmp finish_pickup_lookup
missing_pickup_owner:
    xor eax, eax
finish_pickup_lookup:
    add rsp, 0x38
    ret
lookup_pickup_end:

show_benched_pickup:
    sub rsp, 0x38
    call lookup_pickup_owner
    test rax, rax
    jz finish_show_pickup
    cmp rax, 3
    ja finish_show_pickup
    # Vanilla only unhides these pickups at zero ammo. For a benched
    # owner, use their banked ammo and ignore the saved-ammo marker.
    .byte 0x48, 0x8d, 0x15
    rip_ref PICKUP_BANK_RVA
    test dword ptr [rdx + rax * 4 - 4], 0x7fffffff
    jnz keep_benched_pickup_hidden
    mov rcx, rbx
    .byte 0xe8
    rip_ref 0x1d3040
keep_benched_pickup_hidden:
    xor eax, eax
finish_show_pickup:
    add rsp, 0x38
    ret
show_pickup_end:

bench_pickups_allowed:
    sub rsp, 0x38
    .byte 0x48, 0x8d, 0x0d
    rip_ref 0x1d0e7f0
    mov eax, [rcx + 8]
    cmp eax, 15
    ja bench_pickups_disabled
    mov edx, 0xe8ee
    bt edx, eax
    jnc bench_pickups_disabled
    # hasOnlineLobby returns a boolean. Invert only its low byte.
    .byte 0xe8
    rip_ref 0x231b10
    movzx eax, al
    xor al, 1
    jmp finish_bench_pickups
bench_pickups_disabled:
    xor eax, eax
finish_bench_pickups:
    add rsp, 0x38
    ret
bench_pickups_end:

credit_pickup_owner:
    cmp rcx, 3
    ja add_native_ammo
    .byte 0x48, 0x8d, 0x05
    rip_ref PICKUP_BANK_RVA
    add [rax + rcx * 4 - 4], edx
    ret
add_native_ammo:
    .byte 0xe9
    rip_ref 0x1ddc10

.macro clear_pickup_bank
    .byte 0x48, 0x8d, 0x05
    rip_ref PICKUP_BANK_RVA
    mov qword ptr [rax], 0
    mov dword ptr [rax + 8], 0
.endm

# Entries contain earned ammo alone until the owner has been active.
# A set sign bit means the entry includes their previous live ammo.
remember_pickup_ammo:
    mov edx, [rcx + 0x600]
    cmp edx, 9
    ja finish_ammo_bank
    lea rax, [rip + pickup_owner_indices]
    movsx edx, byte ptr [rax + rdx]
    test edx, edx
    js finish_ammo_bank
    .byte 0x48, 0x8d, 0x05
    rip_ref PICKUP_BANK_RVA
    mov r9d, [rcx + 0x5e4]
    bts r9d, 31
    mov [rax + rdx * 4], r9d
    ret

restore_pickup_ammo:
    mov edx, [rcx + 0x600]
    cmp edx, 9
    ja finish_ammo_bank
    lea rax, [rip + pickup_owner_indices]
    movsx edx, byte ptr [rax + rdx]
    test edx, edx
    js finish_ammo_bank
    .byte 0x48, 0x8d, 0x05
    rip_ref PICKUP_BANK_RVA
    mov r9d, [rax + rdx * 4]
    test r9d, r9d
    jz finish_ammo_bank
    mov dword ptr [rax + rdx * 4], 0
    btr r9d, 31
    jnc transfer_pickup_ammo
    mov dword ptr [rcx + 0x5e4], 0
transfer_pickup_ammo:
    mov edx, r9d
    jmp add_native_ammo
finish_ammo_bank:
    ret
pickup_owner_indices:
    .byte -1, -1, 1, -1, 2, -1, -1, -1, 0, -1
