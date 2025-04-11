;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

%macro PUSHALL 0
    push eax
    push ebx
    push ecx
    push edx
    push ebp
    push edi
    push esi
%endmacro

%macro POPALL 0
    pop esi
    pop edi
    pop ebp
    pop edx
    pop ecx
    pop ebx
    pop eax
%endmacro

%macro ISR_NOERRCODE 1
    global _isr%1
    _isr%1:
        push 0              ; Push dummy error code
        push %1             ; Push interrupt number
        push fs
        jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
    global _isr%1
    _isr%1:
        push %1             ; CPU already pushed error code
        push fs
        jmp isr_common_stub
%endmacro

%macro IRQ 2
    global _irq%1
    _irq%1:
        push 0              ; Push dummy error code
        push %2             ; Push interrupt number
        push fs
        jmp irq_common_stub
%endmacro

; Exception handlers
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; IRQ handlers
IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

extern isr_handler
isr_common_stub:
    PUSHALL
    cld
    push esp              ; Pass pointer to stack as argument
    xor ebp, ebp
    call isr_handler
    add esp, 4           ; Remove pushed esp
    POPALL
    add esp, 12          ; Remove fs, int number, and error code
    iret

extern irq_handler
irq_common_stub:
    PUSHALL
    cld
    push esp              ; Pass pointer to stack as argument
    xor ebp, ebp
    call irq_handler
    add esp, 4           ; Remove pushed esp
    POPALL
    add esp, 12          ; Remove fs, int number, and error code
    iret
