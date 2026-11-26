# The fallen-tree crossings use TreeEnt or Bridge actors. The Bridge trees
# also have separately authored Destructible collision objects.
.macro check_forest_obstacle accepted, other
    test rcx, rcx
    jz \other
    .byte 0x83, 0x3d
    .long 0x1d0e7f8 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 2
    jne \other
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x16fdf0
    cmp [rcx + 0x10], r11
    je \accepted
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x16c150
    cmp [rcx + 0x10], r11
    je \accepted
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x1f4260
    cmp [rcx + 0x10], r11
    jne .Lcheck_destructible\@
    movzx r11d, word ptr [rcx + 0x22]
    sub r11d, 228
    cmp r11d, 2
    ja \other
    jmp \accepted
.Lcheck_destructible\@:
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x0b95b0
    cmp [rcx + 0x10], r11
    jne \other
    cmp word ptr [rcx + 0x22], 1813
    je \accepted
    movzx r11d, word ptr [rcx + 0x22]
    sub r11d, 838
    cmp r11d, 2
    ja \other
.endm

wolf_obstacle_collision:
    # The caller already tested the native collision channels.
    jnz wolf_accept_collision
    test r8, r8
    jz wolf_reject_collision
    mov eax, 0xd0
    mov ecx, 0xc8
    cmp byte ptr [rdi + 0xdc], 0
    cmovne eax, ecx
    mov rcx, [rdi + rax]
    check_forest_obstacle wolf_accept_collision, wolf_reject_collision
wolf_accept_collision:
    .byte 0xe9
    rip_ref 0x06bf35
wolf_reject_collision:
    .byte 0xe9
    rip_ref 0x06c01d

wolf_obstacle_damage:
    check_forest_obstacle copy_obstacle_damage, wolf_native_damage
copy_obstacle_damage:
    # SendDamage reads five aligned vectors. Only this hit receives Burro Blast's
    # obstacle flag, so a later hit from the same breath uses Wolf's native flags.
    sub rsp, 0x78
    movaps xmm0, [rdx]
    movaps xmm1, [rdx + 0x10]
    movaps xmm2, [rdx + 0x20]
    movaps xmm3, [rdx + 0x30]
    movaps xmm4, [rdx + 0x40]
    lea rdx, [rsp + 0x20]
    movaps [rdx], xmm0
    movaps [rdx + 0x10], xmm1
    movaps [rdx + 0x20], xmm2
    movaps [rdx + 0x30], xmm3
    movaps [rdx + 0x40], xmm4
    or byte ptr [rdx + 0x14], 8
    .byte 0xe8
    rip_ref 0x090020
    add rsp, 0x78
    ret
copy_obstacle_damage_end:
wolf_native_damage:
    .byte 0xe9
    rip_ref 0x090020
