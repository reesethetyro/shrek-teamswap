.intel_syntax noprefix
.text
.globl install_team_menu
.macro rip_ref target
    .long \target - (CODE_RVA + (. - install_team_menu) + 4)
.endm
.macro locate_rdx target
    .byte 0x48, 0x8d, 0x15
    rip_ref \target
.endm
.macro copy_entry source, destination
    lea r8, [r10 + 0xe8 * \source]
    lea r9, [r11 + 0xe8 * \destination]
    mov eax, 0xe8 / 8
.Lcopy\@:
    mov rdx, [r8]
    mov [r9], rdx
    add r8, 8
    add r9, 8
    dec eax
    jne .Lcopy\@
.endm
.set DRAFT_RVA, PAGE_RVA + 15 * 0xe8 + 0x50
.set LOCK_RVA, DRAFT_RVA + 8
.macro reject_unavailable_actor actor, destination
    cmp byte ptr [\actor + 0x1d], 0x0e
    je \destination
    # Captive players retain their HP, but the trap still owns their actor.
    cmp byte ptr [\actor + 0x1d], 0x17
    je \destination
    xorps xmm0, xmm0
    ucomiss xmm0, dword ptr [\actor + 0x2f4]
    jae \destination
.endm
.macro reject_scripted_swap destination
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x1d0e7f0
    # The pause book leaves the controller's gameplay mode intact.
    # A cutscene can also claim the controller before the mode changes.
    cmp dword ptr [r10], 0
    jne \destination
    cmp qword ptr [r10 + 0x160], 0
    jne \destination
    .byte 0x83, 0x3d
    .long EGG_ROSTER_ACTIVE_RVA - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 0
    jne \destination
    .byte 0x83, 0x3d
    .long RUN_TEAM_RVA + 8 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 0
    jne \destination
    # Solo alternate scenes load before Hero Time mode is set.
    cmp dword ptr [r10 + 8], 2
    je .Lcheck_solo_scene\@
    cmp dword ptr [r10 + 8], 13
    je .Lcheck_solo_scene\@
    cmp dword ptr [r10 + 8], 14
    jne .Lswap_mode_checked\@
.Lcheck_solo_scene\@:
    .byte 0x4c, 0x8b, 0x15
    rip_ref 0x1d0e6c0
    .byte 0x4c, 0x3b, 0x15
    rip_ref 0x1d0e6d0
    jne \destination
.Lswap_mode_checked\@:
.endm
.macro check_mongo_swamp destination
    .byte 0x83, 0x3d
    .long 0x1d0e7f8 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 1
    jne \destination
.endm
# Entered after the native Team initializer has restored its stack.
# Keep RCX, the destructor callback argument, for the original tail call.
install_team_menu:
    locate_rdx 0x1cf6490
    .byte 0x48, 0x39, 0x15
    rip_ref (0x5b6370 + 115 * 8)
    jne finish
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x1cddc30
    .byte 0x4c, 0x8d, 0x1d
    rip_ref PAGE_RVA

    # Character Info initializes immediately after Team and fills this table.
    .byte 0x4c, 0x89, 0x1d
    rip_ref (0x5b6370 + 115 * 8)
    # Move the existing options down one row, using the empty spacer.
    mov r11, r10
    copy_entry 3, 4
    copy_entry 2, 3
    copy_entry 1, 2
    # Each entry carries its own explicit vertical position.
    add dword ptr [r10 + 2 * 0xe8 + 0x68], 30
    add dword ptr [r10 + 3 * 0xe8 + 0x68], 30
    add dword ptr [r10 + 4 * 0xe8 + 0x68], 30
    locate_rdx (CODE_RVA + label - install_team_menu)
    mov [r10 + 0xe8], rdx
    mov dword ptr [r10 + 0xe8 + 8], LABEL_HASH
    mov dword ptr [r10 + 0xe8 + 12], LABEL_LENGTH
    mov dword ptr [r10 + 0xe8 + 0x18], 115
finish:
    .byte 0xe9
    rip_ref 0x37dc00

# Entered after the native Character Info constructor completes. Clone all
# four active-character rows and their paired portrait commands, along with
# the title, spacer, Back row, help text and terminator.
initialize_swap_page:
    locate_rdx PAGE_RVA
    .byte 0x48, 0x39, 0x15
    rip_ref (0x5b6370 + 115 * 8)
    jne finish_page
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x1cdd060
    mov r11, rdx
    mov eax, 13 * 0xe8 / 8
