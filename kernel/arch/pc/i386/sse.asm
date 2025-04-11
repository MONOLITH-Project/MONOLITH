;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

global _init_sse
_init_sse:
    ; https://wiki.osdev.org/SSE#Adding_support
    mov eax, cr0
    and ax, 0xFFFB
    or  ax, 0x2
    mov cr0, eax
    mov eax, cr4
    or  ax, 3<<9
    mov cr4, eax
    ret
