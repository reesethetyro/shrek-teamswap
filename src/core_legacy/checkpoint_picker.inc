# The book's explicit level-select action calls this before UnPause.
# Only offer the picker for offline sessions and levels with checkpoints.
choose_level_checkpoint:
    sub rsp, 0x228
    live_lea 0x05, CHECKPOINT_REQUEST_RVA
    mov dword ptr [rax], 0
    live_lea 0x0d, 0x1d0e7f0
    live_call 0x231b10
    test al, al
    jnz checkpoint_start_native
    .byte 0x8b, 0x05
    rip_ref 0x1cd7c44
    cmp eax, 26
    ja checkpoint_start_native
    lea r10, [rip + checkpoint_level_table]
    movsxd r11, dword ptr [r10 + rax * 4]
    test r11, r11
    jz checkpoint_start_native
    add r11, r10

    # Packed TASKDIALOGCONFIG at 0x30 and TASKDIALOG_BUTTON array at 0xd0.
    xorps xmm0, xmm0
    .set cp_zero, 0x30
    .rept 10
    movaps [rsp + cp_zero], xmm0
    .set cp_zero, cp_zero + 16
    .endr
    mov dword ptr [rsp + 0x30], 160
    mov dword ptr [rsp + 0x48], 1  # OK
    lea rax, [rip + checkpoint_title]
    mov [rsp + 0x4c], rax
    mov [rsp + 0x5c], rax
    lea rax, [rsp + 0xd0]
    mov [rsp + 0x80], rax
    mov dword ptr [rsp + 0x88], 1  # Level start selected by default
    mov dword ptr [rax], 1
    lea rcx, [rip + checkpoint_level_start]
    mov [rax + 4], rcx
    add rax, 12
    mov ecx, [r11]
    lea edx, [rcx + 1]
    mov [rsp + 0x7c], edx
    add r11, 4
checkpoint_radio_options:
    mov edx, [r11]
    mov [rax], edx
    movsxd rdx, dword ptr [r11 + 4]
    add rdx, r10
    mov [rax + 4], rdx
    add rax, 12
    add r11, 8
    dec ecx
    jnz checkpoint_radio_options

    lea rcx, [rip + checkpoint_comctl]
    .byte 0xff, 0x15
    rip_ref 0x44c8e8  # LoadLibraryA
    test rax, rax
    jz checkpoint_start_native
    mov [rsp + 0x20], rax
    mov rcx, rax
    lea rdx, [rip + checkpoint_taskdialog]
    .byte 0xff, 0x15
    rip_ref 0x44c8e0  # GetProcAddress
    test rax, rax
    jz checkpoint_free_library
    mov dword ptr [rsp + 0x28], 0
    mov dword ptr [rsp + 0x2c], 0
    lea rcx, [rsp + 0x30]
    lea rdx, [rsp + 0x28]
    lea r8, [rsp + 0x2c]
    xor r9d, r9d
    call rax
    test eax, eax
    js checkpoint_free_library
    cmp dword ptr [rsp + 0x28], 1
    jne checkpoint_free_library
    mov edx, [rsp + 0x2c]
    cmp edx, 1
    jbe checkpoint_free_library
    live_lea 0x05, CHECKPOINT_REQUEST_RVA
    mov [rax], edx
checkpoint_free_library:
    mov rcx, [rsp + 0x20]
    .byte 0xff, 0x15
    rip_ref 0x44c918  # FreeLibrary
checkpoint_start_native:
    live_lea 0x0d, 0x1d10ce0
    add rsp, 0x228
    .byte 0xe9
    rip_ref 0x244ae0
choose_level_checkpoint_end:

# Consume the one-shot selection before CreateGame chooses its scene.
# The native loader resolves the checkpoint and places the party.
apply_selected_checkpoint:
    live_lea 0x05, CHECKPOINT_REQUEST_RVA
    mov r10d, [rax]
    test r10d, r10d
    jz checkpoint_native_scene
    mov r11d, r10d
    shr r11d, 24
    .byte 0x44, 0x3b, 0x1d
    rip_ref 0x1d0e7f8
    jne checkpoint_native_scene
    mov dword ptr [rax], 0
    mov r11d, r10d
    shr r11d, 16
    and r11d, 0xff
    .byte 0x44, 0x89, 0x1d
    rip_ref 0x1d0e710
    movzx r10d, r10w
    dec r10d
    .byte 0x44, 0x89, 0x15
    rip_ref 0x5bfa90
checkpoint_native_scene:
    .byte 0x39, 0x3d
    rip_ref 0x1d0e7f8
    .byte 0xe9
    rip_ref 0x22beb8

checkpoint_comctl:
    .asciz "comctl32.dll"
checkpoint_taskdialog:
    .asciz "TaskDialogIndirect"
checkpoint_title:
    .short 83,101,108,101,99,116,32,97,32,99,104,101,99,107,112,111,105,110,116,0
checkpoint_level_start:
    .short 76,101,118,101,108,32,115,116,97,114,116,0
#include "checkpoint_catalog.inc"
