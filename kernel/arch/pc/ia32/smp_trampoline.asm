;
; Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

bits 16
section .text

global smp_trampoline_start
global smp_trampoline_end
global smp_trampoline_cr3
global smp_trampoline_stack_top
global smp_trampoline_cpu
global smp_trampoline_entry

smp_trampoline_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000

    lgdt [cs:gdt_ptr - smp_trampoline_start]

    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:(0x7000 + protected_entry - smp_trampoline_start)

bits 32
protected_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, [0x7000 + smp_trampoline_cr3 - smp_trampoline_start]
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    mov esp, [0x7000 + smp_trampoline_stack_top - smp_trampoline_start]
    and esp, 0xFFFFFFF0
    push dword [0x7000 + smp_trampoline_cpu - smp_trampoline_start]
    call dword [0x7000 + smp_trampoline_entry - smp_trampoline_start]

.halt:
    hlt
    jmp .halt

align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_ptr:
    dw gdt_end - gdt - 1
    dd 0x7000 + gdt - smp_trampoline_start

smp_trampoline_cr3:
    dd 0
smp_trampoline_stack_top:
    dd 0
smp_trampoline_cpu:
    dd 0
smp_trampoline_entry:
    dd 0

smp_trampoline_end:
