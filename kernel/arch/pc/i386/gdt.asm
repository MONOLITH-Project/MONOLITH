;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

extern gdtr

global gdt_flush
gdt_flush:
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

global tss_flush
tss_flush:
    mov ax, 0x2B
    ltr ax
    ret
