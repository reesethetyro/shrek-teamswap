# Stage every replacement before touching the roster or the HUD.

.set STAGING_RVA, DRAFT_RVA + 4
.set RESPAWN_ROSTER_RVA, LOCK_RVA + 4
.set SWAPPED_CHAPTER_RVA, RESPAWN_ROSTER_RVA + 4
.set EGG_ROSTER_ACTIVE_RVA, FARM_ROSTER_RVA + 4
.macro live_lea reg_code, target
    .byte 0x48, 0x8d, \reg_code
    rip_ref \target
.endm
.macro live_call target
    .byte 0xe8
    rip_ref \target
.endm

# Start with the game's native HP after a level load or checkpoint respawn.
.macro clear_health_history
    live_lea 0x05, HP_RVA
    mov dword ptr [rax], 0
    live_lea 0x05, DRAFT_RVA
    mov dword ptr [rax], 0
.endm

reset_hp_on_level_load:
    clear_health_history
    clear_pickup_bank
    live_lea 0x05, SWAPPED_CHAPTER_RVA
    mov qword ptr [rax - 4], 0
    live_lea 0x05, SWAPPED_SCENE_RVA
    mov qword ptr [rax], 0
    live_lea 0x05, HERO_RETURN_RVA
    mov dword ptr [rax], 0
    clear_cinema_state
    push rbx
    sub rsp, 0xa0
    .byte 0xe9
    rip_ref 0x2503c9

reset_hp_on_respawn:
    clear_health_history
    mov [rsp + 8], rbx
    .byte 0xe9
    rip_ref 0x22e515

# Checkpoint and session-driven restarts clear the world before rebuilding.
# Save the actual roster just before teardown, including any scripted changes.
save_roster_before_respawn:
    live_lea 0x0d, RESPAWN_ROSTER_RVA
    live_lea 0x15, 0x1d0e7f0
    mov eax, [rdx + 8]
    # Keep campaign picks while leaving or retrying a solo alternate scene.
    cmp eax, 2
    je preserve_solo_alt_snapshot
    cmp eax, 13
    je preserve_solo_alt_snapshot
    cmp eax, 14
    jne reset_respawn_snapshot
preserve_solo_alt_snapshot:
    cmp eax, [rcx + 4]
    jne reset_respawn_snapshot
    .byte 0x48, 0x8b, 0x05
    rip_ref 0x1d0e6c0
    .byte 0x48, 0x3b, 0x05
    rip_ref 0x1d0e6d8
    je clear_respawn_world
    mov eax, [rdx + 8]
reset_respawn_snapshot:
    mov dword ptr [rcx], 0
    cmp eax, [rcx + 4]
    jne clear_respawn_world
    .byte 0x48, 0x8b, 0x05
    rip_ref 0x1d0e6c0
    test rax, rax
    jz clear_respawn_world
    .byte 0x48, 0x3b, 0x05
    rip_ref SWAPPED_SCENE_RVA
    jne clear_respawn_world
    mov r8d, [rdx + 0x54]
    cmp r8d, 9
    ja clear_respawn_world
    live_lea 0x05, 0x18d23000
    xor edx, edx
snapshot_respawn_slot:
    cmp edx, r8d
    jae clear_respawn_world
    mov r9, [rax + rdx * 8]
    test r9, r9
    jz invalid_respawn_roster
    mov r9d, [r9 + 0x600]
    cmp r9d, 9
    ja invalid_respawn_roster
    inc r9d
    mov [rcx + rdx], r9b
    inc edx
    cmp edx, 4
    jb snapshot_respawn_slot
    jmp clear_respawn_world
invalid_respawn_roster:
    mov dword ptr [rcx], 0
clear_respawn_world:
    # A failed challenge must not turn its temporary hero into a saved pick.
    live_lea 0x05, HERO_RETURN_RVA
    xor r9d, r9d
    xchg r9d, [rax]
    test r9d, r9d
    jz clear_respawn_effects
    locate_rdx RESPAWN_ROSTER_RVA
    cmp dword ptr [rdx], 0
    je clear_respawn_effects
    bsf ecx, r9d
    and ecx, -8
    mov eax, 255
    shl eax, cl
    not eax
    and [rdx], eax
    or [rdx], r9d
clear_respawn_effects:
    clear_pickup_bank
    clear_cinema_state
    .byte 0xe9
    rip_ref 0x22b980

# CreateGame calls this after placing players and restoring checkpoint data.
restore_roster_after_respawn:
    sub rsp, 0x38
    live_call 0x232a00
    live_lea 0x0d, 0x1d0e7f0
    cmp dword ptr [rcx + 8], 7
    jne restore_farm_roster
    live_lea 0x05, RED_RETRY_RVA
    cmp dword ptr [rax], 1
    jne restore_checkpoint_roster
    mov dword ptr [rax], 2
    live_lea 0x0d, RESPAWN_ROSTER_RVA
    xor r10d, r10d
    xchg r10d, [rcx]
    cmp dword ptr [rax + 4], 0
    jne finish_respawn_restore
    mov [rax + 4], r10d
    jmp finish_respawn_restore
restore_farm_roster:
    cmp dword ptr [rcx + 8], 6
    jne restore_checkpoint_roster
    live_lea 0x05, FARM_ROSTER_RVA
    mov r10d, [rax]
    test r10d, r10d
    jz restore_checkpoint_roster
    live_lea 0x05, RESPAWN_ROSTER_RVA
    mov dword ptr [rax], 0
    live_lea 0x05, EGG_ROSTER_ACTIVE_RVA
    cmp dword ptr [rax], 0
    jne finish_respawn_restore
    cmp dword ptr [rcx], 1
    je finish_respawn_restore
    cmp qword ptr [rcx + 0x40], 0
    jne finish_respawn_restore
    .byte 0x48, 0x83, 0x3d
    .long 0x1d0e6c0 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 0
    je finish_respawn_restore
    jmp apply_restored_roster
restore_checkpoint_roster:
    live_lea 0x05, RESPAWN_ROSTER_RVA
    mov r10d, [rax]
    test r10d, r10d
#if MONGO_SWAMP_TEST
    jnz check_saved_respawn_roster
    check_mongo_swamp finish_respawn_restore
    xor edx, edx
    call apply_swap_draft
    jmp finish_respawn_restore
check_saved_respawn_roster:
#else
    jz finish_respawn_restore
#endif
    live_lea 0x0d, 0x1d0e7f0
    mov r9d, [rcx + 8]
    cmp r9d, [rax + 4]
    jne discard_respawn_snapshot
    .byte 0x48, 0x8b, 0x15
    rip_ref 0x1d0e6c0
    test rdx, rdx
    jz discard_respawn_snapshot
    # Forest flight, Mines watergun and Mongo's challenge load before the
    # controller claims Hero Time. Preserve the campaign snapshot across
    # entry and retries; the native actor must survive its own handoff.
    # Keep the campaign snapshot for the return to the primary scene.
    cmp r9d, 2
    je check_solo_alt_destination
    cmp r9d, 13
    je check_solo_alt_destination
    cmp r9d, 14
    jne consume_respawn_snapshot
check_solo_alt_destination:
    .byte 0x48, 0x3b, 0x15
    rip_ref 0x1d0e6d8
    je finish_respawn_restore
consume_respawn_snapshot:
    mov dword ptr [rax], 0
    cmp dword ptr [rcx], 1
    je finish_respawn_restore
    cmp qword ptr [rcx + 0x40], 0
    jne finish_respawn_restore
    # StartAlt uses the same rebuild as respawn. Accept either direction
    # between the authored scenes in the same campaign chapter.
    .byte 0x48, 0x8b, 0x05
    rip_ref SWAPPED_SCENE_RVA
    cmp rdx, rax
    je apply_restored_roster
    live_lea 0x0d, 0x1d0e6d0
    cmp rdx, [rcx]
    jne check_alternate_destination
    cmp rax, [rcx + 8]
    jne finish_respawn_restore
    jmp apply_restored_roster
check_alternate_destination:
    cmp rdx, [rcx + 8]
    jne finish_respawn_restore
    cmp rax, [rcx]
    jne finish_respawn_restore
apply_restored_roster:
    live_lea 0x05, HP_RVA
    mov dword ptr [rax], 0
    locate_rdx DRAFT_RVA
    mov [rdx], r10d
    mov edx, 1
    call apply_swap_draft
    jmp finish_respawn_restore
discard_respawn_snapshot:
    mov dword ptr [rax], 0
finish_respawn_restore:
    add rsp, 0x38
    ret
respawn_restore_end:

add_player_during_stage:
    .byte 0x80, 0x3d
    .long STAGING_RVA - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 0
    jne defer_registration
    push rbx
    push rbp
    push rdi
    push r13
    .byte 0xe9
    rip_ref 0x22b1a6
defer_registration:
    ret

leave_swap_menu:
    movsxd rax, dword ptr [rcx + 0x5d8]
    test eax, eax
    jle original_back
    lea r10, [rax + rax * 4]
    shl r10, 4
    cmp dword ptr [rcx + r10 + 0x38], 115
    jne original_back
apply_before_back:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    xor edx, edx
    call apply_swap_draft
    mov rcx, [rsp + 0x20]
    add rsp, 0x38
original_back:
    movsxd rax, dword ptr [rcx + 0x5d8]
    .byte 0xe9
    rip_ref 0x25b1c7

#if MONGO_SWAMP_TEST
spawn_mongo_before_world_update:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    mov [rsp + 0x28], rdx
    check_mongo_swamp finish_mongo_world_hook
    reject_scripted_swap finish_mongo_world_hook
    locate_rdx DRAFT_RVA
    cmp dword ptr [rdx], 0
    jne finish_mongo_world_hook
    live_lea 0x05, 0x18d23018
    mov rax, [rax]
    test rax, rax
    jz finish_mongo_world_hook
    cmp dword ptr [rax + 0x600], 2
    jne finish_mongo_world_hook
    cmp qword ptr [rax + 0x6d8], 0
    je finish_mongo_world_hook
    locate_rdx (SWAPPED_SCENE_RVA + 8)
    cmp [rdx], rax
    je finish_mongo_world_hook
    mov [rdx], rax
    xor edx, edx
    call apply_swap_draft
finish_mongo_world_hook:
    mov rcx, [rsp + 0x20]
    mov rdx, [rsp + 0x28]
    add rsp, 0x38
    .byte 0xe9
    rip_ref 0x244b30
spawn_mongo_before_world_update_end:
#endif

apply_before_resume:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    xor edx, edx
    call apply_swap_draft
    mov rcx, [rsp + 0x20]
    add rsp, 0x38
    .byte 0xe9
    rip_ref 0x244ae0

# Locals: new players [0x30..0x4f], old players [0x50..0x6f],
# desired character IDs [0x70..0x7f], loop index [0x80].
apply_swap_draft:
    sub rsp, 0x98
    mov [rsp + 0x94], edx
    locate_rdx DRAFT_RVA
    cmp dword ptr [rdx], 0
#if MONGO_SWAMP_TEST
    jne validate_draft
    check_mongo_swamp apply_done
    # An empty draft on entry or resume can replace vanilla slot four.
    live_lea 0x05, 0x18d23018
    mov rax, [rax]
    test rax, rax
    jz apply_done
    cmp dword ptr [rax + 0x600], 2
    jne apply_done
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x48c348
    cmp [rax], r11
    je apply_done
    mov dword ptr [rdx], 0x08000000
validate_draft:
#else
    je apply_done
#endif
    cmp dword ptr [rsp + 0x94], 0
    jne check_swap_chapter
    reject_scripted_swap discard_draft
check_swap_chapter:
    live_lea 0x0d, 0x1d0e7f0
    mov eax, [rcx + 8]
    cmp eax, 13
    je check_native_challenge_scene
    cmp eax, 14
    jne check_supported_swap_chapter
check_native_challenge_scene:
    .byte 0x4c, 0x8b, 0x15
    rip_ref 0x1d0e6c0
    .byte 0x4c, 0x3b, 0x15
    rip_ref 0x1d0e6d8
    je discard_draft
check_supported_swap_chapter:
    cmp eax, 15
    ja discard_draft
    mov r8d, 0xe8ee # Campaign chapters 1, 2, 3, 5â€“7, 11, 13â€“15.
    bt r8d, eax
    jnc discard_draft
    # The expanded roster moved the count to +0x54. Match native SwapPlayer.
    mov eax, [rcx + 0x54]
    lea r8d, [eax - 1]
    cmp r8d, 8
    ja discard_draft
    mov r8d, 4
    cmp eax, r8d
    cmovb r8d, eax
    mov [rsp + 0x90], r8d
    .byte 0x48, 0x83, 0x3d
    .long 0x1d0e688 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 0
    je discard_draft
    live_lea 0x05, 0x18d23000
    movups xmm0, [rax]
    movups xmm1, [rax + 0x10]
    movups [rsp + 0x50], xmm0
    movups [rsp + 0x60], xmm1
    xorps xmm0, xmm0
    movups [rsp + 0x30], xmm0
    movups [rsp + 0x40], xmm0
    xor ecx, ecx
    xor r8d, r8d
    # Include the chapter's remaining teammates in duplicate detection.
    mov r11d, 4
seed_extra_ids:
    .byte 0x44, 0x3b, 0x1d
    rip_ref 0x1d0e844
    jae resolve_slot
    mov r10, [rax + r11 * 8]
    test r10, r10
    jz next_extra_id
    mov r9d, [r10 + 0x600]
    cmp r9d, 9
    ja next_extra_id
    bts r8d, r9d
next_extra_id:
    inc r11d
    jmp seed_extra_ids
resolve_slot:
    mov r10, [rsp + rcx * 8 + 0x50]
    test r10, r10
    jz discard_draft
    cmp byte ptr [r10 + 0x608], cl
    jne discard_draft
    # Reject an invalid world transform before staging any replacement actors.
    # Both infinities and NaNs have all exponent bits set.
    xor r11d, r11d
validate_swap_transform:
    mov eax, [r10 + r11 * 4 + 0x90]
    and eax, 0x7f800000
    cmp eax, 0x7f800000
    je discard_draft
    inc r11d
    cmp r11d, 16
    jb validate_swap_transform
    mov eax, ecx
    movzx eax, byte ptr [rdx + rax]
    test eax, eax
    jz original_id
    cmp dword ptr [rsp + 0x94], 0
    jne respawn_id
    cmp eax, CANDIDATE_COUNT
    ja discard_draft
    lea r11, [rip + candidate_ids]
    movzx eax, byte ptr [r11 + rax - 1]
    jmp remember_id
respawn_id:
    dec eax
    jmp remember_id
original_id:
    mov eax, [r10 + 0x600]
remember_id:
    cmp eax, 9
    ja discard_draft
    bts r8d, eax
    jc discard_draft
    mov [rsp + rcx * 4 + 0x70], eax
    inc ecx
    cmp ecx, [rsp + 0x90]
    jb resolve_slot

    mov dword ptr [rsp + 0x80], 0
prepare_player:
    mov ecx, [rsp + 0x80]
    mov r10, [rsp + rcx * 8 + 0x50]
    mov eax, [rsp + rcx * 4 + 0x70]
    cmp eax, [r10 + 0x600]
#if MONGO_SWAMP_TEST
    jne check_replacement_health
    cmp eax, 2
    jne next_prepare
    check_mongo_swamp next_prepare
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x48c348
    cmp [r10], r11
    je next_prepare
check_replacement_health:
#else
    je next_prepare
#endif
    cmp dword ptr [rsp + 0x94], 0
    jne prepare_replacement
    reject_unavailable_actor r10, discard_staged
prepare_replacement:
    cmp eax, 8
    jne read_character_file
    live_call 0x109010
    mov rcx, rax
    jmp load_character_file
read_character_file:
    lea r11, [rip + character_files]
    mov edx, [r11 + rax * 8]
#if MONGO_SWAMP_TEST
    cmp eax, 2
    jne use_regular_character_file
    check_mongo_swamp use_regular_character_file
    mov edx, 0x5b2320
use_regular_character_file:
#endif
    test edx, edx
    jz discard_staged
    live_lea 0x0d, 0
    add rcx, rdx
load_character_file:
    mov [rsp + 0x88], rcx
    live_call 0x08ebe0
    test al, al
    jz discard_staged
    mov rcx, [rsp + 0x88]
    mov rdx, [rcx + 0x28]
    test rdx, rdx
    jz discard_staged
    live_lea 0x0d, 0x1d10ce0
    mov eax, [rsp + 0x80]
    mov eax, [rsp + rax * 4 + 0x70]
    lea r11, [rip + character_files]
    mov r9d, [r11 + rax * 8 + 4]
#if MONGO_SWAMP_TEST
    cmp eax, 2
    jne use_regular_character_handler
    check_mongo_swamp use_regular_character_handler
    mov r9d, 0x1aa5f0
use_regular_character_handler:
#endif
    live_lea 0x05, 0
    add r9, rax
    xor r8d, r8d
    mov eax, [rsp + 0x80]
    cmp dword ptr [rsp + rax * 4 + 0x70], 8
    jne staged_model_ready
    live_lea 0x05, FIONA_MODEL_RVA
    cmp qword ptr [rax], 0
    je staged_model_ready
    mov rdx, [rax]
    movzx r8d, word ptr [rax + 8]
staged_model_ready:
    mov dword ptr [rsp + 0x20], 0xc
    mov word ptr [rsp + 0x28], 0xffff
    live_lea 0x05, STAGING_RVA
    mov byte ptr [rax], 1
    live_call 0x241dd0
    locate_rdx STAGING_RVA
    mov byte ptr [rdx], 0
    test rax, rax
    jz discard_staged
    mov ecx, [rsp + 0x80]
    mov [rsp + rcx * 8 + 0x30], rax
    mov edx, [rsp + rcx * 4 + 0x70]
    cmp [rax + 0x600], edx
    jne discard_staged
