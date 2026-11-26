.intel_syntax noprefix
.text
.globl ammo_start
ammo_start:
.globl ammo_base
ammo_base:
.byte 0x48,0x8d,0x05
.long -(CODE_RVA+(. - ammo_start)+4)
ret
.globl ammo_state
ammo_state:
.byte 0x48,0x8d,0x05
.long STATE_RVA+0x3c00-(CODE_RVA+(. - ammo_start)+4)
ret
