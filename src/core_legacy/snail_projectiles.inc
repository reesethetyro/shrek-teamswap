# A snail's native target offset is zero, at the bottom of its shell.
# Aim at the body above that point. World Y increases downward.
set_snail_aim_height:
    or dword ptr [rbx + 0x364], 0x40000000
    mov dword ptr [rbx + 0x474], 0xbe6147ae # -0.22 world units.
    .byte 0xe9
    rip_ref 0x14407d