#if MONGO_SWAMP_TEST
    cmp edx, 2
    jne copy_replacement_slot
    check_mongo_swamp copy_replacement_slot
    # Native Mongo starts hidden until the scripted reveal. Clear only
    # the draw suppression bit for the staged Swamp replacement.
    and dword ptr [rax + 0x18], -2
    # Node 53 is a rigid button with geometry about one eleventh of
    # node 51's width. Scale its basis and keep its attachment point.
    mov r11, rax
    .irp link, 0x30, 0x30, 0x30, 0x30, 0x28, 0x30, 0x28
    mov r11, [r11 + \link]
    test r11, r11
    jz copy_replacement_slot
    .endr
    cmp word ptr [r11 + 0x22], 53
    jne copy_replacement_slot
    mov r8, [r11 + 0x60]
    test r8, r8
    jz copy_replacement_slot
    cmp dword ptr [r8 + 4], 20
    jne copy_replacement_slot
    mov r8d, 0x412e6666 # 10.9
    movd xmm0, r8d
    shufps xmm0, xmm0, 0
    .irp axis, 0x90, 0xa0, 0xb0
    movups xmm1, [r11 + \axis]
    mulps xmm1, xmm0
    movups [r11 + \axis], xmm1
    .endr
copy_replacement_slot:
#endif
    mov r10, [rsp + rcx * 8 + 0x50]
    mov edx, [r10 + 0x604]
    mov [rax + 0x604], edx
    mov rdx, [r10 + 0x610]
    mov [rax + 0x610], rdx
    movups xmm0, [r10 + 0x90]
    movups xmm1, [r10 + 0xa0]
    movups xmm2, [r10 + 0xb0]
    movups xmm3, [r10 + 0xc0]
    movups [rax + 0x90], xmm0
    movups [rax + 0xa0], xmm1
    movups [rax + 0xb0], xmm2
    movups [rax + 0xc0], xmm3
    # Start movement at the transferred position. Preserve authored HoldPlayer
    # staging destinations when this slot is already held by a room trigger.
    movups [rax + 0x1d0], xmm3
    movups [rax + 0x1e0], xmm3
    mov edx, [r10 + 0x390]
    mov [rax + 0x390], edx
    test byte ptr [r10 + 0x200], 2
    je movement_target_ready
    movups xmm0, [r10 + 0x1d0]
    movups xmm1, [r10 + 0x1e0]
    movups [rax + 0x1d0], xmm0
    movups [rax + 0x1e0], xmm1
    or dword ptr [rax + 0x200], 2
movement_target_ready:
#if DRAGON_SWAMP_TEST
    cmp dword ptr [rax + 0x600], 7
    jne next_prepare
    check_free_dragon discard_staged, rax, r11
    movss xmm3, [rax + 0xc4]
    subss xmm3, [rip + dragon_spawn_lift]
    movss [rax + 0xc4], xmm3
#endif
next_prepare:
    inc dword ptr [rsp + 0x80]
    mov eax, [rsp + 0x90]
    cmp dword ptr [rsp + 0x80], eax
    jb prepare_player

    cmp dword ptr [rsp + 0x94], 0
    jne begin_install
    # Snapshot all current actors before replacing any of them. This also
    # handles moving a damaged character directly to another slot.
    live_lea 0x0d, HP_RVA
    xor edx, edx
save_party_hp:
    mov r10, [rsp + rdx * 8 + 0x50]
    mov eax, [r10 + 0x600]
    cmp eax, 9
    ja next_saved_hp
    mov r11d, [r10 + 0x2f4]
    mov [rcx + rax * 4 + 4], r11d
    bts dword ptr [rcx], eax
next_saved_hp:
    inc edx
    cmp edx, [rsp + 0x90]
    jb save_party_hp
    mov dword ptr [rsp + 0x80], 0
save_party_ammo:
    mov eax, [rsp + 0x80]
    cmp qword ptr [rsp + rax * 8 + 0x30], 0
    je next_saved_ammo
    mov rcx, [rsp + rax * 8 + 0x50]
    call remember_pickup_ammo
next_saved_ammo:
    inc dword ptr [rsp + 0x80]
    mov eax, [rsp + 0x90]
    cmp [rsp + 0x80], eax
    jb save_party_ammo

begin_install:
    mov dword ptr [rsp + 0x80], 0
install_player:
    mov eax, [rsp + 0x80]
    mov r8, [rsp + rax * 8 + 0x30]
    test r8, r8
    jz next_install
    mov rdx, [rsp + rax * 8 + 0x50]
    mov rcx, rdx
    call remember_cinema_actor
    mov eax, [rsp + 0x80]
    mov r8, [rsp + rax * 8 + 0x30]
    mov rdx, [rsp + rax * 8 + 0x50]
    inc word ptr [rdx + 0x1e]
    live_lea 0x0d, 0x1d0e7f0
    live_call 0x22f0f0
    # A midlevel spawn can bypass the startup callback that creates its HUD.
    # Initialize it after assignment so the portrait uses the destination slot.
    mov eax, [rsp + 0x80]
    mov rcx, [rsp + rax * 8 + 0x30]
    cmp qword ptr [rcx + 0x6d8], 0
    jne refresh_control
    xor edx, edx
    live_call 0x1e0700
refresh_control:
    cmp dword ptr [rsp + 0x94], 0
    jne read_replacement_actor
    mov eax, [rsp + 0x80]
    mov rcx, [rsp + rax * 8 + 0x30]
    call restore_pickup_ammo
read_replacement_actor:
    mov eax, [rsp + 0x80]
    mov r8, [rsp + rax * 8 + 0x30]
    mov rdx, [rsp + rax * 8 + 0x50]
    mov r10d, [r8 + 0x600]
    live_lea 0x0d, HP_RVA
    bt dword ptr [rcx], r10d
    jnc restore_control
    mov r11d, [rcx + r10 * 4 + 4]
    mov [r8 + 0x2f4], r11d
restore_control:
    # The game holds a separate reference to the controlled actor.
    live_lea 0x0d, 0x1d0e830
    cmp [rcx], rdx
    jne retire_hud
    inc word ptr [r8 + 0x1e]
    mov [rcx], r8
    mov rcx, rdx
    live_call 0x2adb00
retire_hud:
    mov eax, [rsp + 0x80]
    mov rdx, [rsp + rax * 8 + 0x50]
    mov rcx, [rdx + 0x6d8]
    test rcx, rcx
    jz retire_player
    mov dword ptr [rcx + 8], 4
retire_player:
    mov rcx, rdx
    live_call 0x2adee0
    mov eax, [rsp + 0x80]
    mov rcx, [rsp + rax * 8 + 0x50]
    live_call 0x2adb00
next_install:
    inc dword ptr [rsp + 0x80]
    cmp dword ptr [rsp + 0x80], 4
    jb install_player
    live_lea 0x05, SWAPPED_CHAPTER_RVA
    live_lea 0x0d, 0x1d0e7f0
    mov ecx, [rcx + 8]
    mov [rax], ecx
    live_lea 0x05, SWAPPED_SCENE_RVA
    .byte 0x48, 0x8b, 0x0d
    rip_ref 0x1d0e6c0
    mov [rax], rcx
    # Only a successful player edit can replace the farm's saved lineup.
    cmp dword ptr [rsp + 0x94], 0
    jne discard_draft
    live_lea 0x0d, 0x1d0e7f0
    cmp dword ptr [rcx + 8], 6
    jne discard_draft
    cmp dword ptr [rsp + 0x90], 4
    jne discard_draft
    live_lea 0x05, FARM_ROSTER_RVA
    xor ecx, ecx
save_farm_slot:
    mov edx, [rsp + rcx * 4 + 0x70]
    inc edx
    mov [rax + rcx], dl
    inc ecx
    cmp ecx, 4
    jb save_farm_slot
    jmp discard_draft

discard_staged:
    mov dword ptr [rsp + 0x80], 0
delete_staged:
    mov eax, [rsp + 0x80]
    mov rcx, [rsp + rax * 8 + 0x30]
    test rcx, rcx
    jz next_staged
    live_call 0x2adee0
next_staged:
    inc dword ptr [rsp + 0x80]
    cmp dword ptr [rsp + 0x80], 4
    jb delete_staged
discard_draft:
    locate_rdx DRAFT_RVA
    mov dword ptr [rdx], 0
apply_done:
    add rsp, 0x98
    ret
apply_end:
character_files:
    .long 0x5e0680, 0x23f520 # Shrek
    .long 0x5ac580, 0x0b2620 # Donkey
    .long 0x5ad460, 0x11ce50 # Gingy
    .long 0x5accb0, 0x107d40 # Fairy
    .long 0x5ae2c0, 0x196db0 # Lil' Red
    .long 0x5ad4c0, 0x120ee0 # Shrek & Donkey
    .long 0x5bac40, 0x1cd1e0 # Puss
