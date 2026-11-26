# Record the world actor ID of a player before a swap removes it.
.set CINEMA_ACTORS_RVA, CINEMA_IDS_RVA + 40
.set CINEMA_VISIBLE_RVA, CINEMA_ACTORS_RVA + 80
.set CINEMA_SLOT_ORIGINS_RVA, DRAFT_RVA + 20

.macro clear_cinema_state
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_IDS_RVA
    .set .Lclear_offset\@, 0
    .rept 15
    mov qword ptr [rax + (.Lclear_offset\@)], 0
    .set .Lclear_offset\@, .Lclear_offset\@ + 8
    .endr
    mov dword ptr [rax + 120], 0
    .byte 0x48, 0x8d, 0x05
    rip_ref FIONA_MODEL_RVA
    mov qword ptr [rax], 0
    mov qword ptr [rax + 8], 0
    mov dword ptr [rax + 64], 0
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_SLOT_ORIGINS_RVA
    mov dword ptr [rax], 0
.endm

remember_cinema_actor:
    sub rsp, 0x38
    mov rax, [rcx + 0x10]
    test rax, rax
    jz finish_remember_actor
    mov edx, [rcx + 0x600]
    cmp edx, 9
    ja finish_remember_actor
    cmp edx, 8
    jne remember_actor_identity
    .byte 0x4c, 0x8d, 0x15
    rip_ref FIONA_MODEL_RVA
    cmp qword ptr [r10], 0
    jne remember_actor_identity
    mov r11, [rcx + 0x80]
    test r11, r11
    jz remember_actor_identity
    mov [r10], r11
    movzx r11d, word ptr [rcx + 0x22]
    mov [r10 + 8], r11d
remember_actor_identity:
    mov [rsp + 0x20], edx
    movzx edx, byte ptr [rcx + 0x608]
    mov [rsp + 0x24], edx
    mov edx, 0x26
    xor r8d, r8d
    call rax
    test eax, eax
    jle finish_remember_actor
    mov edx, [rsp + 0x20]
    mov ecx, [rsp + 0x24]
    cmp ecx, 4
    jae remember_actor_id
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_SLOT_ORIGINS_RVA
    cmp byte ptr [r10 + rcx], 0
    jne remember_actor_id
    lea r9d, [rdx + 1]
    mov [r10 + rcx], r9b
remember_actor_id:
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_IDS_RVA
    cmp dword ptr [r10 + rdx * 4], 0
    jne finish_remember_actor
    mov [r10 + rdx * 4], eax
finish_remember_actor:
    add rsp, 0x38
    ret
remember_cinema_end:

# Let native lookup choose first. When a swap retired its authored actor,
# create a scene-only replacement and hand it to the native animation setup.
resolve_cinema_actor:
    sub rsp, 0x38
    mov [rsp + 0x30], r9d
    .byte 0xe8
    rip_ref 0x244830
    test rax, rax
    jz resolve_missing_actor
    cmp dword ptr [rsp + 0x30], 5002
    jne finish_resolve_actor
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x2198c0
    cmp [rax + 0x10], r10
    jne finish_resolve_actor
    cmp dword ptr [rax + 0x748], 0
    jne finish_resolve_actor
    .byte 0x48, 0x8d, 0x0d
    rip_ref 0x1d0e7f0
    locate_rdx 0x109000
    .byte 0xe8
    rip_ref 0x22c560
    test rax, rax
    jnz finish_resolve_actor
resolve_missing_actor:
    mov r9d, [rsp + 0x30]
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_IDS_RVA
    xor edx, edx
find_cinema_actor:
    cmp [r10 + rdx * 4], r9d
    je found_cinema_actor
    inc edx
    cmp edx, 10
    jb find_cinema_actor
    jmp finish_resolve_actor
found_cinema_actor:
    mov [rsp + 0x30], edx
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_ACTORS_RVA
    mov rax, [r10 + rdx * 8]
    test rax, rax
    jnz finish_resolve_actor
    cmp edx, 8
    jne resolve_cinema_file
    .byte 0xe8
    rip_ref 0x109010
    mov rcx, rax
    jmp load_cinema_file
resolve_cinema_file:
    lea r11, [rip + character_files]
    mov eax, [r11 + rdx * 8]
    test eax, eax
    jz finish_resolve_actor
    .byte 0x48, 0x8d, 0x0d
    rip_ref 0
    add rcx, rax
