# Only a Dragon created by Swap Team gets the free-flight animation table.
.macro check_free_dragon other, actor=rcx, scratch=rax
    lea \scratch, [rip + dragon_walk_animations]
    cmp [\actor + 0x520], \scratch
    jne \other
    cmp qword ptr [\actor + 0x750], 0
    jne \other
.endm

init_swamp_dragon:
    cmp edx, 0x18
    jne check_free_dragon_mode
    .byte 0x80, 0x3d
    .long STAGING_RVA - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 1
    jne check_free_dragon_mode
    lea rax, [rip + dragon_walk_animations]
    mov [rcx + 0x520], rax
    mov dword ptr [rcx + 0x528], 3
    mov dword ptr [rcx + 0x368], 0x3cd55555
    # Use the regular Player spawn mode and Fairy's collision floor clearance.
    # The rail actor otherwise enters gameplay before a floor is established.
    xor edx, edx
    or dword ptr [rcx + 0x364], 0x20
    mov dword ptr [rcx + 0x224], 0x3e100000
    jmp init_free_dragon_mode
check_free_dragon_mode:
    check_free_dragon init_rail_dragon
init_free_dragon_mode:
    .byte 0xe9
    rip_ref 0x1e0c20
init_rail_dragon:
    mov [rsp + 0x10], rsi
    .byte 0xe9
    rip_ref 0x0b3515

move_swamp_dragon:
    check_free_dragon move_rail_dragon
    .byte 0xe9
    rip_ref 0x08bf00
move_rail_dragon:
    push rbx
    sub rsp, 0x30
    .byte 0xe9
    rip_ref 0x0b33b6

update_swamp_dragon:
    check_free_dragon update_rail_dragon
update_free_dragon_frame:
    sub rsp, 0x38
    mov [rsp + 0x30], rcx
    cmp byte ptr [rcx + 0x1d], 0x0e
    je update_free_dragon_mode
    movss xmm1, [rip + dragon_hover_height]
    .byte 0xe8
    rip_ref 0x1e0860
    mov rcx, [rsp + 0x30]
update_free_dragon_mode:
    .byte 0xe8
    rip_ref 0x1e51b0
    mov rcx, [rsp + 0x30]
    # Let the normal hit states finish before restoring the locomotion clip.
    movzx eax, byte ptr [rcx + 0x1d]
    cmp al, 1
    jbe restore_dragon_flight
    cmp al, 3
    jne finish_free_dragon_frame
restore_dragon_flight:
    cmp dword ptr [rcx + 0x2a4], 5
    jne finish_free_dragon_frame
    mov qword ptr [rcx + 0x2d8], 0
    mov edx, 1
    xor r8d, r8d
    xor r9d, r9d
    mov byte ptr [rsp + 0x20], 1
    mov byte ptr [rsp + 0x28], 0
    .byte 0xe8
    rip_ref 0x08f270
finish_free_dragon_frame:
    add rsp, 0x38
    ret
update_free_dragon_frame_end:
update_rail_dragon:
    push rbx
    sub rsp, 0x50
    .byte 0xe9
    rip_ref 0x0b4a36

frame_swamp_dragon:
    check_free_dragon frame_rail_dragon
    mov byte ptr [rcx + 0x3c0], 0
frame_rail_dragon:
    .byte 0xe9
    rip_ref 0x1e4b70

# Normal party hit handling for the free-roaming actor.
attack_swamp_dragon:
    check_free_dragon attack_rail_dragon
    .byte 0xe9
    rip_ref 0x1de260
attack_rail_dragon:
    mov rax, rsp
    mov [rax + 0x10], rbx
    .byte 0xe9
    rip_ref 0x0b30e7

dragon_spawn_lift:
    .float 0.5
dragon_hover_height:
    .float 1.5

photo_swamp_dragon_portrait:
    .byte 0x48, 0x8d, 0x0d
    rip_ref 0x463220
    cmp [rax], rcx
    jne native_photo_portrait
    check_free_dragon native_photo_portrait, rax, rcx
    mov ecx, 1
    jmp resolve_photo_portrait
native_photo_portrait:
    movsxd rcx, dword ptr [rax + 0x600]
resolve_photo_portrait:
    .byte 0xe9
    rip_ref 0x26f484

# KillMeNow checks the mesh contact. Require that contact to reach the
# free-roaming Dragon's body center before replaying the native hazard check.
check_dragon_hazard:
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x463220
    cmp [rsi], rax
    jne native_hazard_contact
    check_free_dragon native_hazard_contact, rsi

    movss xmm0, [rdi + 0x80]
    subss xmm0, [rsi + 0xc0]
    mulss xmm0, xmm0
    movss xmm1, [rdi + 0x88]
    subss xmm1, [rsi + 0xc8]
    mulss xmm1, xmm1
    addss xmm0, xmm1
    comiss xmm0, [rip + dragon_hazard_radius_squared]
    ja ignore_dragon_hazard_contact
    movss xmm0, [rdi + 0x84]
    subss xmm0, [rsi + 0xc4]
    comiss xmm0, [rip + dragon_hazard_clearance]
    ja ignore_dragon_hazard_contact

native_hazard_contact:
    cmp byte ptr [rbx + 0x198], 0
    .byte 0xe9
    rip_ref 0x201a7b
ignore_dragon_hazard_contact:
    .byte 0xe9
    rip_ref 0x201af9

dragon_hazard_radius_squared:
    .float 0.0225
dragon_hazard_clearance:
    .float 0.25

# Select the engine's direct playback path for the Swamp Dragon. Keep the
# caller's clip, playback rate, and flags, including finite hit reactions.
transition_swamp_dragon_animation:
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x463220
    cmp [rcx], rax
    jne native_animation_transition
    check_free_dragon native_animation_transition
    xor r8d, r8d
native_animation_transition:
    mov [rsp + 8], rbx
    .byte 0xe9
    rip_ref 0x08f275

# Fairy's movement classes, using Dragon's flight loop for each.
dragon_walk_animations:
    .long 1, 0, 0, 2
    .long 1, 0, 0, 1
    .long 1, 0, 0, 0
    .long -1, 0, 0, 0