#if DRAGON_SWAMP_TEST
    .long 0x5ac5e0, 0x0b3430 # Dragon
#else
    .long 0, 0              # Dragon is not an available draft choice
#endif
    .long 0, 0x109000        # Fiona uses the chapter-specific native file lookup
    .long 0x5ab570, 0x06b1f0 # Wolf

# StartHeroTime selects the authored hero by actor handler. Replace one
# slot temporarily when a campaign hero has been swapped out. Special
# actors such as Dragon, Mongo, and Watergun Handsome are spawned by the
# native challenge and continue through the original entry.
start_campaign_hero_time:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    mov [rsp + 0x28], rdx
    call prepare_campaign_hero
    mov rcx, [rsp + 0x20]
    mov rdx, [rsp + 0x28]
    add rsp, 0x38
    mov [rsp + 0x18], rbx
    .byte 0xe9
    rip_ref 0x22ef35
start_campaign_hero_end:

prepare_campaign_hero:
    sub rsp, 0x38
    mov [rsp + 0x28], rdx
    live_lea 0x05, 0
    mov r10, rdx
    sub r10, rax
    mov eax, r10d
    cmp rax, r10
    jne start_native_hero
    test eax, eax
    jz start_native_hero
    lea r11, [rip + character_files]
    xor r9d, r9d
find_requested_hero:
    cmp eax, [r11 + r9 * 8 + 4]
    je found_requested_hero
    inc r9d
    cmp r9d, 10
    jb find_requested_hero
    jmp start_native_hero
found_requested_hero:
    cmp r9d, 7
    je start_native_hero
    inc r9d
    mov [rsp + 0x30], r9d
    cmp dword ptr [rcx], 0
    je hero_entry_phase_ready
    # The Far Far Away wagon starts Shrek's challenge from a scripted
    # handoff, before the controller has returned to ordinary gameplay.
    cmp dword ptr [rcx + 8], 3
    jne start_native_hero
    cmp r9d, 1
    jne start_native_hero
hero_entry_phase_ready:
    locate_rdx HERO_RETURN_RVA
    cmp dword ptr [rdx], 0
    jne start_native_hero
    mov r8d, [rcx + 0x54]
    lea eax, [r8 - 1]
    cmp eax, 8
    ja start_native_hero
    live_lea 0x05, 0x18d23000
    xor r9d, r9d
find_active_hero:
    mov r10, [rax + r9 * 8]
    test r10, r10
    jz next_hero_player
    mov r11, [rsp + 0x28]
    cmp [r10 + 0x10], r11
    je start_native_hero
next_hero_player:
    inc r9d
    cmp r9d, r8d
    jb find_active_hero

    .byte 0x4c, 0x8d, 0x1d
    rip_ref CINEMA_SLOT_ORIGINS_RVA
    xor ecx, ecx
    mov r9d, [rsp + 0x30]
find_original_hero_slot:
    cmp byte ptr [r11 + rcx], r9b
    je borrow_hero_slot
    inc ecx
    cmp ecx, r8d
    jae use_first_hero_slot
    cmp ecx, 4
    jb find_original_hero_slot
use_first_hero_slot:
    xor ecx, ecx
borrow_hero_slot:
    mov r10, [rax + rcx * 8]
    test r10, r10
    jz start_native_hero
    mov r10d, [r10 + 0x600]
    cmp r10d, 9
    ja start_native_hero
    inc r10d
    mov r11d, [rsp + 0x30]
    shl ecx, 3
    shl r10d, cl
    shl r11d, cl
    mov [rdx], r10d
    locate_rdx DRAFT_RVA
    mov [rdx], r11d
    mov edx, 1
    call apply_swap_draft
start_native_hero:
    add rsp, 0x38
    ret
prepare_campaign_hero_end:

# RhythmFiona borrows regular Fiona before StartHeroTime is called. Prepare
# the original hero before that lookup, rather than after it has failed.
prepare_swamp_rhythm_fiona:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    mov [rsp + 0x28], rdx
    mov [rsp + 0x30], r8
    live_lea 0x05, 0x1d0e7f0
    cmp dword ptr [rax + 8], 1
    jne continue_rhythm_player_update
    cmp dword ptr [rcx + 0x748], 0
    jne continue_rhythm_player_update
    live_lea 0x15, 0x109000
    cmp [rcx + 0x750], rdx
    jne continue_rhythm_player_update
    live_lea 0x0d, 0x1d0e7f0
    call prepare_campaign_hero
    live_lea 0x05, FIONA_MODEL_RVA
    cmp dword ptr [rax + 12], 1
    jne continue_rhythm_player_update
    live_lea 0x0d, 0x1d0e7f0
    live_lea 0x15, 0x109000
    live_call 0x22c560
    test rax, rax
    jz continue_rhythm_player_update
    live_lea 0x15, FIONA_MODEL_RVA
    movups xmm0, [rdx + 16]
    movups [rax + 0x90], xmm0
    movups xmm0, [rdx + 32]
    movups [rax + 0xa0], xmm0
    movups xmm0, [rdx + 48]
    movups [rax + 0xb0], xmm0
    mov rdx, rax
    lea rcx, [rax + 0x390]
    live_call 0x08d7c0
continue_rhythm_player_update:
    mov rcx, [rsp + 0x20]
    mov rdx, [rsp + 0x28]
    mov r8, [rsp + 0x30]
    add rsp, 0x38
    .byte 0xe9
    rip_ref 0x21add0
prepare_swamp_rhythm_fiona_end:

# Native menu input 0x22 is Y/Restore Defaults. Only claim it on Swap Team;
# leave the Options menu's existing restore behavior unchanged.
restore_team_defaults_input:
    sub rsp, 0x38
    live_call 0x12d0e0
    test al, al
    jz restore_team_defaults_done
    movsxd rax, dword ptr [rbx + 0x5d8]
    cmp eax, 1
    jl native_restore_defaults_input
    lea rax, [rax + rax * 4]
    shl rax, 4
    add rax, rbx
    cmp dword ptr [rax + 0x38], 115
    jne native_restore_defaults_input
    cmp dword ptr [rax - 0x18], 133
    jne native_restore_defaults_input
    reject_scripted_swap reject_team_defaults
    mov dword ptr [rsp + 0x20], 0
    xor ecx, ecx
collect_vanilla_team_slot:
    live_lea 0x05, 0x18d23000
    mov r11, [rax + rcx * 8]
    test r11, r11
    jz reject_team_defaults
    reject_unavailable_actor r11, reject_team_defaults
    live_lea 0x15, CINEMA_SLOT_ORIGINS_RVA
    movzx eax, byte ptr [rdx + rcx]
    test eax, eax
    jnz recorded_vanilla_team_slot
    mov eax, [r11 + 0x600]
    inc eax
recorded_vanilla_team_slot:
    dec eax
    cmp eax, 9
    ja reject_team_defaults
    xor r8d, r8d
    cmp eax, [r11 + 0x600]
    je remember_vanilla_team_slot
    lea rdx, [rip + candidate_positions]
    movzx r8d, byte ptr [rdx + rax]
    test r8d, r8d
    jz reject_team_defaults
remember_vanilla_team_slot:
    mov [rsp + rcx + 0x20], r8b
    inc ecx
    cmp ecx, 4
    jb collect_vanilla_team_slot
    live_lea 0x05, DRAFT_RVA
    mov edx, [rsp + 0x20]
    mov [rax], edx
    xor ecx, ecx
draw_vanilla_team_slot:
    movzx edx, byte ptr [rsp + rcx + 0x20]
    imul r9d, ecx, 2 * 0xe8
    live_lea 0x05, (PAGE_RVA + 0xe8)
    add r9, rax
    test edx, edx
    jnz vanilla_candidate_label
    imul edx, ecx, 2 * 0xe8
    live_lea 0x05, (0x1cdd060 + 0xe8)
    movups xmm0, [rax + rdx]
    movups [r9], xmm0
    jmp next_vanilla_team_label
vanilla_candidate_label:
    dec edx
    shl edx, 6
    lea rax, [rip + candidate_names]
    add rax, rdx
    mov [r9], rax
    mov rdx, [rax + 56]
    mov [r9 + 8], rdx
next_vanilla_team_label:
    inc ecx
    cmp ecx, 4
    jb draw_vanilla_team_slot
    live_lea 0x05, 0x1d0e688
    mov rcx, [rax]
    test rcx, rcx
    jz reject_team_defaults
    live_call 0x129ea0
reject_team_defaults:
    xor eax, eax
    jmp restore_team_defaults_done
native_restore_defaults_input:
    mov eax, 1
restore_team_defaults_done:
    add rsp, 0x38
    ret
restore_team_defaults_input_end:

# The first entry can be requested during the intro. Wait for gameplay
# ownership before activating the boss or starting Red's challenge.
queue_red_hero_time:
    cmp byte ptr [rcx + 0x158], 0
    jne red_hero_request_done
    live_lea 0x05, RED_RETRY_RVA
    mov [rax + 8], rcx
    cmp dword ptr [rax], 0
    jne red_hero_request_done
    mov dword ptr [rax], 4
red_hero_request_done:
    xor eax, eax
    ret

# Red's script places the cached actor before it calls StartHeroTime.
# Resolve the active actor here so the stock trigger uses its own spawn node.
start_red_hero_time:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    mov [rsp + 0x28], rdx
    cmp byte ptr [rcx + 0x158], 0
    jne trigger_red_hero_time
    cmp qword ptr [rcx + 0x168], 0
    jne prepare_red_actor
    # Resolve the boss from the same authored ID used by the first frame.
    mov rdx, [rcx + 0x150]
    test rdx, rdx
    jz wait_for_red
    mov edx, [rdx + 0xc]
    live_call 0x08ebc0
    test rax, rax
    jz wait_for_red
    live_lea 0x15, 0x1889a0
    cmp [rax + 0x10], rdx
    jne wait_for_red
    mov rcx, [rsp + 0x20]
    inc word ptr [rax + 0x1e]
    mov [rcx + 0x168], rax
    movss xmm0, [rax + 0x2f4]
    .byte 0xf3, 0x0f, 0x59, 0x05
    rip_ref 0x456f98
    movss [rcx + 0x1b0], xmm0
prepare_red_actor:
    live_lea 0x0d, 0x1d0e7f0
    live_lea 0x15, 0x196db0
    call prepare_campaign_hero
    live_lea 0x0d, 0x1d0e7f0
    live_lea 0x15, 0x196db0
    live_call 0x22c560
    mov rcx, [rsp + 0x20]
    mov [rcx + 0x160], rax
    test rax, rax
    jz wait_for_red
    # The boss caches its spell target independently during its first frame.
    # Refresh that borrowed pointer after the replacement Red exists.
    mov rdx, [rcx + 0x168]
    test rdx, rdx
    jz wait_for_red
    mov [rdx + 0x550], rax
trigger_red_hero_time:
    mov rcx, [rsp + 0x20]
    mov rdx, [rsp + 0x28]
    add rsp, 0x38
    .byte 0xe9
    rip_ref 0x18ea90
wait_for_red:
    xor eax, eax
    add rsp, 0x38
    ret
start_red_hero_time_end:

# The native first-frame callback has now resolved the spawn and props.
remember_red_challenge:
    live_lea 0x05, RED_RETRY_RVA
    mov [rax + 8], rdx
    .byte 0xe9
    rip_ref 0x2bdcd0

retry_red_hero_time:
    live_lea 0x05, RED_RETRY_RVA
    mov dword ptr [rax], 1
    .byte 0xe9
    rip_ref 0x22e890

win_red_hero_time:
    live_lea 0x05, RED_RETRY_RVA
    cmp dword ptr [rax], 0
    je end_red_hero_time
    mov dword ptr [rax], 3
end_red_hero_time:
    .byte 0xe9
    rip_ref 0x22c300

# Restore the borrowed slot on the first regular frame after Hero Time.
finish_campaign_hero_time:
    sub rsp, 0x38
    live_call 0x238ca0
#if FACTORY_POTION_TEST
    call finish_factory_potion_test
#endif
    call restore_finished_run_team
    live_lea 0x05, RED_RETRY_RVA
    cmp dword ptr [rax], 4
    je restart_red_challenge
    cmp dword ptr [rax], 2
    je restart_red_challenge
    cmp dword ptr [rax], 3
    jne restore_borrowed_hero_slot
    reject_scripted_swap finish_campaign_hero_update
    mov ecx, [rax + 4]
    mov qword ptr [rax], 0
    test ecx, ecx
    jz restore_borrowed_hero_slot
    live_lea 0x05, HERO_RETURN_RVA
    mov dword ptr [rax], 0
    jmp apply_hero_return
restore_borrowed_hero_slot:
    live_lea 0x05, HERO_RETURN_RVA
    cmp dword ptr [rax], 0
    je finish_campaign_hero_update
    reject_scripted_swap finish_campaign_hero_update
    xor ecx, ecx
    xchg ecx, [rax]
apply_hero_return:
    locate_rdx DRAFT_RVA
    mov [rdx], ecx
    mov edx, 1
    call apply_swap_draft
    jmp finish_campaign_hero_update
restart_red_challenge:
    reject_scripted_swap finish_campaign_hero_update
    mov rcx, [rax + 8]
    test rcx, rcx
    jz finish_campaign_hero_update
    cmp byte ptr [rcx + 0x158], 0
    jne finish_campaign_hero_update
    cmp qword ptr [rcx + 0x180], 0
    je finish_campaign_hero_update
    mov [rsp + 0x28], rcx
    xor edx, edx
    call start_red_hero_time
    live_lea 0x05, RED_RETRY_RVA
    cmp dword ptr [rax], 4
    jne finish_campaign_hero_update
    mov rcx, [rsp + 0x28]
    cmp byte ptr [rcx + 0x158], 0
    je finish_campaign_hero_update
    mov dword ptr [rax], 0
finish_campaign_hero_update:
    add rsp, 0x38
    ret
finish_campaign_hero_end:

# RunGame's authored setup assumes particular character classes exist.
# Its movement controller already drives players by slot. Install a common
# event wrapper while retaining each actor's native character handler.
setup_current_run_team:
    live_lea 0x05, 0x1d0e7f0
    cmp dword ptr [rax + 8], 7
    je setup_factory_run_team
    mov [rsp + 8], rbx
    .byte 0xe9
    rip_ref 0x21f305
setup_factory_run_team:
    sub rsp, 0x38
    mov [rsp + 0x28], rcx
    live_lea 0x05, RUN_TEAM_RVA
    mov [rax], rcx
    mov dword ptr [rax + 8], 1
    mov dword ptr [rax + 12], 0
    live_lea 0x05, 0x18d23000
    mov dword ptr [rsp + 0x30], 0
setup_run_slot:
    live_lea 0x05, 0x18d23000
    mov r9d, [rsp + 0x30]
    mov r10, [rax + r9 * 8]
    test r10, r10
    jz next_run_slot
    mov rcx, r10
    call release_run_reference
    live_lea 0x05, 0x18d23000
    mov r9d, [rsp + 0x30]
    mov r10, [rax + r9 * 8]
    mov rcx, [rsp + 0x28]
    inc word ptr [rcx + 0x1e]
    mov [r10 + 0x730], rcx
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x221aa0
    mov [r10 + 0x10], r11
next_run_slot:
    inc dword ptr [rsp + 0x30]
    cmp dword ptr [rsp + 0x30], 4
    jb setup_run_slot
    # Native FirstFrame has a cauldron-run-only relocation (node 27).
    # The original Shrek slot can now contain any character, including the
    # mounted duo; keeping its platform position leaves it behind the hazard.
    mov r9, [rsp + 0x28]
    cmp word ptr [r9 + 0x22], 27
    jne run_team_setup_done
    live_lea 0x05, CINEMA_SLOT_ORIGINS_RVA
    xor edx, edx
find_cauldron_shrek_slot:
    cmp byte ptr [rax + rdx], 1
    je place_cauldron_runner
    inc edx
    cmp edx, 4
    jb find_cauldron_shrek_slot
    xor edx, edx
place_cauldron_runner:
    live_lea 0x05, 0x18d23000
    mov rcx, [rax + rdx * 8]
    test rcx, rcx
    jz run_team_setup_done
    mov [rsp + 0x20], rcx
    live_lea 0x05, 0x4b8c90
    movss xmm0, [rax]
    movss [rcx + 0xc0], xmm0
    movss xmm0, [r9 + 0xc4]
    movss [rcx + 0xc4], xmm0
    test dword ptr [rcx + 0x18], 0x200000
    jz run_team_setup_done
    live_call 0x2b1330
    mov rdx, [rsp + 0x20]
    live_lea 0x0d, 0x1d72a40
    live_call 0x214cc0
run_team_setup_done:
    xor eax, eax
    add rsp, 0x38
    ret
setup_current_run_team_end:

# Player::Update uses this shared RCptr to enter run control and notify
# RunGameCont. Clear it with normal ownership rules on completion/deletion.
release_run_reference:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    mov [rsp + 0x28], rdx
    mov [rsp + 0x30], r8
    xor eax, eax
    xchg rax, [rcx + 0x730]
    test rax, rax
    jz run_reference_released
    mov rcx, rax
    live_call 0x2adb00
run_reference_released:
    mov rcx, [rsp + 0x20]
    mov rdx, [rsp + 0x28]
    mov r8, [rsp + 0x30]
    mov eax, [rcx + 0x600]
    lea r11, [rip + character_files]
    mov r11d, [r11 + rax * 8 + 4]
    live_lea 0x05, 0
    add rax, r11
    add rsp, 0x38
    ret
release_run_reference_end:

# Native camera, obstacle, and controller queries recognize five hardcoded
# run handlers. Resolve our wrapper through the active run association.
resolve_current_run_rect:
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x221aa0
    cmp rcx, r11
    jne original_run_rect_check
    .byte 0x4c, 0x8d, 0x1d
    rip_ref RUN_TEAM_RVA
    cmp qword ptr [r11], 0
    je original_run_rect_check
    live_lea 0x05, RUN_TEAM_RVA
    mov rax, [rax]
    test rax, rax
    .byte 0xe9
    rip_ref 0x21ed08
original_run_rect_check:
    cmp rcx, r13
    je native_run_rect_found
    cmp rcx, rsi
    .byte 0xe9
    rip_ref 0x21ecea
native_run_rect_found:
    .byte 0xe9
    rip_ref 0x21ecff

resolve_current_run_follower:
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x221aa0
    cmp rcx, r11
    jne original_run_follower_check
    .byte 0x4c, 0x8d, 0x1d
    rip_ref RUN_TEAM_RVA
    cmp qword ptr [r11], 0
    je original_run_follower_check
    live_lea 0x2d, RUN_TEAM_RVA
    mov rbp, [rbp]
    test rbp, rbp
    .byte 0xe9
    rip_ref 0x226234
original_run_follower_check:
    cmp rcx, rsi
    je native_run_follower_found
    .byte 0xe9
    rip_ref 0x226217
native_run_follower_found:
    .byte 0xe9
    rip_ref 0x22622b

current_run_player:
    sub rsp, 0x38
    live_lea 0x05, RUN_TEAM_RVA
    cmp qword ptr [rax], 0
    je native_shrek_run_player
    live_lea 0x05, 0x1d0e7f0
    cmp dword ptr [rax + 8], 7
    jne native_shrek_run_player
    mov eax, [rcx + 0x600]
    cmp eax, 9
    ja unrecognized_run_player
    lea r11, [rip + character_files]
    mov r11d, [r11 + rax * 8 + 4]
    live_lea 0x05, 0
    add r11, rax
    cmp edx, 4
    je restore_run_player_handler
    cmp edx, 11
    jne check_run_damage
    test r8, r8
    jz run_player_done
    cmp dword ptr [r8], 48
    jne run_player_done
    live_lea 0x05, RUN_TEAM_RVA
    mov r9, [r8 + 8]
    cmp [rax], r9
    jne run_player_done
    mov dword ptr [rax + 8], 0
    call release_run_reference
    mov [rcx + 0x10], rax
    xor eax, eax
    add rsp, 0x38
    ret
restore_run_player_handler:
    call release_run_reference
    mov [rcx + 0x10], rax
    add rsp, 0x38
    jmp rax
check_run_damage:
    cmp edx, 12
    jne delegate_run_player
    test r8, r8
    jz delegate_run_player
    # Native RunFollower sends this when a runner falls behind the hazard.
    live_lea 0x05, 0x2216d0
    cmp [r8 + 0x10], rax
    jne delegate_run_player
    live_lea 0x05, RUN_TEAM_RVA
    cmp dword ptr [rax + 8], 0
    je delegate_run_player
    # Collision volume overlap is too early for the mounted duo. Use the
    # same trailing plane for every runner, measured at their actor origin.
    movss xmm0, [rcx + 0xc0]
    subss xmm0, [r8 + 0xc0]
    mulss xmm0, [r8 + 0xb0]
    movss xmm1, [rcx + 0xc8]
    subss xmm1, [r8 + 0xc8]
    mulss xmm1, [r8 + 0xb8]
    addss xmm0, xmm1
    xorps xmm1, xmm1
    comiss xmm0, xmm1
    ja run_player_done
    xorps xmm0, xmm0
    comiss xmm0, [rcx + 0x2f4]
    jae delegate_run_player
    mov dword ptr [rcx + 0x2f4], 0
    mov rax, [rcx]
    xor r8d, r8d
    mov edx, 14
    add rsp, 0x38
    jmp qword ptr [rax + 0x58]
delegate_run_player:
    add rsp, 0x38
    jmp r11
native_shrek_run_player:
    add rsp, 0x38
    mov [rsp + 8], rbx
    .byte 0xe9
    rip_ref 0x221aa5
unrecognized_run_player:
run_player_done:
    xor eax, eax
    add rsp, 0x38
    ret
current_run_player_end:

# The factory escape should return directly to normal control on success.
finish_factory_run_without_dance:
    cmp dword ptr [rcx + 8], 7
    jne native_run_victory
    live_lea 0x05, RUN_TEAM_RVA
    mov dword ptr [rax + 12], 1
    xor eax, eax
    ret
native_run_victory:
    .byte 0xe9
    rip_ref 0x230070

# EndGame has already restored HP. Wait until EndRunGame has handed back
# control, then enter each character's own idle mode without a get-up pose.
# InitMode's force flag skips Player's animation setup; it must remain false.
restore_finished_run_team:
    sub rsp, 0x38
    live_lea 0x05, RUN_TEAM_RVA
    cmp dword ptr [rax + 12], 1
    jne finished_run_restored
    reject_scripted_swap finished_run_restored
    live_lea 0x05, RUN_TEAM_RVA
    mov dword ptr [rax + 12], 0
    mov dword ptr [rsp + 0x30], 0
restore_finished_run_slot:
    live_lea 0x05, 0x18d23000
    mov r9d, [rsp + 0x30]
    mov rdx, [rax + r9 * 8]
    test rdx, rdx
    jz next_finished_run_slot
    mov [rsp + 0x28], rdx
    lea rcx, [rdx + 0x390]
    live_call 0x08d7c0
    mov rcx, [rsp + 0x28]
    mov rax, [rcx]
    mov edx, 0
    mov r8d, 0
    call qword ptr [rax + 0x58]
next_finished_run_slot:
    inc dword ptr [rsp + 0x30]
    cmp dword ptr [rsp + 0x30], 4
    jb restore_finished_run_slot
finished_run_restored:
    add rsp, 0x38
    ret
restore_finished_run_team_end:

# A removed barrage can leave the shared projectile count nonzero after
# scripted world transitions. Retain the native gate whenever any real
# barrage projectile remains; do not rewrite its reference/lifetime count.
check_factory_fairy_attack:
    live_lea 0x05, 0x1cd5e98
    cmp dword ptr [rax], 0
    je fairy_attack_available
    live_lea 0x05, 0x1d0e7f0
    cmp dword ptr [rax + 8], 7
    jne fairy_attack_blocked
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x101c30
    live_lea 0x05, 0x1d10d70
    mov r8, rax
    mov r10, [rax + 8]
    mov edx, 4096
scan_fairy_animated:
    test r10, r10
    jz scan_fairy_pending
    cmp r10, r8
    je scan_fairy_pending
    mov r9, [r10 + 16]
    test r9, r9
    jz next_fairy_animated
    cmp [r9 + 16], r11
    je fairy_attack_blocked
next_fairy_animated:
    mov r10, [r10 + 8]
    dec edx
    jnz scan_fairy_animated
    jmp fairy_attack_blocked
scan_fairy_pending:
    live_lea 0x05, 0x1d10d90
    mov r8, rax
    mov r10, [rax + 8]
    mov edx, 4096
scan_fairy_pending_node:
    test r10, r10
    jz scan_fairy_frame_calls
    cmp r10, r8
    je scan_fairy_frame_calls
    mov r9, [r10 + 16]
    test r9, r9
    jz next_fairy_pending
    cmp [r9 + 16], r11
    je fairy_attack_blocked
next_fairy_pending:
    mov r10, [r10 + 8]
    dec edx
    jnz scan_fairy_pending_node
    jmp fairy_attack_blocked
scan_fairy_frame_calls:
    live_lea 0x05, 0x1d10d80
    mov r8, rax
    mov r10, [rax]
    mov edx, 4096
scan_fairy_frame_node:
    test r10, r10
    jz fairy_attack_available
    cmp r10, r8
    je fairy_attack_available
    mov r9, [r10 + 16]
    test r9, r9
    jz next_fairy_frame
    cmp [r9 + 16], r11
    je fairy_attack_blocked
next_fairy_frame:
    mov r10, [r10]
    dec edx
    jnz scan_fairy_frame_node
fairy_attack_blocked:
    .byte 0xe9
    rip_ref 0x1078e1
fairy_attack_available:
    .byte 0xe9
    rip_ref 0x107869