copy_page:
    mov r8, [r10]
    mov [r11], r8
    add r10, 8
    add r11, 8
    dec eax
    jne copy_page
    locate_rdx (CODE_RVA + label - install_team_menu)
    .byte 0x4c, 0x8d, 0x1d
    rip_ref PAGE_RVA
    mov [r11], rdx
    mov dword ptr [r11 + 8], LABEL_HASH
    mov dword ptr [r11 + 12], LABEL_LENGTH
    # Remove the confirm actions from each name and its companion entry.
    # Retain its type and active-character expression for cursor navigation.
    lea r10, [r11 + 0xe8]
    mov eax, 8
clear_confirm:
    mov dword ptr [r10 + 0xb8], 0
    add r10, 0xe8
    dec eax
    jne clear_confirm

    # Back and the native Y Restore All Defaults prompt.
    lea r10, [r11 + 11 * 0xe8]
    lea rdx, [rip + back_prompt]
    mov [r10], rdx
    mov dword ptr [r10 + 8], BACK_PROMPT_HASH
    mov dword ptr [r10 + 12], BACK_PROMPT_LENGTH
    movups xmm0, [r10 + 0x28]
    movups [r10 + 0x18], xmm0
    locate_rdx 0x494220
    mov [r10 + 0x28], rdx
    mov dword ptr [r10 + 0x30], 5
    mov dword ptr [r10 + 0x34], 0
    # A separate menu on the right page uses its own 320-wide coordinates.
    lea r10, [r11 + 13 * 0xe8]
    mov dword ptr [r10 + 0x10], 3
    mov dword ptr [r10 + 0x58], 64
    mov dword ptr [r10 + 0x68], 112
    mov dword ptr [r10 + 0x78], 192
    mov dword ptr [r10 + 0x88], 192
    mov dword ptr [r10 + 0xe8 + 0x10], 10
    mov dword ptr [r10 + 2 * 0xe8], 108
    mov dword ptr [r10 + 2 * 0xe8 + 0x4c], -1
finish_page:
    .byte 0xe9
    rip_ref 0x37dc00

# Native screens also request the empty page at slot 115. Draw the roster
# only for the active menu state opened from Team. The preview's synthetic
# state is private to the right-page renderer and needs no global registration.
select_swap_draw_entries:
    cmp eax, 115
    je check_swap_draw_context
    cmp eax, 108
    jne load_native_draw_entries
    .byte 0x4c, 0x8d, 0x15
    rip_ref (PAGE_RVA + 15 * 0xe8)
    cmp rdx, r10
    jne load_native_draw_entries
    .byte 0x4c, 0x8d, 0x3d
    rip_ref (PAGE_RVA + 13 * 0xe8)
    jmp continue_draw_entries
check_swap_draw_context:
    movsxd r10, dword ptr [rbp + 0x5d8]
    test r10d, r10d
    jle load_empty_draw_entries
    lea r10, [r10 + r10 * 4]
    shl r10, 4
    lea r10, [rbp + r10 + 0x38]
    cmp rdx, r10
    jne load_empty_draw_entries
    cmp dword ptr [rdx - 0x50], 133
    je load_native_draw_entries
load_empty_draw_entries:
    .byte 0x4c, 0x8d, 0x3d
    rip_ref 0x1cf6490
    jmp continue_draw_entries
load_native_draw_entries:
    mov r15, [rcx + rax * 8 + 0x5b6370]
continue_draw_entries:
    .byte 0xe9
    rip_ref 0x259b83

# DrawState uses style 15 for the roster (ribbon only) and style 0 for its
# right-page portrait (plain parchment).
swap_page_style:
    cmp eax, 115
    jne check_portrait_style
    .byte 0x4c, 0x8d, 0x15
    rip_ref PAGE_RVA
    cmp r15, r10
    jne draw_normal
    mov ecx, 15
    jmp draw_normal
check_portrait_style:
    cmp eax, 108
    jne draw_normal
    .byte 0x4c, 0x8d, 0x15
    rip_ref (PAGE_RVA + 13 * 0xe8)
    cmp r15, r10
    jne draw_normal
    xor ecx, ecx
draw_normal:
    lea rax, [rdx + 0x5b67a0]
    .byte 0xe9
    rip_ref 0x259d74

