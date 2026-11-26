.intel_syntax noprefix
.text
.globl append_player, get_player, clear_roster

# Prototype layout: five 64-bit player references, then a 32-bit count.
# Windows x64 ABI. A successful append returns a zero-based slot.
append_player:
    mov eax, [rcx + 40]
    cmp eax, 5
    jae append_failed
    test rdx, rdx
    jz append_failed
    xor r8d, r8d
check_duplicate:
    cmp r8d, eax
    jae store_player
    cmp [rcx + 8*r8], rdx
    je append_failed
    inc r8d
    jmp check_duplicate
store_player:
    mov [rcx + 8*rax], rdx
    inc dword ptr [rcx + 40]
    ret
append_failed:
    mov eax, -1
    ret

.p2align 4
get_player:
    xor eax, eax
    cmp edx, 5
    jae return_player
    cmp edx, [rcx + 40]
    jae return_player
    mov rax, [rcx + 8*rdx]
return_player:
    ret

.p2align 4
clear_roster:
    xor eax, eax
    mov [rcx], rax
    mov [rcx + 8], rax
    mov [rcx + 16], rax
    mov [rcx + 24], rax
    mov [rcx + 32], rax
    mov [rcx + 40], eax
    ret
