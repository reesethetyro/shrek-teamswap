 .intel_syntax noprefix
.text
.globl quick_start
quick_start:
.globl quick_base
quick_base:
.byte 0x48,0x8d,0x05
.long -(CODE_RVA+(. - quick_start)+4)
ret
.globl quick_state
quick_state:
.byte 0x48,0x8d,0x05
.long STATE_RVA-(CODE_RVA+(. - quick_start)+4)
ret
.globl quick_world_hook
.seh_proc quick_world_hook
quick_world_hook:
sub rsp,0x38
.seh_stackalloc 0x38
.seh_endprologue
mov [rsp+0x20],rcx
mov [rsp+0x28],rdx
call quick_tick
mov rcx,[rsp+0x20]
mov rdx,[rsp+0x28]
add rsp,0x38
jmp quick_world_original
.seh_endproc
.globl quick_world_original
.seh_proc quick_world_original
quick_world_original:
mov rax,rsp
mov [rax+0x10],rbx
.seh_savereg rbx,0x10
.seh_endprologue
.byte 0xe9
.long 2394615-(CODE_RVA+(. - quick_start)+4)
.seh_endproc

.globl attack_pickup_original
.seh_proc attack_pickup_original
attack_pickup_original:
mov [rsp+0x10],rbx
.seh_savereg rbx,0x10
.seh_endprologue
.byte 0xe9
.long 2012373-(CODE_RVA+(. - quick_start)+4)
.seh_endproc

.p2align 4
.globl potion_damage_0
.seh_proc potion_damage_0
potion_damage_0:
pushfq
.seh_stackalloc 8
sub rsp,0xd0
.seh_stackalloc 0xd0
.seh_endprologue
mov [rsp+32],rcx
mov [rsp+40],rdx
mov [rsp+48],r8
mov [rsp+56],r9
mov [rsp+64],r10
mov [rsp+72],r11
mov [rsp+80],rax
movups [rsp+96],xmm0
movups [rsp+112],xmm1
movups [rsp+128],xmm2
movups [rsp+144],xmm3
movups [rsp+160],xmm4
movups [rsp+176],xmm5
mov rcx,rdi
call quick_damage
mov [rsp+0xc0],eax
mov rcx,[rsp+32]
mov rdx,[rsp+40]
mov r8,[rsp+48]
mov r9,[rsp+56]
mov r10,[rsp+64]
mov r11,[rsp+72]
mov rax,[rsp+80]
movups xmm0,[rsp+96]
movups xmm1,[rsp+112]
movups xmm2,[rsp+128]
movups xmm3,[rsp+144]
movups xmm4,[rsp+160]
movups xmm5,[rsp+176]
mov eax,[rsp+0xc0]
add rsp,0xd0
popfq
ret
.seh_endproc

.p2align 4
.globl potion_damage_1
.seh_proc potion_damage_1
potion_damage_1:
pushfq
.seh_stackalloc 8
sub rsp,0xd0
.seh_stackalloc 0xd0
.seh_endprologue
mov [rsp+32],rcx
mov [rsp+40],rdx
mov [rsp+48],r8
mov [rsp+56],r9
mov [rsp+64],r10
mov [rsp+72],r11
mov [rsp+80],rax
movups [rsp+96],xmm0
movups [rsp+112],xmm1
movups [rsp+128],xmm2
movups [rsp+144],xmm3
movups [rsp+160],xmm4
movups [rsp+176],xmm5
mov rcx,rsi
call quick_damage
mov [rsp+0xc0],eax
mov rcx,[rsp+32]
mov rdx,[rsp+40]
mov r8,[rsp+48]
mov r9,[rsp+56]
mov r10,[rsp+64]
mov r11,[rsp+72]
mov rax,[rsp+80]
movups xmm0,[rsp+96]
movups xmm1,[rsp+112]
movups xmm2,[rsp+128]
movups xmm3,[rsp+144]
movups xmm4,[rsp+160]
movups xmm5,[rsp+176]
mov eax,[rsp+0xc0]
add rsp,0xd0
popfq
ret
.seh_endproc