# RenderMsg already selected the right-hand page's render target. Substitute
# only its draw state, leaving the real menu stack and input context intact.
render_right_preview:
    cmp dword ptr [rdi + 0x160], 1
    jne render_page
    .byte 0x4c, 0x8b, 0x15
    rip_ref 0x1d0e680
    test r10, r10
    je render_page
    add r10, 0x178
    movsxd r11, dword ptr [r10 + 0x5d8]
    test r11d, r11d
    jle render_page
    lea r11, [r11 + 4 * r11]
    shl r11, 4
    add r10, r11
    cmp dword ptr [r10 + 0x38], 115
    jne render_page
    mov r11d, [r10 + 0x84]
    .byte 0x4c, 0x8d, 0x15
    rip_ref (PAGE_RVA + 13 * 0xe8)
    mov dword ptr [r10 + 0x10], 3
    sub r11d, 1
    cmp r11d, 6
    ja use_preview_state
    test r11b, 1
    jne use_preview_state
    shl r11d, 4
    mov eax, r11d
    shr eax, 5
    .byte 0x4c, 0x8d, 0x1d
    rip_ref DRAFT_RVA
    movzx r11d, byte ptr [r11 + rax]
    test r11d, r11d
    jnz render_candidate
#if DRAGON_SWAMP_TEST
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x18d23000
    mov r11, [r11 + 8 * rax]
    test r11, r11
    jz use_native_portrait
    cmp dword ptr [r11 + 0x600], 7
    jne use_native_portrait
    mov r11d, CANDIDATE_COUNT
    jmp render_candidate
use_native_portrait:
#endif
    mov r11d, eax
    shl r11d, 5
    lea rdx, [rip + portrait_labels]
    jmp set_portrait
render_candidate:
    dec r11d
    shl r11d, 5
    lea rdx, [rip + candidate_portraits]
set_portrait:
    add rdx, r11
    mov [r10], rdx
    mov r11, [rdx + 24]
    mov [r10 + 8], r11
    mov dword ptr [r10 + 0x10], 6
use_preview_state:
    lea rdx, [r10 + 2 * 0xe8]
render_page:
    .byte 0xe9
    rip_ref 0x259b50

# Restore native labels when the player enters Swap Team again.
reset_swap_draft:
    cmp edx, 133
    jne check_swap_menu
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x1cddc30
    mov dword ptr [r10 + 0xe8 + 0x10], 2
    mov eax, 140
    .byte 0x83, 0x3d
    .long 0x1d0e7f8 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 4
    jne space_team_rows
    mov dword ptr [r10 + 0xe8 + 0x10], 3
    mov eax, 110
space_team_rows:
    mov [r10 + 2 * 0xe8 + 0x68], eax
    add eax, 30
    mov [r10 + 3 * 0xe8 + 0x68], eax
    add eax, 30
    mov [r10 + 4 * 0xe8 + 0x68], eax
    jmp load_menu
check_swap_menu:
    cmp edx, 115
    jne load_menu
    .byte 0x4c, 0x8d, 0x15
    rip_ref DRAFT_RVA
    mov dword ptr [r10], 0
    .byte 0x4c, 0x8d, 0x15
    rip_ref (0x1cdd060 + 0xe8)
    .byte 0x4c, 0x8d, 0x1d
    rip_ref (PAGE_RVA + 0xe8)
    mov eax, 4
reset_label:
    movups xmm0, [r10]
    movups [r11], xmm0
    add r10, 2 * 0xe8
    add r11, 2 * 0xe8
    dec eax
    jne reset_label
    .byte 0x4c, 0x8d, 0x15
    rip_ref LOCK_RVA
    mov dword ptr [r10], 0
    xor eax, eax
refresh_row_status:
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x18d23000
    mov r11, [r11 + rax * 8]
    test r11, r11
    jz next_row_status
    reject_unavailable_actor r11, disable_row_status
    jmp next_row_status
disable_row_status:
    bts dword ptr [r10], eax
next_row_status:
    inc eax
    cmp eax, 4
    jb refresh_row_status
load_menu:
    mov [rsp + 8], rbx
    .byte 0xe9
    rip_ref 0x25b915