load_cinema_file:
    mov [rsp + 0x28], rcx
    .byte 0xe8
    rip_ref 0x08ebe0
    test al, al
    jz failed_cinema_spawn
    mov rcx, [rsp + 0x28]
    mov rdx, [rcx + 0x28]
    test rdx, rdx
    jz failed_cinema_spawn
    mov eax, [rsp + 0x30]
    lea r11, [rip + character_files]
    mov r9d, [r11 + rax * 8 + 4]
    .byte 0x48, 0x8d, 0x05
    rip_ref 0
    add r9, rax
    .byte 0x48, 0x8d, 0x0d
    rip_ref 0x1d10ce0
    xor r8d, r8d
    cmp dword ptr [rsp + 0x30], 8
    jne cinema_model_ready
    .byte 0x48, 0x8d, 0x05
    rip_ref FIONA_MODEL_RVA
    cmp qword ptr [rax], 0
    je cinema_model_ready
    mov rdx, [rax]
    movzx r8d, word ptr [rax + 8]
cinema_model_ready:
    mov dword ptr [rsp + 0x20], 0xc
    mov word ptr [rsp + 0x28], 0xffff
    .byte 0x48, 0x8d, 0x05
    rip_ref STAGING_RVA
    mov byte ptr [rax], 1
    .byte 0xe8
    rip_ref 0x241dd0
    .byte 0x4c, 0x8d, 0x15
    rip_ref STAGING_RVA
    mov byte ptr [r10], 0
    test rax, rax
    jz finish_resolve_actor
    mov edx, [rsp + 0x30]
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_ACTORS_RVA
    mov [r10 + rdx * 8], rax
    inc word ptr [rax + 0x1e]
    jmp finish_resolve_actor
failed_cinema_spawn:
    xor eax, eax
finish_resolve_actor:
    add rsp, 0x38
    ret
resolve_cinema_end:

load_cinema_cast:
    sub rsp, 0x38
    .byte 0xe8
    rip_ref 0x0a9350
    .byte 0x48, 0x8d, 0x05
    rip_ref SWAPPED_CHAPTER_RVA
    .byte 0x48, 0x8d, 0x15
    rip_ref 0x1d0e7f8
    mov ecx, [rdx]
    cmp [rax], ecx
    jne finish_load_cast
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_VISIBLE_RVA
    mov dword ptr [rax], 1
finish_load_cast:
    add rsp, 0x38
    ret
load_cinema_end:

retire_cinema_cast:
    sub rsp, 0x38
    # Swamp's singing outro ends the level. Do not reveal the playable Red
    # during its exit fade while the story Red is still on screen.
    cmp dword ptr [rcx + 8], 1
    jne retire_cinema_begin
    test rdx, rdx
    jz retire_cinema_begin
    cmp dword ptr [rdx + 0x1b4], 6
    jne retire_cinema_begin
    .byte 0x48, 0x8d, 0x05
    rip_ref FIONA_MODEL_RVA
    mov dword ptr [rax + 64], 1
retire_cinema_begin:
    .byte 0xe8
    rip_ref 0x22c290
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_VISIBLE_RVA
    mov dword ptr [rax], 0
    mov dword ptr [rsp + 0x20], 0
retire_cinema_actor:
    mov edx, [rsp + 0x20]
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_ACTORS_RVA
    xor ecx, ecx
    xchg [rax + rdx * 8], rcx
    test rcx, rcx
    jz next_cinema_actor
    mov [rsp + 0x28], rcx
    .byte 0xe8
    rip_ref 0x2adee0
    mov rcx, [rsp + 0x28]
    .byte 0xe8
    rip_ref 0x2adb00
next_cinema_actor:
    inc dword ptr [rsp + 0x20]
    cmp dword ptr [rsp + 0x20], 10
    jb retire_cinema_actor
    add rsp, 0x38
    ret
retire_cinema_end:

# CleanUp still has every actor track attached at this point, including
# the native actors whose positions should take precedence over proxies.
finish_cinema_positions:
    call move_swapped_cinema_players
    mov rcx, [rdi + 0x158]
    .byte 0xe9
    rip_ref 0x0a8f0f

move_swapped_cinema_players:
    sub rsp, 0x38
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_VISIBLE_RVA
    cmp dword ptr [rax], 0
    je finish_cinema_positions_return
    mov dword ptr [rsp + 0x20], 0
