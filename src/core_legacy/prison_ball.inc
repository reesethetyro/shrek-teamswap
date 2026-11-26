# Prison Break uses BigBall actors for the rolling metal spheres.
.macro prison_ball_chapter fallback
    .byte 0x83, 0x3d
    .long 0x1d0e7f8 - (CODE_RVA + (. - install_team_menu) + 5)
    .byte 11
    jne \fallback
.endm

prison_ball_handler:
    prison_ball_chapter native_ball_handler
    test rcx, rcx
    jz native_ball_handler
    cmp edx, 0xb
    je prison_ball_special
    cmp edx, 3
    je prison_ball_collision
    test edx, edx
    jne native_ball_handler
    cmp qword ptr [rcx + 0x38], 0
    jne prison_ball_collision
offer_prison_ball:
    sub rsp, 0x38
    mov [rsp + 0x20], rcx
    mov [rsp + 0x28], r8
    reject_scripted_swap finish_prison_ball_offer
    mov rcx, r10
    locate_rdx 0x23f520
    live_call 0x22c560
    test rax, rax
    jz finish_prison_ball_offer
    mov rcx, rax
    mov rdx, [rsp + 0x20]
    xor r8d, r8d
    live_call 0x23f100
finish_prison_ball_offer:
    mov rcx, [rsp + 0x20]
    mov r8, [rsp + 0x28]
    xor edx, edx
    add rsp, 0x38
    jmp native_ball_handler
offer_prison_ball_end:

prison_ball_special:
    test r8, r8
    jz native_ball_handler
    cmp dword ptr [r8], 8
    jne native_ball_handler
    mov rax, [r8 + 8]
    test rax, rax
    jz native_ball_handler
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x23f520
    cmp [rax + 0x10], r10
    jne native_ball_handler
    cmp [rax + 0x758], rcx
    jne native_ball_handler
    cmp qword ptr [rcx + 0x38], 0
    jne native_ball_handler
grab_prison_ball:
    sub rsp, 0x38
    mov rdx, rcx
    mov rcx, rax
    pxor xmm0, xmm0
    movups [rdx + 0x190], xmm0
    live_call 0x23e840
    mov eax, 1
    add rsp, 0x38
    ret
grab_prison_ball_end:

prison_ball_collision:
    # A carried sphere is attached to Shrek's hand. Resume the native
    # update and collision as soon as Shrek's release detaches it.
    cmp qword ptr [rcx + 0x38], 0
    je native_ball_handler
    xor eax, eax
    ret
native_ball_handler:
    .byte 0xe9
    rip_ref 0x14da40

.macro prison_carry_reach label, target, original, distance
\label:
    .byte 0xf3, 0x0f, 0x10, 0x0d
    rip_ref \original
    prison_ball_chapter .Lreach_done\@
    mov rax, [rbx + 0x758]
    test rax, rax
    jz .Lreach_done\@
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x146440
    cmp [rax + 0x10], r10
    jne .Lreach_done\@
    mov eax, \distance
    movd xmm1, eax
.Lreach_done\@:
    .byte 0xe9
    rip_ref \target
.endm

prison_carry_reach prison_ball_vertical_reach, 0x23e66f, 0x450934, 0x3f800000
prison_carry_reach prison_ball_horizontal_reach, 0x23e69c, 0x452bb8, 0x3fc00000

# Preserve the grab animation's native socket-relative position.
# Lower the sphere without moving its center to the socket origin.
prison_ball_carry_height:
    prison_ball_chapter .Lstore_carry_translation
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x146440
    cmp [rcx + 0x10], r10
    jne .Lstore_carry_translation
    mov r10d, 0x3e600000  # 0.21875 world units down (positive Y)
    movd xmm0, r10d
    shufps xmm0, xmm0, 0
    mulps xmm0, [rsp + 0x20]  # inverse carry-socket matrix Y row
    addps xmm15, xmm0
.Lstore_carry_translation:
    movups [rcx + 0xc0], xmm15
    .byte 0xe9
    rip_ref 0x23ea46

# BigBall does not implement the barrel's throw message. Put its center
# above the placement point before detaching so stock physics and the
# machine's authored BigBallTrigger can handle it normally.
prison_ball_release_position:
    prison_ball_chapter .Lstore_release_translation
    .byte 0x4c, 0x8d, 0x15
    rip_ref 0x146440
    cmp [rax + 0x10], r10
    jne .Lstore_release_translation
    mov r10, [rsp + 0x30]  # saved placement-point pointer
    test r10, r10
    jz .Lplace_in_front
    movups xmm3, [r10]
    jmp .Lraise_ball_center
.Lplace_in_front:
    # Shrek's put-down animation passes no target. Place the ball along
    # Shrek's facing direction, which is positive local Z.
    mov r10d, 0x3f480000  # 0.78125 along local Z
    movd xmm3, r10d
    shufps xmm3, xmm3, 0
    movups xmm1, [rdi + 0xb0]
    mulps xmm3, xmm1
    movups xmm1, [rdi + 0xc0]
    addps xmm3, xmm1
.Lraise_ball_center:
    mov r10d, 0x3ec00000  # BigBall radius
    movd xmm0, r10d
    pslldq xmm0, 4
    subps xmm3, xmm0
    xorps xmm0, xmm0
    movups [rax + 0x190], xmm0
    mov dword ptr [rax + 0x1b4], 0  # refresh the cached ground height
.Lstore_release_translation:
    movups [rax + 0xc0], xmm3
    .byte 0xe9
    rip_ref 0x23f6be

# A direct drop can land inside the switch's inner extent on its first
# contact. Start the native approach effect once before settling it.
prison_ball_switch_effect:
    test al, al
    jz .Lswitch_approach
    prison_ball_chapter .Lswitch_settle
    cmp byte ptr [rdi + 0x235], 0  # approach effect already active
    jne .Lswitch_settle
    .byte 0xe9
    rip_ref 0x148362
.Lswitch_settle:
    .byte 0xe9
    rip_ref 0x148455
.Lswitch_approach:
    .byte 0xe9
    rip_ref 0x1482b7