# Tint locked names while keeping Up/Down navigation available.
draw_fallen_row:
    .byte 0x44, 0x8b, 0x1d
    rip_ref LOCK_RVA
    reject_scripted_swap tint_all_team_rows
    jmp locate_locked_rows
tint_all_team_rows:
    mov r11d, 0xf
locate_locked_rows:
    .byte 0x4c, 0x8d, 0x15
    rip_ref (PAGE_RVA + 0xe8)
check_fallen_row:
    test r11d, r11d
    jz draw_native_row
    cmp r13, r10
    jne next_fallen_row
    test r11b, 1
    jz draw_native_row
    .byte 0x8b, 0x05
    rip_ref 0x5e46b4
    .byte 0xe9
    rip_ref 0x257f46
next_fallen_row:
    shr r11d, 1
    add r10, 2 * 0xe8
    jmp check_fallen_row
draw_native_row:
    cmp byte ptr [rbp + 0x480], 0
    .byte 0xe9
    rip_ref 0x257f32

cycle_left:
    movsxd rdx, dword ptr [rbx + 0x5d8]
    lea rcx, [rdx + 4 * rdx]
    add rcx, rcx
    cmp dword ptr [rbx + 8 * rcx + 0x38], 115
    jne normal_left
    mov r8d, [rbx + 8 * rcx + 0x84]
    mov r9d, -1
    jmp cycle_character
normal_left:
    .byte 0xe9
    rip_ref 0x25da91

cycle_right:
    cmp ecx, 115
    jne normal_right
    mov r8d, [rbx + 8 * rdx + 0x84]
    mov r9d, 1
    jmp cycle_character
normal_right:
    cmp ecx, 0x1f
    je scroll_native_right
    .byte 0xe9
    rip_ref 0x25dc51
scroll_native_right:
    .byte 0xe9
    rip_ref 0x25dbe4

cycle_character:
    reject_scripted_swap cycle_done
    sub r8d, 1
    cmp r8d, 6
    ja cycle_done
    test r8b, 1
    jnz cycle_done
    shr r8d, 1
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x18d23000
    mov r10, [r10 + r8 * 8]
    test r10, r10
    jz cycle_done
    reject_unavailable_actor r10, cycle_done
    # Build a mask of characters currently shown in the other three rows.
    .byte 0x4c, 0x8d, 0x15
    rip_ref DRAFT_RVA
    xor ecx, ecx
    xor edx, edx
collect_displayed:
    cmp edx, r8d
    je next_displayed
    cmp edx, 4
    jae displayed_unedited
    movzx eax, byte ptr [r10 + rdx]
    test eax, eax
    jnz displayed_candidate
displayed_unedited:
    .byte 0x4c, 0x8d, 0x1d
    rip_ref 0x18d23000
    mov r11, [r11 + 8 * rdx]
    test r11, r11
    jz next_displayed
    mov eax, [r11 + 0x600]
    jmp mark_displayed
displayed_candidate:
    lea r11, [rip + candidate_ids]
    movzx eax, byte ptr [r11 + rax - 1]
mark_displayed:
    cmp eax, 9
    ja next_displayed
    bts ecx, eax
next_displayed:
    inc edx
    .byte 0x3b, 0x15
    rip_ref 0x1d0e844
    jae displayed_done
    cmp edx, 9
    jb collect_displayed
displayed_done:
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x18d23000
    mov rax, [rax + r8 * 8]
    mov eax, [rax + 0x600]
    xor edx, edx
    cmp eax, 9
    ja find_displayed_position
    lea rdx, [rip + candidate_positions]
    movzx edx, byte ptr [rdx + rax]
find_displayed_position:
    movzx r11d, byte ptr [r10 + r8]
    test r11d, r11d
    cmovz r11d, edx
    # Traverse the ordered list from the displayed character. An unlisted
    # original character keeps its own position at the wraparound boundary.
next_candidate:
    add r11d, r9d
    cmp r11d, CANDIDATE_COUNT
    jle check_low
    xor r11d, r11d
check_low:
    test r11d, r11d
    jns check_candidate
    mov r11d, CANDIDATE_COUNT
check_candidate:
    test r11d, r11d
    jnz check_original_position
    test edx, edx
    jnz next_candidate
