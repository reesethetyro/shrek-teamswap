# A homing hit uses the chicken's flinch animation with a horizontal
# impulse. Bypass the damage message and its scripted side effects.
check_friar_fairy_hit:
    live_lea 0x05, 0x1d0e7f0
    mov eax, [rax + 8]
    cmp eax, 6
    ja native_projectile_damage
    mov r10d, 0x4a # Shrek's Swamp (1), Far Far Away (3), Jack and Jill's Farm (6).
    bt r10d, eax
    jnc native_projectile_damage
    live_lea 0x05, 0x46e928 # FairyJumpEffect vtable.
    cmp [rbx], rax
    jne native_projectile_damage
    test rcx, rcx
    jz native_projectile_damage
    live_lea 0x05, 0x45f110 # Chicken vtable.
    cmp [rcx], rax
    jne native_projectile_damage
    cmp byte ptr [rcx + 0x1d], 3
    jbe flinch_chicken
    cmp byte ptr [rcx + 0x1d], 6
    jne consume_homing_shot
flinch_chicken:
    jmp play_grounded_flinch
consume_homing_shot:
    mov eax, 1
    ret
native_projectile_damage:
    .byte 0xe9
    rip_ref 0x090020

play_grounded_flinch:
    sub rsp, 0x38
    # Native mode 5 normalizes the impulse. A fixed horizontal unit
    # vector keeps that calculation defined even for vertical impacts.
    mov dword ptr [rcx + 0x320], 0
    mov dword ptr [rcx + 0x324], 0
    mov dword ptr [rcx + 0x328], 0x3f800000
    mov dword ptr [rcx + 0x32c], 0
    mov eax, [rcx + 0x390]
    mov [rcx + 0x558], eax
    mov rax, [rcx]
    mov edx, 5
    xor r8d, r8d
    call qword ptr [rax + 0x58]
    mov eax, 1
    add rsp, 0x38
    ret
play_grounded_flinch_end:
