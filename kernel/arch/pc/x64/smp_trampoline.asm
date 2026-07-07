;
; Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

global smp_ap_entry
extern smp_ap_main

smp_ap_entry:
    mov rax, [rdi + 24]                 ; limine_mp_info->extra_argument
    mov rcx, [rax + 24]                 ; limine_mp_info->extra_argument->cr3
    mov cr3, rcx
    mov rsp, [rax + 16]                 ; limine_mp_info->extra_argument->stack_top
    and rsp, -16
    xor rbp, rbp
    call smp_ap_main

.halt:
    hlt
    jmp .halt
