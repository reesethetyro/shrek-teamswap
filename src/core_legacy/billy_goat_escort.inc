.macro only_path_goat fallback
    .byte 0x83, 0x3d
    .long 0x1d0e7f8 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 5
    jne \fallback
    cmp byte ptr [rsi + 0x6bd], 1
    jne \fallback
.endm

# An accepted escort can follow the party through combat. Native damage
# handling still runs, but the route's automatic enemy-triggered retreat
# no longer takes over the forward escort.
scan_goat_escort_enemies:
    only_path_goat native_goat_enemy_scan
    xor eax, eax
    ret
native_goat_enemy_scan:
    .byte 0xe9
    rip_ref 0x07a000

# Entered with the native BillyGoat::UpdateMode frame and its spline in RAX.
# Follow with Character::WalkToAITarget's normal navigation mode. Use the
# spline endpoint only to decide when to hand back to the quest script.
follow_billy_goat:
    only_path_goat native_goat_spline
    test rax, rax
    jz hold_billy_goat
    mov ecx, [rax + 0x0c]
    dec ecx
    cmp ecx, 3
    jl hold_billy_goat
    mov rdi, rax
    mov eax, ecx
    mov ecx, 3
    xor edx, edx
    div ecx
    cvtsi2ss xmm2, eax
    mov rcx, rdi
    lea rdx, [rsp + 0x30]
    live_call 0x2a3a10

    live_lea 0x05, 0x1d0e7f0
    mov r8d, [rax + 0x54]
    lea eax, [r8 - 1]
    cmp eax, 8
    ja hold_billy_goat
    .byte 0x4c, 0x8d, 0x0d
    rip_ref 0x18d23000
    movups xmm0, [rsi + 0xc0]
    mov eax, 0x7f800000
    movd xmm4, eax
    xor edx, edx
    xor r10d, r10d
find_goat_leader:
    mov rax, [r9 + rdx * 8]
    test rax, rax
    jz next_goat_leader
    cmp dword ptr [rax + 0x18], 0
    jl next_goat_leader
    cmp qword ptr [rax + 0x490], 0
    je next_goat_leader
    movups xmm1, [rax + 0xc0]
    subps xmm1, xmm0
    dpps xmm1, xmm1, 0x71
    ucomiss xmm1, xmm4
    jp next_goat_leader
    jae next_goat_leader
    movaps xmm4, xmm1
    mov r10, rax
next_goat_leader:
    inc edx
    cmp edx, r8d
    jb find_goat_leader
    test r10, r10
    jz hold_billy_goat

    movups xmm2, [r10 + 0xc0]
    movaps xmm3, [rsp + 0x30]
    movaps xmm1, xmm3
    subps xmm1, xmm2
    dpps xmm1, xmm1, 0x71
    mov eax, 0x3fb851ec  # arrival radius squared (1.2 units)
    movd xmm5, eax
    ucomiss xmm5, xmm1
    jb follow_goat_leader
    movaps xmm1, xmm3
    subps xmm1, xmm0
    dpps xmm1, xmm1, 0x71
    ucomiss xmm5, xmm1
    jae finish_goat_escort

follow_goat_leader:
    # Use actual horizontal velocity. The player's animation speed can
    # remain nonzero when standing still.
    movups xmm1, [r10 + 0x190]
    dpps xmm1, xmm1, 0x51
    mov eax, 0x3727c5ac  # 0.00001 squared-speed dead zone
    movd xmm5, eax
    ucomiss xmm1, xmm5
    jbe hold_billy_goat
    mov eax, 0x3f4f5c29  # idle radius squared (0.9 units)
    movd xmm5, eax
    ucomiss xmm5, xmm4
    jae hold_billy_goat
    movups [rsi + 0x1e0], xmm2
    movups [rsi + 0x1f0], xmm2
    mov dword ptr [rsi + 0x248], 1  # normal AI navigation, including walk-around
    mov rcx, rsi
    live_call 0x0942c0
    jmp continue_goat_escort
hold_billy_goat:
    movups xmm0, [rsi + 0xc0]
    movups [rsi + 0x1e0], xmm0
    movups [rsi + 0x1f0], xmm0
    and dword ptr [rsi + 0x200], -2
    mov dword ptr [rsi + 0x248], 0
    mov dword ptr [rsi + 0x398], 0
continue_goat_escort:
    .byte 0xe9
    rip_ref 0x07ae3e
finish_goat_escort:
    .byte 0xe9
    rip_ref 0x07ad54
native_goat_spline:
    .byte 0xf3, 0x0f, 0x10, 0x1d
    rip_ref 0x4547c4
    .byte 0xe9
    rip_ref 0x07ac45