check_original_position:
    cmp r11d, edx
    jne check_alternative
    cmp byte ptr [r10 + r8], 0
    je cycle_done
    .byte 0x48, 0x8d, 0x05
    rip_ref 0x18d23000
    mov rax, [rax + r8 * 8]
    mov eax, [rax + 0x600]
    cmp eax, 9
    ja next_candidate
    bt ecx, eax
    jc next_candidate
    # Draft index zero preserves the native actor, including special forms.
    xor r11d, r11d
    jmp remember_cycle_candidate
check_alternative:
    cmp r11b, [r10 + r8]
    je cycle_done
    lea rax, [rip + candidate_ids - 1]
    movzx eax, byte ptr [rax + r11]
#if DRAGON_SWAMP_TEST
    cmp eax, 7
    jne check_candidate_available
    .byte 0x8b, 0x05
    rip_ref 0x1d0e7f8
    cmp eax, 15
    ja next_candidate
    bt dword ptr [rip + campaign_swap_chapters], eax
    jnc next_candidate
    mov eax, 7
check_candidate_available:
#endif
    bt ecx, eax
    jc next_candidate
remember_cycle_candidate:
    mov [r10 + r8], r11b
    imul r8d, 2 * 0xe8
    .byte 0x4c, 0x8d, 0x15
    rip_ref (PAGE_RVA + 0xe8)
    add r10, r8
    test r11d, r11d
    jnz show_candidate_name
    locate_rdx (0x1cdd060 + 0xe8)
    movups xmm0, [rdx + r8]
    movups [r10], xmm0
    jmp play_cycle_highlight
show_candidate_name:
    dec r11d
    shl r11d, 6
    lea rdx, [rip + candidate_names]
    add rdx, r11
    mov [r10], rdx
    mov rax, [rdx + 56]
    mov [r10 + 8], rax
play_cycle_highlight:
    # Use the same HUD sound as successful Up/Down navigation.
    .byte 0x48, 0x8b, 0x0d
    rip_ref 0x1d0e688
    test rcx, rcx
    jz cycle_done
    .byte 0xe8
    rip_ref 0x129ea0
cycle_done:
    .byte 0xe9
    rip_ref 0x25e41c
label:
    .asciz "SwapTeamMenu"
back_prompt:
    .asciz "$Back $1   $RestoreAllDefaults $2"
.macro portrait_label slot, hash
    .asciz "$ActiveCharTexInt(\slot)"
    .zero 24 - 21
    .long \hash, 20
.endm
portrait_labels:
    portrait_label 0, PORTRAIT_0_HASH
    portrait_label 1, PORTRAIT_1_HASH
    portrait_label 2, PORTRAIT_2_HASH
    portrait_label 3, PORTRAIT_3_HASH
.macro candidate_label value, count, hash
    .asciz "\value"
    .zero 23 - \count
    .long \hash, \count
.endm
candidate_ids:
    .byte CANDIDATE_IDS
candidate_positions:
    .byte CANDIDATE_POSITIONS
#if DRAGON_SWAMP_TEST
campaign_swap_chapters:
    .long 0xe8ee
#endif
.macro candidate_name value, count, hash
    .asciz "\value"
    .zero 55 - \count
    .long \hash, \count
.endm
candidate_names:
    candidate_name NAME_0, NAME_0_LENGTH, NAME_0_HASH
    candidate_name NAME_1, NAME_1_LENGTH, NAME_1_HASH
    candidate_name NAME_2, NAME_2_LENGTH, NAME_2_HASH
    candidate_name NAME_3, NAME_3_LENGTH, NAME_3_HASH
    candidate_name NAME_4, NAME_4_LENGTH, NAME_4_HASH
    candidate_name NAME_5, NAME_5_LENGTH, NAME_5_HASH
    candidate_name NAME_6, NAME_6_LENGTH, NAME_6_HASH
    candidate_name NAME_7, NAME_7_LENGTH, NAME_7_HASH
    candidate_name NAME_8, NAME_8_LENGTH, NAME_8_HASH
#if DRAGON_SWAMP_TEST
    candidate_name NAME_9, NAME_9_LENGTH, NAME_9_HASH
#endif
candidate_portraits:
    candidate_label TEX_0, 2, TEX_0_HASH
    candidate_label TEX_1, 2, TEX_1_HASH
    candidate_label TEX_2, 2, TEX_2_HASH
    candidate_label TEX_3, 2, TEX_3_HASH
    candidate_label TEX_4, 2, TEX_4_HASH
    candidate_label TEX_5, 2, TEX_5_HASH
    candidate_label TEX_6, 2, TEX_6_HASH
    candidate_label TEX_7, 2, TEX_7_HASH
    candidate_label TEX_8, 2, TEX_8_HASH
#if DRAGON_SWAMP_TEST
    candidate_label TEX_9, 2, TEX_9_HASH
#endif
#include "cinema_cast.inc"
#include "bench_pickups.inc"
#include "wolf_forest.inc"
#include "water_cannon.inc"
#if DRAGON_SWAMP_TEST
#include "dragon_swamp.inc"
#endif
#include "live_swap.inc"
#include "friar_chicken.inc"
#include "snail_projectiles.inc"
# A flying Fairy cannot trip a ground cage or become its restored captive.
check_fairy_cage_restore:
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x107d40
    cmp [rdi + 0x10], r10
    je skip_fairy_cage_restore
    xor edx, edx
    mov [rsp + 0xb0], rbp
    .byte 0xe9
    rip_ref 0x135a1c
skip_fairy_cage_restore:
    .byte 0xe9
    rip_ref 0x135c15

check_fairy_cage_contact:
    mov rax, [rsi + 0x10]
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x107d40
    cmp rax, r10
    je skip_fairy_cage_contact
    test rax, rax
    .byte 0xe9
    rip_ref 0x141af3
skip_fairy_cage_contact:
    .byte 0xe9
    rip_ref 0x14183a

# AllocChannel must receive a loaded bank and a successful sub-sound lookup
# before passing the result to FMOD playback. Reuse its native failure return.
check_sound_bank:
    mov rcx, [rsi + 8 * rcx + 0x3dc27e0]
    test rcx, rcx
    jz sound_lookup_failed
    mov qword ptr [rsp + 0x38], 0
    .byte 0xe9
    rip_ref 0x2cb354
check_sound_lookup:
    test eax, eax
    jne sound_lookup_failed
    cmp qword ptr [rsp + 0x38], 0
    je sound_lookup_failed
    mov r8, [rsi + 8 * rbx + 0x3dc27b0]
    .byte 0xe9
    rip_ref 0x2cb361
sound_lookup_failed:
    .byte 0xe9
    rip_ref 0x2cb3b9