cinema_position_slot:
    mov eax, [rsp + 0x20]
    locate_rdx CINEMA_SLOT_ORIGINS_RVA
    movzx ecx, byte ptr [rdx + rax]
    test ecx, ecx
    jz next_cinema_position_slot
    # A vanilla actor can still be present in another team slot. In that
    # case Cinema uses it directly and never creates a proxy. Resolve the
    # authored track ID, so either kind of source supplies the destination.
    locate_rdx CINEMA_IDS_RVA
    mov r11d, [rdx + rcx * 4 - 4]
    test r11d, r11d
    jle next_cinema_position_slot
    locate_rdx 0x18d23000
    mov r9, [rdx + rax * 8]
    test r9, r9
    jz next_cinema_position_slot
    cmp byte ptr [r9 + 0x608], al
    jne next_cinema_position_slot
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x0aa950
    cmp [r9 + 0x10], rax
    je next_cinema_position_slot
    mov rdx, [rdi + 0x158]
    test rdx, rdx
    jz next_cinema_position_slot
    mov rdx, [rdx]
find_cinema_position_track:
    test rdx, rdx
    jz missing_cinema_position_track
    cmp [rdx + 0x74], r11d
    je found_cinema_position_track
    mov rdx, [rdx]
    test rdx, rdx
    jz missing_cinema_position_track
    jmp find_cinema_position_track
    # The chicken introduction has no authored track for every party slot.
    # A replacement in an untracked slot still joins the staged group.
missing_cinema_position_track:
    cmp dword ptr [r9 + 0x600], 5
    jne next_cinema_position_slot
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x1d0e7f0
    cmp dword ptr [rax + 8], 1
    jne next_cinema_position_slot
    cmp word ptr [rdi + 0x22], 2
    jne next_cinema_position_slot
    cmp r11d, 5000
    je next_cinema_position_slot
    mov r11d, 5000
    mov rdx, [rdi + 0x158]
    mov rdx, [rdx]
    jmp find_cinema_position_track
found_cinema_position_track:
    mov r8, [rdx + 0x10]
    test r8, r8
    jz next_cinema_position_slot
check_cinema_pose:
    cmp dword ptr [rdx + 0x70], 0
    jl next_cinema_position_slot
    xor ecx, ecx
check_cinema_position_component:
    mov eax, [r8 + rcx * 4 + 0xc0]
    and eax, 0x7f800000
    cmp eax, 0x7f800000
    je next_cinema_position_slot
    inc ecx
    cmp ecx, 3
    jb check_cinema_position_component
    cmp dword ptr [r8 + 0xcc], 0x3f800000
    jne next_cinema_position_slot
    cmp r11d, 5002
    jne cinema_pose_saved
    .byte 0x48, 0x8d, 0x05
    rip_ref FIONA_MODEL_RVA
    movups xmm0, [r8 + 0x90]
    movups [rax + 16], xmm0
    movups xmm0, [r8 + 0xa0]
    movups [rax + 32], xmm0
    movups xmm0, [r8 + 0xb0]
    movups [rax + 48], xmm0
    mov dword ptr [rax + 12], 1
cinema_pose_saved:
    movups xmm0, [r8 + 0xc0]
    movups [r9 + 0xc0], xmm0
    mov dword ptr [r9 + 0x4d0], 20
    mov [rsp + 0x28], r9
    test dword ptr [r9 + 0x18], 0x200000
    jz next_cinema_position_slot
    mov rcx, r9
    .byte 0xe8
    rip_ref 0x2b1330
    mov rdx, [rsp + 0x28]
    .byte 0x48, 0x8d, 0x0d
    rip_ref 0x1d72a40
    .byte 0xe8
    rip_ref 0x214cc0
next_cinema_position_slot:
    inc dword ptr [rsp + 0x20]
    cmp dword ptr [rsp + 0x20], 4
    jb cinema_position_slot
finish_cinema_positions_return:
    add rsp, 0x38
    ret
move_swapped_cinema_players_end:

# Native cutscene actors receive Cinema's message handler during casting.
# Suppress the ordinary draw call only for party actors outside that cast.
draw_cinema_cast:
    # Factory cinema 11 animates only the camera, cauldron and scenery.
    # Keep the frozen gameplay lineup visible in this environment-only shot.
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x1d0e7f0
    cmp dword ptr [rax + 8], 1
    je check_swamp_eyeball_camera
    cmp dword ptr [rax + 8], 7
    jne check_stalk_reveal
    mov rax, [rax + 0x160]
    test rax, rax
    jz check_native_cinema_cast
    cmp dword ptr [rax + 0x1b4], 11
    je draw_native_object
    jmp check_native_cinema_cast
