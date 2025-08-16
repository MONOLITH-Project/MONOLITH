;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

%macro PUSHALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POPALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

section .text
extern sys_hello
extern sys_request_fb

section .rodata
syscall_table:
    dq sys_hello
    dq sys_request_fb
syscall_table_end:

section .text
global _syscall_handler
_syscall_handler:
    cmp rax, (syscall_table_end-syscall_table) / 8
    jge .invalid
    PUSHALL
    xor rbp, rbp
    call [syscall_table + rax * 8]
    POPALL
    iretq
.invalid
    mov rax, -1
    iretq
