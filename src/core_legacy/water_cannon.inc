# An accepted Boom mount normally waits for an animation event before
# entering the mode that reads aim, fire, and dismount input.
finish_water_cannon_mount:
    test rcx, rcx
    jz native_mount_animation
    cmp dword ptr [rdi + 0x6b0], 3
    jne native_mount_animation
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x29e730 # WaterCannonBoom_mh
    cmp [rcx + 0x10], r10
    jne native_mount_animation
    mov eax, [rdi + 0x600]
    cmp eax, 8
    ja native_mount_animation
    mov r10d, 0x113 # Shrek, Donkey, Lil' Red, Fiona.
    bt r10d, eax
    jnc native_mount_animation
    mov rax, [rdi]
    mov rcx, rdi
    mov edx, 20
    xor r8d, r8d
    .byte 0xe9
    rip_ref 0x1e58d8 # UpdateMode's virtual InitMode call and epilogue.

native_mount_animation:
    mov r10, [rdi + 0x4f8]
    .byte 0xe9
    rip_ref 0x1e541b

# Treat Fairy like an out-of-range operator so any old prompt and
# interaction offer are released by the same path used on walking away.
filter_fairy_water_cannon_offer:
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x107d40 # Fairy_mh
    cmp [rbx + 0x10], r10
    je clear_fairy_water_cannon_offer
    cmp [rdi + 0x1f8], r13b
    .byte 0xe9
    rip_ref 0x29db19
clear_fairy_water_cannon_offer:
    .byte 0xe9
    rip_ref 0x29dbc7

# Reject a mount already queued before the offer was cleared.
reject_fairy_water_cannon_mount:
    test rsi, rsi
    jz native_water_cannon_mount
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x107d40 # Fairy_mh
    cmp [rsi + 0x10], r10
    je deny_fairy_water_cannon_mount
native_water_cannon_mount:
    cmp byte ptr [rbx + 0x1f8], 0
    .byte 0xe9
    rip_ref 0x29f42f
deny_fairy_water_cannon_mount:
    .byte 0xe9
    rip_ref 0x29f484
