.intel_syntax noprefix
.text
.globl fairy_start
fairy_start:
.globl fairy_base
fairy_base:
.byte 0x48,0x8d,0x05
.long -(CODE_RVA+(. - fairy_start)+4)
ret
.globl fairy_state
fairy_state:
.byte 0x48,0x8d,0x05
.long STATE_RVA-(CODE_RVA+(. - fairy_start)+4)
ret
.globl fairy_update_hook
.seh_proc fairy_update_hook
fairy_update_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
call fairy_tick
mov rcx,[rsp+0x20]
add rsp,0x38
jmp fairy_update_original
.seh_endproc
.globl fairy_update_original
.seh_proc fairy_update_original
fairy_update_original:
push rbx
.seh_pushreg rbx
sub rsp,0x30
.seh_stackalloc 0x30
.seh_endprologue
.byte 0xe9
.long 0x1e0aa6-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc
.globl fairy_clear_hook
.seh_proc fairy_clear_hook
fairy_clear_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
call fairy_reset
call fairy_lever_reset
call fairy_exit_reset
mov rcx,[rsp+0x20]
add rsp,0x38
jmp fairy_clear_original
.seh_endproc
.globl fairy_clear_original
.seh_proc fairy_clear_original
fairy_clear_original:
mov [rsp+0x10],rbx
.seh_savereg rbx,0x10
.seh_endprologue
.byte 0xe9
.long 0x241ab5-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl fairy_bridge_update
.seh_proc fairy_bridge_update
fairy_bridge_update:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
call fairy_bridge_update_original
mov rcx,[rsp+0x20]
call fairy_lever_update
xor eax,eax
add rsp,0x38
ret
.seh_endproc
.globl fairy_bridge_update_original
.seh_proc fairy_bridge_update_original
fairy_bridge_update_original:
push rbp
.seh_pushreg rbp
push rbx
.seh_pushreg rbx
push rdi
.seh_pushreg rdi
push r12
.seh_pushreg r12
.seh_endprologue
.byte 0xe9
.long 0x1fcf26-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc
.globl fairy_bridge_trigger
.seh_proc fairy_bridge_trigger
fairy_bridge_trigger:
sub rsp,0x48
.seh_stackalloc 0x48
.seh_endprologue
mov [rsp+0x20],rcx
mov [rsp+0x28],rdx
call fairy_bridge_trigger_original
mov [rsp+0x30],rax
mov rcx,[rsp+0x20]
mov rdx,[rsp+0x28]
call fairy_lever_started
mov rax,[rsp+0x30]
add rsp,0x48
ret
.seh_endproc
.globl fairy_bridge_trigger_original
.seh_proc fairy_bridge_trigger_original
fairy_bridge_trigger_original:
mov [rsp+0x18],rbp
.seh_savereg rbp,0x18
.seh_endprologue
.byte 0xe9
.long 0x1fc465-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc
.globl fairy_action_check
.seh_proc fairy_action_check
fairy_action_check:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
call fairy_lever_maintain
mov rcx,[rsp+0x20]
add rsp,0x38
jmp fairy_action_original
.seh_endproc
.globl fairy_action_original
.seh_proc fairy_action_original
fairy_action_original:
push rdi
.seh_pushreg rdi
sub rsp,0x30
.seh_stackalloc 0x30
.seh_endprologue
.byte 0xe9
.long 0x1df036-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl fairy_bridge_ground
fairy_bridge_ground:
cmp dword ptr [rdi+0x600],3
je .Lbridge_allow
 test byte ptr [rax+0x1b0],1
jz .Lbridge_deny
.Lbridge_allow:
.byte 0xe9
.long 0x1fd094-(CODE_RVA+(. - fairy_start)+4)
.Lbridge_deny:
.byte 0xe9
.long 0x1fd212-(CODE_RVA+(. - fairy_start)+4)
.globl fairy_simple_ground
fairy_simple_ground:
cmp dword ptr [rdi+0x600],3
je .Lsimple_allow
 test byte ptr [rax+0x1b0],1