.p2align 4
.globl potion_damage_2
.seh_proc potion_damage_2
potion_damage_2:
pushfq
.seh_stackalloc 8
sub rsp,0xd0
.seh_stackalloc 0xd0
.seh_endprologue
mov [rsp+32],rcx
mov [rsp+40],rdx
mov [rsp+48],r8
mov [rsp+56],r9
mov [rsp+64],r10
mov [rsp+72],r11
mov [rsp+80],rax
movups [rsp+96],xmm0
movups [rsp+112],xmm1
movups [rsp+128],xmm2
movups [rsp+144],xmm3
movups [rsp+160],xmm4
movups [rsp+176],xmm5
mov rcx,rbp
call quick_damage
mov [rsp+0xc0],eax
mov rcx,[rsp+32]
mov rdx,[rsp+40]
mov r8,[rsp+48]
mov r9,[rsp+56]
mov r10,[rsp+64]
mov r11,[rsp+72]
mov rax,[rsp+80]
movups xmm0,[rsp+96]
movups xmm1,[rsp+112]
movups xmm2,[rsp+128]
movups xmm3,[rsp+144]
movups xmm4,[rsp+160]
movups xmm5,[rsp+176]
mov eax,[rsp+0xc0]
add rsp,0xd0
popfq
ret
.seh_endproc

.p2align 4
.globl potion_damage_3
.seh_proc potion_damage_3
potion_damage_3:
pushfq
.seh_stackalloc 8
sub rsp,0xd0
.seh_stackalloc 0xd0
.seh_endprologue
mov [rsp+32],rcx
mov [rsp+40],rdx
mov [rsp+48],r8
mov [rsp+56],r9
mov [rsp+64],r10
mov [rsp+72],r11
mov [rsp+80],rax
movups [rsp+96],xmm0
movups [rsp+112],xmm1
movups [rsp+128],xmm2
movups [rsp+144],xmm3
movups [rsp+160],xmm4
movups [rsp+176],xmm5
mov rcx,[rbx+0x1e8]
call quick_damage
mov [rsp+0xc0],eax
mov rcx,[rsp+32]
mov rdx,[rsp+40]
mov r8,[rsp+48]
mov r9,[rsp+56]
mov r10,[rsp+64]
mov r11,[rsp+72]
mov rax,[rsp+80]
movups xmm0,[rsp+96]
movups xmm1,[rsp+112]
movups xmm2,[rsp+128]
movups xmm3,[rsp+144]
movups xmm4,[rsp+160]
movups xmm5,[rsp+176]
mov eax,[rsp+0xc0]
add rsp,0xd0
popfq
ret
.seh_endproc

.p2align 4
.globl potion_damage_4
.seh_proc potion_damage_4
potion_damage_4:
pushfq
.seh_stackalloc 8
sub rsp,0xd0
.seh_stackalloc 0xd0
.seh_endprologue
mov [rsp+32],rcx
mov [rsp+40],rdx
mov [rsp+48],r8
mov [rsp+56],r9
mov [rsp+64],r10
mov [rsp+72],r11
mov [rsp+80],rax
movups [rsp+96],xmm0
movups [rsp+112],xmm1
movups [rsp+128],xmm2
movups [rsp+144],xmm3
movups [rsp+160],xmm4
movups [rsp+176],xmm5
mov rcx,rax
call quick_damage
mov [rsp+0xc0],eax
mov rcx,[rsp+32]
mov rdx,[rsp+40]
mov r8,[rsp+48]
mov r9,[rsp+56]
mov r10,[rsp+64]
mov r11,[rsp+72]
mov rax,[rsp+80]
movups xmm0,[rsp+96]
movups xmm1,[rsp+112]
movups xmm2,[rsp+128]
movups xmm3,[rsp+144]
movups xmm4,[rsp+160]
movups xmm5,[rsp+176]
mov ebx,[rsp+0xc0]
add rsp,0xd0
popfq
ret
.seh_endproc

.p2align 4
.globl potion_damage_5
.seh_proc potion_damage_5
potion_damage_5:
pushfq
.seh_stackalloc 8
sub rsp,0xd0
.seh_stackalloc 0xd0
.seh_endprologue
mov [rsp+32],rcx
mov [rsp+40],rdx
mov [rsp+48],r8
mov [rsp+56],r9
mov [rsp+64],r10
mov [rsp+72],r11
mov [rsp+80],rax
movups [rsp+96],xmm0
movups [rsp+112],xmm1
movups [rsp+128],xmm2
movups [rsp+144],xmm3
movups [rsp+160],xmm4
movups [rsp+176],xmm5
mov ecx,ebx
call quick_damage_slot
mov [rsp+0xc0],eax
mov rcx,[rsp+32]
mov rdx,[rsp+40]
mov r8,[rsp+48]
mov r9,[rsp+56]
mov r10,[rsp+64]
mov r11,[rsp+72]
mov rax,[rsp+80]
movups xmm0,[rsp+96]
movups xmm1,[rsp+112]
movups xmm2,[rsp+128]
movups xmm3,[rsp+144]
movups xmm4,[rsp+160]
movups xmm5,[rsp+176]
mov edi,[rsp+0xc0]
add rsp,0xd0
popfq
ret
.seh_endproc
