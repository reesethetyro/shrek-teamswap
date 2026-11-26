# AppleBullet advances 0.104 world units per simulation tick. Give it
# enough time to reach farther targets using the same collision path.
check_apple_flight_time:
    comiss xmm0, dword ptr [rip + apple_flight_ticks]
    .byte 0xe9
    rip_ref 0x19944e
.p2align 2
apple_flight_ticks:
    .float 84.0