jz .Lsimple_deny
.Lsimple_allow:
.byte 0xe9
.long 0x202f94-(CODE_RVA+(. - fairy_start)+4)
.Lsimple_deny:
.byte 0xe9
.long 0x203114-(CODE_RVA+(. - fairy_start)+4)
.globl fairy_simple_message
.seh_proc fairy_simple_message
fairy_simple_message:
sub rsp,0x48
.seh_stackalloc 0x48
.seh_endprologue
mov [rsp+0x20],rcx
mov [rsp+0x28],edx
call fairy_simple_original
mov [rsp+0x30],rax
cmp dword ptr [rsp+0x28],0
jne .Lsimple_done
mov rcx,[rsp+0x20]
call fairy_lever_update
.Lsimple_done:
mov rax,[rsp+0x30]
add rsp,0x48
ret
.seh_endproc
.globl fairy_simple_original
.seh_proc fairy_simple_original
fairy_simple_original:
push rbp
.seh_pushreg rbp
push rbx
.seh_pushreg rbx
push rsi
.seh_pushreg rsi
lea rbp,[rsp-0x30]
.seh_endprologue
.byte 0xe9
.long 0x202d79-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl fairy_lever_height
fairy_lever_height:
cmp dword ptr [rdi+0x600],3
jne .Lnative_height
xorps xmm0,xmm0
ret
.Lnative_height:
.byte 0xe9
.long 0x94770-(CODE_RVA+(. - fairy_start)+4)
#include "fairy_compiled.s"


.globl settings_get_hook
settings_get_hook:
cmp edx,10
je settings_hide_get
cmp edx,9
je settings_get
.byte 0x48,0x8d,0x0d
.long 0x5c46a0-(CODE_RVA+(. - fairy_start)+4)
.byte 0xe9
.long 0x1bd820-(CODE_RVA+(. - fairy_start)+4)
_dummy_get:
.globl settings_set_hook
settings_set_hook:
cmp edx,10
je settings_hide_set
cmp edx,9
je settings_set
.byte 0x48,0x8d,0x0d
.long 0x5c46a0-(CODE_RVA+(. - fairy_start)+4)
.byte 0xe9
.long 0x1bdc30-(CODE_RVA+(. - fairy_start)+4)
.globl settings_ctor_hook
.seh_proc settings_ctor_hook
settings_ctor_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
call settings_init
mov rcx,[rsp+0x20]
add rsp,0x38
xor r8d,r8d
mov dword ptr [rcx],-1
.byte 0xe9
.long 0x24e8d9-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl settings_load_hook
.seh_proc settings_load_hook
settings_load_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rdx
call settings_load_original
mov rcx,[rsp+0x20]
call settings_load
add rsp,0x38
ret
.seh_endproc
.globl settings_load_original
.seh_proc settings_load_original
settings_load_original:
mov [rsp+8],rbx
.seh_savereg rbx,8
.seh_endprologue
.byte 0xe9
.long 1820293-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl settings_save_hook
.seh_proc settings_save_hook
settings_save_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rdx
call settings_save_original
mov rcx,[rsp+0x20]
call settings_save
add rsp,0x38
ret
.seh_endproc
.globl settings_save_original
.seh_proc settings_save_original
settings_save_original:
mov [rsp+8],rbx
.seh_savereg rbx,8
.seh_endprologue
.byte 0xe9
.long 1820629-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc


# Validate the state actually being rendered, including the native flip snapshot.
.macro validate_custom_page owner,counter,counterd,yes,no
lea r10,[\owner+0x38]
cmp rdx,r10
jbe \no
lea r10,[\owner+0x588]
cmp rdx,r10
je .Lsnapshot\@
ja \no
mov r10,rdx
sub r10,\owner
sub r10,0x38
.Lalignment\@:
sub r10,80
jz .Lstack\@
js \no
jmp .Lalignment\@
.Lstack\@:
cmp dword ptr [rdx-80],133
je \yes
jmp \no
.Lsnapshot\@:
.byte 0x4c,0x8d,0x15
.long STATE_RVA+0x3a00-(CODE_RVA+(. - fairy_start)+4)
mov \counterd,8
.Lorigin\@:
cmp [r10],\owner
jne .Lnext\@
cmp dword ptr [r10+8],eax
jne \no
cmp dword ptr [r10+12],133
je \yes
jmp \no
.Lnext\@:
add r10,16
dec \counterd
jnz .Lorigin\@
jmp \no
.endm
.globl settings_draw_hook
settings_draw_hook:
cmp eax,109
je custom_page_draw
cmp eax,115
jne settings_old_draw
custom_page_draw:
validate_custom_page rbp,r15,r15d,custom_page_valid,settings_empty
custom_page_valid:
cmp eax,109
jne custom_roster_entries
.byte 0x4c,0x8d,0x3d
.long STATE_RVA+0x1000+10*0xe8-(CODE_RVA+(. - fairy_start)+4)
jmp settings_draw_resume
custom_roster_entries:
mov r15,[rcx+rax*8+0x5b9370]
jmp settings_draw_resume
settings_empty:
.byte 0x4c,0x8d,0x3d
.long 0x1cfa350-(CODE_RVA+(. - fairy_start)+4)
settings_draw_resume:
.byte 0xe9
.long 0x259ba3-(CODE_RVA+(. - fairy_start)+4)
settings_old_draw:
.byte 0xe9
.long OLD_DRAW_RVA-(CODE_RVA+(. - fairy_start)+4)
.globl settings_style_hook
settings_style_hook:
cmp eax,108
jne .Lsettings_style
.byte 0x4c,0x8d,0x15
.long 0x18d59dc8-(CODE_RVA+(. - fairy_start)+4)
cmp r15,r10
jne settings_old_style
# Use Character Info's own vine decoration around this portrait page.
.byte 0x48,0x63,0x0d
.long 0x5e84f0+27*4-(CODE_RVA+(. - fairy_start)+4)
lea rax,[rdx+0x5b97a0]
.byte 0xe9
.long 0x259d94-(CODE_RVA+(. - fairy_start)+4)
.Lsettings_style:

cmp eax,109
jne settings_old_style
.byte 0x4c,0x8d,0x15
.long STATE_RVA+0x1000+10*0xe8-(CODE_RVA+(. - fairy_start)+4)
cmp r15,r10
jne settings_old_style
# Resolve Sound's STYLE through the native menu-to-style map.
.byte 0x48,0x63,0x0d
.long 0x5e84f0+75*4-(CODE_RVA+(. - fairy_start)+4)
settings_old_style:
.byte 0xe9
.long OLD_STYLE_RVA-(CODE_RVA+(. - fairy_start)+4)

.globl fairy_exit_original
.seh_proc fairy_exit_original
fairy_exit_original:
mov [rsp+8],rbx
.seh_savereg rbx,8
.seh_endprologue
.byte 0xe9
.long 0x202075-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl fairy_table_original
.seh_proc fairy_table_original
fairy_table_original:
mov [rsp+8],rbx
.seh_savereg rbx,8
.seh_endprologue
.byte 0xe9
.long 0x25e6f5-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl swap_back_hook
.seh_proc swap_back_hook
swap_back_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
call swap_remember_departing
mov rcx,[rsp+0x20]
add rsp,0x38
.byte 0xe9
.long OLD_BACK_RVA-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl swap_preview_hook
.seh_proc swap_preview_hook
swap_preview_hook:
sub rsp,0x10
.seh_stackalloc 0x10
.seh_endprologue
mov [rsp],rcx
mov [rsp+8],rdx
# Right-hand native states are blank placeholders. Resolve their corresponding
# left-hand roster state: page 1 is active, page 3 is the departing snapshot.
cmp dword ptr [rdi+0x160],1
je .Lpreview_active
cmp dword ptr [rdi+0x160],3
jne .Lpreview_native
.byte 0x48,0x8b,0x0d
.long 0x1d12540-(CODE_RVA+(. - fairy_start)+4)
test rcx,rcx
je .Lpreview_native
add rcx,0x178
lea rdx,[rcx+0x588]
jmp .Lpreview_check
.Lpreview_active:
.byte 0x48,0x8b,0x0d
.long 0x1d12540-(CODE_RVA+(. - fairy_start)+4)
test rcx,rcx
je .Lpreview_native
add rcx,0x178
movsxd r10,dword ptr [rcx+0x5d8]
cmp r10d,1
jl .Lpreview_native
cmp r10d,16
jg .Lpreview_native
lea r10,[r10+r10*4]
shl r10,4
lea rdx,[rcx+r10+0x38]
.Lpreview_check:
cmp dword ptr [rdx],115
jne .Lpreview_native
mov eax,115
validate_custom_page rcx,r11,r11d,.Lpreview_valid,.Lpreview_native
.Lpreview_valid:
mov r11d,[rdx+0x4c]
mov rcx,[rsp]
add rsp,0x10
.byte 0xe9
.long 0x18d5a47d-(CODE_RVA+(. - fairy_start)+4)
.Lpreview_native:
mov rcx,[rsp]
mov rdx,[rsp+8]
add rsp,0x10
.byte 0xe9
.long 0x259b70-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.p2align 4
.globl shield_pixels
shield_pixels:
.incbin "shield-bar.rgba"

.globl portrait_draw_original
.seh_proc portrait_draw_original
portrait_draw_original:
mov rax,rsp
mov [rax+8],rbx
.seh_savereg rbx,8
.seh_endprologue
.byte 0xe9
.long 0x127ba7-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc

.globl fairy_potion_original
.seh_proc fairy_potion_original
fairy_potion_original:
mov [rsp+0x10],rbx
.seh_savereg rbx,0x10
.seh_endprologue
.byte 0xe9
.long 0x1e90c5-(CODE_RVA+(. - fairy_start)+4)
.seh_endproc