# Farm cinema 02 is the beanstalk growth reveal. Keep the selected party
# visible, hide only spawned vanilla stand-ins, and follow their staging.
check_stalk_reveal:
    cmp dword ptr [rax + 8], 6
    jne check_native_cinema_cast
    mov rax, [rax + 0x160]
    test rax, rax
    jz check_native_cinema_cast
    cmp dword ptr [rax + 0x1b4], 2
    jne check_native_cinema_cast
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_ACTORS_RVA
    xor r11d, r11d
stalk_proxy_slot:
    cmp [r10 + r11 * 8], rdx
    je skip_gameplay_actor
    inc r11d
    cmp r11d, 10
    jb stalk_proxy_slot
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x18d23000
    xor r11d, r11d
stalk_party_slot:
    cmp [r10 + r11 * 8], rdx
    je stalk_selected_actor
    inc r11d
    cmp r11d, 4
    jb stalk_party_slot
    jmp draw_native_object
stalk_selected_actor:
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x0aa950
    cmp [rdx + 0x10], rax
    je draw_native_object
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_SLOT_ORIGINS_RVA
    movzx r11d, byte ptr [rax + r11]
    test r11d, r11d
    jz draw_native_object
    dec r11d
    cmp r11d, 9
    ja draw_native_object
    .byte 0x48, 0x8d, 0x05
    rip_ref CINEMA_ACTORS_RVA
    mov rax, [rax + r11 * 8]
    test rax, rax
    jz draw_native_object
    # Match the stand-in's full root transform; never borrow its skeleton.
    cmp dword ptr [rax + 0xcc], 0x3f800000
    jne draw_native_object
    .irp field, 0xc0, 0xc4, 0xc8
    mov r10d, [rax + \field]
    and r10d, 0x7f800000
    cmp r10d, 0x7f800000
    je draw_native_object
    .endr
    .irp field, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xc0, 0xc8
    mov r10, [rax + \field]
    mov [rdx + \field], r10
    .endr
    jmp draw_native_object
check_swamp_eyeball_camera:
    mov rax, [rax + 0x160]
    test rax, rax
    jz check_native_cinema_cast
    # Follow EyeBallCounter's own completion-cinema link rather than
    # permitting every Swamp movie or guessing a cinema file number.
    .byte 0x4c, 0x8b, 0x15
    rip_ref 0x1cd6780
    test r10, r10
    jz check_native_cinema_cast
    cmp byte ptr [r10 + 0x18c], 0
    je check_native_cinema_cast
    mov r11, [r10 + 0x80]
    cmp [rax + 0x80], r11
    jne check_native_cinema_cast
    mov r11, [r10 + 0x180]
    test r11, r11
    jz check_native_cinema_cast
    movzx r10d, word ptr [rax + 0x22]
    cmp r10d, [r11 + 0x40]
    je draw_native_object
check_native_cinema_cast:
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_VISIBLE_RVA
    cmp dword ptr [r10], 0
    jne filter_cinema_party
    .byte 0x4c, 0x8d, 0x15
    rip_ref FIONA_MODEL_RVA
    cmp dword ptr [r10 + 64], 0
    je draw_native_object
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x1d0e7f0
    cmp dword ptr [r10 + 8], 1
    jne draw_native_object
filter_cinema_party:
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x0aa950
    cmp [rdx + 0x10], r10
    je draw_native_object
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x18d23000
    xor r11d, r11d
match_gameplay_actor:
    cmp [r10 + r11 * 8], rdx
    je check_gameplay_actor_visibility
    inc r11d
    cmp r11d, 9
    jb match_gameplay_actor
    jmp draw_native_object
check_gameplay_actor_visibility:
    .byte 0x4c, 0x8d, 0x15
    rip_ref CINEMA_VISIBLE_RVA
    cmp dword ptr [r10], 0
    jne skip_gameplay_actor
    cmp dword ptr [rdx + 0x600], 4
    je skip_gameplay_actor
draw_native_object:
    mov [rsp + 0x10], rbx
    .byte 0xe9
    rip_ref 0x2b3305
skip_gameplay_actor:
    ret