# PotionSpawner sends conveyor contact every frame, even after its pickup
# has passed the belt. Clip this scripted movement to the belt's authored
# local collision extent, rather than letting the pickup float away.
move_factory_conveyor_potion:
    live_lea 0x15, 0x1d0e7f0
    cmp dword ptr [rdx + 8], 7
    jne native_conveyor_potion
    cmp dword ptr [rdx], 0
    jne native_conveyor_potion
    mov r9, [rcx + 0x1d0]
    cmp dword ptr [r9 + 0x14], 0
    jle native_conveyor_potion
    movups xmm0, [r8 + 0xc0]
    subps xmm0, [rcx + 0xc0]
    mulps xmm0, [rcx + 0xb0]
    movaps xmm1, xmm0
    shufps xmm1, xmm1, 0x55
    addss xmm0, xmm1
    movaps xmm1, xmm0
    # z component was preserved in xmm0 by scalar addss.
    shufps xmm1, xmm1, 0xaa
    addss xmm0, xmm1
    comiss xmm0, [rcx + 0x168]
    jae native_conveyor_potion
    lea rcx, [rbx + 0x160]
    .byte 0xe9
    rip_ref 0x08c820
native_conveyor_potion:
    mov edx, 37
    jmp rax

# Egg actors borrow the matching regular actors during their initialization.
# Restore the farm's stock IDs before the first of the four actors borrows one.
#if EGG_TRACE_TEST
.macro trace_egg_ids destination
    live_lea 0x05, 0x18d23000
    xor r8d, r8d
    xor ecx, ecx
    live_lea 0x15, 0x1d0e7f0
    mov r9d, [rdx + 0x54]
.Ltrace_ids\@:
    cmp ecx, r9d
    jae .Ltrace_done\@
    mov rdx, [rax + rcx * 8]
    test rdx, rdx
    jz .Ltrace_next\@
    mov edx, [rdx + 0x600]
    inc edx
    push rcx
    shl ecx, 3
    shl edx, cl
    pop rcx
    or r8d, edx
.Ltrace_next\@:
    inc ecx
    cmp ecx, 4
    jb .Ltrace_ids\@
.Ltrace_done\@:
    mov [r11 + \destination], r8d
.endm
#endif
prepare_egg_roster:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
#if EGG_TRACE_TEST
    live_lea 0x05, FIONA_MODEL_RVA
    mov edx, [rax + 104]
    and edx, 3
    mov [rsp + 0x30], edx
    inc dword ptr [rax + 104]
    imul edx, edx, 24
    lea r11, [rax + rdx]
    mov rdx, [rcx + 0x7b0]
    live_lea 0x05, 0
    sub rdx, rax
    mov [r11], edx
    live_lea 0x05, 0x1d0e7f0
    mov edx, [rax + 0x54]
    mov [r11 + 4], edx
    live_lea 0x05, EGG_ROSTER_ACTIVE_RVA
    mov edx, [rax]
    mov [r11 + 12], edx
    trace_egg_ids 8
#endif
    live_lea 0x05, 0x1d0e7f0
    cmp dword ptr [rax + 8], 6
    jne snatch_egg_player
    live_lea 0x05, EGG_ROSTER_ACTIVE_RVA
    cmp dword ptr [rax], 0
    jne snatch_egg_player
    mov dword ptr [rax], 1
    live_lea 0x05, FARM_ROSTER_RVA
    cmp dword ptr [rax], 0
    je snatch_egg_player
    locate_rdx DRAFT_RVA
    mov dword ptr [rdx], 0x07030201 # Shrek, Donkey, Gingy, Puss
    mov edx, 1
    call apply_swap_draft
snatch_egg_player:
    mov rcx, [rsp + 0x20]
#if EGG_TRACE_TEST
    live_call 0x0d2150
    mov [rsp + 0x28], eax
    live_lea 0x05, FIONA_MODEL_RVA
    mov edx, [rsp + 0x30]
    imul edx, edx, 24
    lea r11, [rax + rdx]
    mov edx, [rsp + 0x28]
    mov [r11 + 16], edx
    trace_egg_ids 20
    mov eax, [rsp + 0x28]
    add rsp, 0x38
    ret
#else
    add rsp, 0x38
    .byte 0xe9
    rip_ref 0x0d2150
#endif
prepare_egg_roster_end:

# Leaving the farm ends this session. In-chapter rebuilds preserve the picks.
set_farm_level:
#if FACTORY_POTION_TEST
    live_lea 0x05, POTION_TEST_PENDING_RVA
    mov dword ptr [rax], 0
#endif
    cmp edx, 6
    je continue_set_farm_level
    live_lea 0x05, FARM_ROSTER_RVA
    mov qword ptr [rax], 0
continue_set_farm_level:
    cmp edx, 7
    je continue_set_campaign_level
    live_lea 0x05, RED_RETRY_RVA
    mov qword ptr [rax], 0
continue_set_campaign_level:
    mov r10d, [rcx + 0xc]
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0
    .byte 0xe9
    rip_ref 0x22e7db

# Test entry uses the authored potion checkpoint once per selected level.
#if FACTORY_POTION_TEST
# RestoreCheckPoint and first-frame setup can place the original party again.
# Complete the test entry once normal gameplay owns the initialized world.
finish_factory_potion_test:
    sub rsp, 0x38
    live_lea 0x05, POTION_TEST_PENDING_RVA
    cmp dword ptr [rax], 2
    jne potion_test_entry_done
    live_lea 0x0d, 0x1d0e7f0
    cmp dword ptr [rcx + 8], ENTRY_TEST_CHAPTER
    jne potion_test_entry_done
    reject_scripted_swap potion_test_entry_done
    mov ecx, ENTRY_TEST_CHECKPOINT
    live_call 0x0974a0
    test rax, rax
    jz potion_test_entry_done
    mov [rsp + 0x28], rax
    live_lea 0x05, POTION_TEST_PENDING_RVA
    mov dword ptr [rax], 3
    mov rcx, [rsp + 0x28]
    live_call 0x097800
    mov rcx, [rsp + 0x28]
    xor edx, edx
    live_call 0x098670
potion_test_entry_done:
    add rsp, 0x38
    ret
finish_factory_potion_test_end:

start_factory_potion_test:
    live_lea 0x05, POTION_TEST_PENDING_RVA
    # Both pending (2) and completed (3) belong to the current selection.
    # A checkpoint retry must never re-arm a completed testing shortcut.
    cmp dword ptr [rax], 0
    jne continue_factory_checkpoint_load
    .byte 0x83, 0x3d
    .long 0x1d0e7f8 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte ENTRY_TEST_CHAPTER
    jne continue_factory_checkpoint_load
    mov dword ptr [rax], 2
    live_lea 0x05, 0x5bfa90
    mov dword ptr [rax], ENTRY_TEST_CHECKPOINT
continue_factory_checkpoint_load:
    # Apply after scene and controller initialization, immediately before
    # CreateGame resolves the requested checkpoint from the loaded world.
    .byte 0x8b, 0x0d
    rip_ref 0x5bfa90
    .byte 0xe9
    rip_ref 0x22bf9e
#endif

# Every world rebuild ends the previous egg round. If this load creates
# another round, egg actor initialization will set the flag again.
reset_egg_on_world_create:
    live_lea 0x05, RUN_TEAM_RVA
    mov qword ptr [rax], 0
    mov dword ptr [rax + 8], 0
    live_lea 0x05, RED_RETRY_RVA
    mov qword ptr [rax + 8], 0
    cmp dword ptr [rax], 4
    jne reset_egg_roster
    mov dword ptr [rax], 0
reset_egg_roster:
    live_lea 0x05, EGG_ROSTER_ACTIVE_RVA
    mov dword ptr [rax], 0
    mov [rsp + 8], rbx
    .byte 0xe9
    rip_ref 0x22bd95

# StartGame itself CREATES the four EggPlayer actors through
# ShrekWorld::CreateRootObject_Networked (0x241f80). Do not preflight by
# calling that function: it consumes the regular party before native startup
# and makes the native second creation fail. Let startup run exactly once.
start_checked_egg_game:
    .byte 0xe9
    rip_ref 0x0d2480
start_checked_egg_game_end:
# Run after the native cage attachment transform is copied to the player.
# Only supported characters held by an actual ground Cage get this pose.
fit_player_in_ground_cage:
    sub rsp, 0x38
    mov eax, [rdi + 0x600]
    cmp eax, 9
    ja finish_ground_cage_pose
    # Lil' Red, Shrek & Donkey, Puss, and Wolf use fitted cage poses.
    mov edx, (1 << 4) | (1 << 5) | (1 << 6) | (1 << 9)
    bt edx, eax
    jnc finish_ground_cage_pose
    mov rax, [rdi + 0x6e8]
    test rax, rax
    jz finish_ground_cage_pose
    mov rax, [rax + 0x38]
    test rax, rax
    jz finish_ground_cage_pose
    mov rax, [rax + 0x38]
    test rax, rax
    jz finish_ground_cage_pose
    live_lea 0x15, 0x134840
    cmp [rax + 0x10], rdx
    jne finish_ground_cage_pose
    cmp [rax + 0x1e0], rdi
    jne finish_ground_cage_pose

    cmp dword ptr [rdi + 0x600], 6
    jne use_standard_cage_pose
    call hold_puss_cage_knockout
    test al, al
    jnz clear_ground_cage_birds
