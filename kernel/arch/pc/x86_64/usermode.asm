;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

global jump_usermode
section .text

; Jump to usermode
; void jump_usermode(int (*entry)(int, char **), void *user_stack);
; rdi = entry
; rsi = user_stack
jump_usermode:

    push 0x23      ; User data segment (SS)
    push rsi       ; User stack top (RSP)
    push 0x202     ; RFLAGS (interrupts enabled)
    push 0x1B      ; User code segment (CS)
    push rdi       ; Entry point (RIP)

    iretq          ; Transition to user mode