gameplay_cycle_player:
    movsxd rax, ecx
    .byte 0x4c, 0x8d, 0x05
    rip_ref 0x18d23000
    mov r8, [r8 + rax * 8]
    .byte 0xe9
    rip_ref 0x237add
    .long initialize_swap_page - install_team_menu
    .long swap_page_style - install_team_menu
    .long render_right_preview - install_team_menu
    .long reset_swap_draft - install_team_menu
    .long cycle_left - install_team_menu
    .long cycle_right - install_team_menu
    .long add_player_during_stage - install_team_menu
    .long leave_swap_menu - install_team_menu
    .long apply_before_back - install_team_menu
    .long original_back - install_team_menu
    .long apply_before_resume - install_team_menu
    .long apply_swap_draft - install_team_menu
    .long apply_end - install_team_menu
    .long reset_hp_on_level_load - install_team_menu
    .long reset_hp_on_respawn - install_team_menu
    .long draw_fallen_row - install_team_menu
    .long gameplay_cycle_player - install_team_menu
    .long save_roster_before_respawn - install_team_menu
    .long restore_roster_after_respawn - install_team_menu
    .long respawn_restore_end - install_team_menu
    .long remember_cinema_actor - install_team_menu
    .long remember_cinema_end - install_team_menu
    .long resolve_cinema_actor - install_team_menu
    .long resolve_cinema_end - install_team_menu
    .long load_cinema_cast - install_team_menu
    .long load_cinema_end - install_team_menu
    .long retire_cinema_cast - install_team_menu
    .long retire_cinema_end - install_team_menu
    .long draw_cinema_cast - install_team_menu
    .long lookup_pickup_owner - install_team_menu
    .long lookup_pickup_end - install_team_menu
    .long show_benched_pickup - install_team_menu
    .long show_pickup_end - install_team_menu
    .long bench_pickups_allowed - install_team_menu
    .long bench_pickups_end - install_team_menu
    .long credit_pickup_owner - install_team_menu
    .long wolf_obstacle_collision - install_team_menu
    .long wolf_obstacle_damage - install_team_menu
    .long copy_obstacle_damage - install_team_menu
    .long copy_obstacle_damage_end - install_team_menu
    .long check_sound_bank - install_team_menu
    .long check_sound_lookup - install_team_menu
    .long check_fairy_cage_restore - install_team_menu
    .long check_fairy_cage_contact - install_team_menu
    .long finish_water_cannon_mount - install_team_menu
    .long filter_fairy_water_cannon_offer - install_team_menu
    .long reject_fairy_water_cannon_mount - install_team_menu
    .long check_friar_fairy_hit - install_team_menu
    .long play_grounded_flinch - install_team_menu
    .long play_grounded_flinch_end - install_team_menu
    .long finish_cinema_positions - install_team_menu
    .long move_swapped_cinema_players - install_team_menu
    .long move_swapped_cinema_players_end - install_team_menu
    .long set_snail_aim_height - install_team_menu
    .long start_campaign_hero_time - install_team_menu
    .long start_campaign_hero_end - install_team_menu
    .long finish_campaign_hero_time - install_team_menu
    .long finish_campaign_hero_end - install_team_menu
    .long prepare_egg_roster - install_team_menu
    .long prepare_egg_roster_end - install_team_menu
    .long set_farm_level - install_team_menu
    .long reset_egg_on_world_create - install_team_menu
    .long start_checked_egg_game - install_team_menu
    .long start_checked_egg_game_end - install_team_menu
    .long select_swap_draw_entries - install_team_menu
    .long fit_player_in_ground_cage - install_team_menu
    .long fit_player_in_ground_cage_end - install_team_menu
    .long load_puss_cage_knockout - install_team_menu
    .long load_puss_cage_knockout_end - install_team_menu
    .long hold_puss_cage_knockout - install_team_menu
    .long hold_puss_cage_knockout_end - install_team_menu
    .long prepare_campaign_hero - install_team_menu
    .long prepare_campaign_hero_end - install_team_menu
    .long start_red_hero_time - install_team_menu
    .long start_red_hero_time_end - install_team_menu
    .long retry_red_hero_time - install_team_menu
    .long win_red_hero_time - install_team_menu
    .long remember_red_challenge - install_team_menu
    .long queue_red_hero_time - install_team_menu
    .long setup_current_run_team - install_team_menu
    .long current_run_player - install_team_menu
    .long resolve_current_run_rect - install_team_menu
    .long resolve_current_run_follower - install_team_menu
    .long setup_factory_run_team - install_team_menu
    .long setup_current_run_team_end - install_team_menu
    .long release_run_reference - install_team_menu
    .long release_run_reference_end - install_team_menu
    .long current_run_player_end - install_team_menu
    .long finish_factory_run_without_dance - install_team_menu
    .long restore_finished_run_team - install_team_menu
    .long restore_finished_run_team_end - install_team_menu
    .long check_factory_fairy_attack - install_team_menu
    .long move_factory_conveyor_potion - install_team_menu
    .long prepare_swamp_rhythm_fiona - install_team_menu
    .long prepare_swamp_rhythm_fiona_end - install_team_menu
    .long restore_team_defaults_input - install_team_menu
    .long restore_team_defaults_input_end - install_team_menu
#if FACTORY_POTION_TEST
    .long start_factory_potion_test - install_team_menu
    .long finish_factory_potion_test - install_team_menu
    .long finish_factory_potion_test_end - install_team_menu
#endif
#if SWAMP_EYEBALL_TEST
    .long complete_swamp_eyeballs_for_test - install_team_menu
    .long complete_swamp_eyeballs_for_test_end - install_team_menu
#endif
#if DRAGON_SWAMP_TEST
    .long init_swamp_dragon - install_team_menu
    .long move_swamp_dragon - install_team_menu
    .long update_swamp_dragon - install_team_menu
    .long frame_swamp_dragon - install_team_menu
    .long attack_swamp_dragon - install_team_menu
    .long update_free_dragon_frame - install_team_menu
    .long update_free_dragon_frame_end - install_team_menu
    .long check_dragon_hazard - install_team_menu
    .long transition_swamp_dragon_animation - install_team_menu
    .long photo_swamp_dragon_portrait - install_team_menu
#endif
#if MONGO_SWAMP_TEST
    .long spawn_mongo_before_world_update - install_team_menu
    .long spawn_mongo_before_world_update_end - install_team_menu
#endif