use_standard_cage_pose:
    # The second entry in the character's death animation table is the idle pose.
    mov rax, [rdi + 0x4e0]
    test rax, rax
    jz finish_ground_cage_pose
    mov edx, [rax + 4]
    test edx, edx
    js finish_ground_cage_pose
    cmp [rdi + 0x2a4], edx
    je clear_ground_cage_birds
    mov rax, [rdi]
    mov rcx, rdi
    xor r8d, r8d
    mov r9d, -1
    mov byte ptr [rsp + 0x20], 1
    mov byte ptr [rsp + 0x28], 0
    call qword ptr [rax + 0x50]
clear_ground_cage_birds:
    cmp qword ptr [rdi + 0x708], 0
    je align_player_in_cage
    lea rcx, [rdi + 0x708]
    live_call 0x08c820
align_player_in_cage:
    # Advance along the cage attachment's horizontal forward axis.
    mov eax, [rdi + 0x600]
    cmp eax, 5
    je adjust_cage_height
    movss xmm0, [rip + red_cage_forward]
    cmp eax, 9
    jne adjust_cage_position
    movss xmm0, [rip + wolf_cage_forward]
adjust_cage_position:
    movss xmm1, [rdi + 0xb0]
    mulss xmm1, xmm0
    addss xmm1, [rdi + 0xc0]
    movss [rdi + 0xc0], xmm1
    movss xmm1, [rdi + 0xb8]
    mulss xmm1, xmm0
    addss xmm1, [rdi + 0xc8]
    movss [rdi + 0xc8], xmm1
adjust_cage_height:
    movss xmm1, [rdi + 0xc4]
    cmp dword ptr [rdi + 0x600], 6
    jne adjust_standard_cage_height
    addss xmm1, [rip + puss_cage_height]
    jmp store_ground_cage_height
adjust_standard_cage_height:
    addss xmm1, [rip + ground_cage_height]
store_ground_cage_height:
    movss [rdi + 0xc4], xmm1
finish_ground_cage_pose:
    lea rcx, [rdi + 0x390]
    mov rdx, rdi
    add rsp, 0x38
    .byte 0xe9
    rip_ref 0x08d7c0
fit_player_in_ground_cage_end:
red_cage_forward:
    .float 0.24
wolf_cage_forward:
    .float -0.39
ground_cage_height:
    .float -0.22

puss_cage_height:
    .float 0.02

.set PUSS_CAGE_TRACK_RVA, PUSS_CAGE_ANIMATION_RVA + 8
.set PUSS_CAGE_TABLE_RVA, PUSS_CAGE_ANIMATION_RVA + 16

# Ogre Killer's final defeat goes from clip 23 to the knockout hold at 6.
# Load the original boss asset on demand, using the engine's ANI converter.
load_puss_cage_knockout:
    sub rsp, 0x38
    lea rcx, [rip + puss_boss_animation_path]
    xor edx, edx
    live_call 0x2d09f0
    test rax, rax
    jz disable_puss_cage_knockout
    mov [rsp + 0x20], rax
    cmp dword ptr [rax], 24
    jne free_unused_puss_file
    mov rcx, rax
    live_call 0x2aa8b0
    test eax, eax
    jle free_unused_puss_file
    add eax, 15
    and eax, -16
    mov [rsp + 0x28], rax
    lea ecx, [rax + 0x108]
    live_call 0x2a8070
    test rax, rax
    jz free_unused_puss_file
    mov [rsp + 0x30], rax
    mov rdx, rax
    mov rcx, [rsp + 0x20]
    live_call 0x2aacd0
    mov rcx, [rsp + 0x20]
    live_call 0x2a8060

    mov rax, [rsp + 0x30]
    mov r10, [rax + 0x38]
    test r10, r10
    jz free_unused_puss_group
    mov ecx, [r10 + 0x10]
    lea eax, [rcx - 1]
    cmp eax, 127
    ja free_unused_puss_group
    lea r11, [r10 + 0x18]
find_puss_cage_track:
    mov rdx, [r11]
    cmp word ptr [rdx], 30
    je cache_puss_cage_animation
    add r11, 8
    dec ecx
    jnz find_puss_cage_track
free_unused_puss_group:
    mov rcx, [rsp + 0x30]
    live_call 0x2a8060
    jmp disable_puss_cage_knockout
free_unused_puss_file:
    mov rcx, [rsp + 0x20]
    live_call 0x2a8060
disable_puss_cage_knockout:
    live_lea 0x05, PUSS_CAGE_ANIMATION_RVA
    mov qword ptr [rax], -1
    add rsp, 0x38
    ret
cache_puss_cage_animation:
    live_lea 0x05, PUSS_CAGE_ANIMATION_RVA
    mov [rax], r10
    mov [rax + 8], rdx
    mov rdx, [rsp + 0x30]
    add rdx, [rsp + 0x28]
    mov [rax + 16], rdx
    add rsp, 0x38
    ret
load_puss_cage_knockout_end:

hold_puss_cage_knockout:
    sub rsp, 0x38
    live_lea 0x05, PUSS_CAGE_ANIMATION_RVA
    cmp qword ptr [rax], 0
    jne check_puss_cage_animation
    call load_puss_cage_knockout
check_puss_cage_animation:
    live_lea 0x05, PUSS_CAGE_ANIMATION_RVA
    cmp qword ptr [rax], -1
    je puss_cage_animation_unavailable
    mov r8, [rax + 8]
    mov r10, rdi
    mov r11d, 256
    # Photo Mode can rebind the ordinary clip while leaving the same
    # reported animation index. Check a bone track from the boss clip.
find_active_puss_cage_track:
    mov rax, [r10 + 0x70]
    test rax, rax
    jz next_puss_cage_node
    cmp [rax], r8
    je puss_cage_animation_ready
next_puss_cage_node:
    mov rax, [r10 + 0x30]
    test rax, rax
    jnz visit_puss_cage_node
next_puss_cage_sibling:
    cmp r10, rdi
    je bind_puss_cage_animation
    mov rax, [r10 + 0x28]
    test rax, rax
    jnz visit_puss_cage_node
    mov r10, [r10 + 0x38]
    test r10, r10
    jz puss_cage_animation_unavailable
    dec r11d
    jnz next_puss_cage_sibling
    jmp puss_cage_animation_unavailable
visit_puss_cage_node:
    mov r10, rax
    dec r11d
    jnz find_active_puss_cage_track
    jmp puss_cage_animation_unavailable

bind_puss_cage_animation:
    mov r9, [rdi + 0x288]
    test r9, r9
    jz puss_cage_animation_unavailable
    cmp dword ptr [r9], 32
    jne puss_cage_animation_unavailable
    mov [rsp + 0x30], r9
    live_lea 0x05, PUSS_CAGE_ANIMATION_RVA
    mov r10, [rax + 16]
    xor ecx, ecx
copy_puss_cage_animation_table:
    mov r11, [r9 + rcx * 8]
    mov [r10 + rcx * 8], r11
    inc ecx
    cmp ecx, 33
    jb copy_puss_cage_animation_table
    mov rax, [rax]
    mov [r10 + 0x38], rax
    mov [rdi + 0x288], r10
    mov qword ptr [rdi + 0x2d8], 0
    mov rax, [rdi]
    mov rcx, rdi
    mov edx, 6
    xor r8d, r8d
    mov r9d, -1
    mov byte ptr [rsp + 0x20], 1
    mov byte ptr [rsp + 0x28], 0
    call qword ptr [rax + 0x50]
    mov rax, [rsp + 0x30]
    mov [rdi + 0x288], rax
puss_cage_animation_ready:
    mov al, 1
    add rsp, 0x38
    ret
puss_cage_animation_unavailable:
    xor eax, eax
    add rsp, 0x38
    ret
hold_puss_cage_knockout_end:
puss_boss_animation_path:
    .asciz "gamedata\\chars\\pibboss.ani"

#if SWAMP_EYEBALL_TEST
# The counter's first update runs after the level has registered its pickups.
# Use the native completion callback to unlock the route and advance the goal.
complete_swamp_eyeballs_for_test:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    live_call 0x13e7a0
    mov [rsp + 0x28], eax
    live_lea 0x05, 0x1d0e7f0
    cmp dword ptr [rax + 8], 1
    jne return_swamp_counter_update
    mov rcx, [rsp + 0x20]
    live_lea 0x05, 0x1cd6780
    cmp [rax], rcx
    jne return_swamp_counter_update
    cmp byte ptr [rcx + 0x170], 0
    jne return_swamp_counter_update
    cmp byte ptr [rcx + 0x18c], 0
    jne return_swamp_counter_update
    live_lea 0x05, 0x1cd6758
    mov edx, [rax]
    test edx, edx
    jle return_swamp_counter_update
    live_lea 0x05, 0x1cd675c
    mov [rax], edx
    mov [rcx + 0x188], edx
    live_call 0x135870
return_swamp_counter_update:
    mov eax, [rsp + 0x28]
    add rsp, 0x38
    ret
complete_swamp_eyeballs_for_test_end:
#endif
