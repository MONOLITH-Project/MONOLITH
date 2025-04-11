;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

extern gdtr

global flush_gdt
flush_gdt:
    lgdt [gdtr]    ; Remove 'rel' as it's x86_64 specific
    mov eax, .flush      ; Load the offset of flush
    push 0x08            ; Kernel code segment
    push eax             ; Push return address
    retf                 ; Far return
.flush:
    mov ax, 0x10         ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

global flush_tss
flush_tss:
    mov ax, 0x2B
    ltr ax
    ret
