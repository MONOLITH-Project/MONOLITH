;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

section .multiboot_header
align 8                      ; Ensure header is 8-byte aligned
header_start:
dd 0xE85250D6                ; Multiboot2 magic number
dd 0                         ; Architecture 0 (protected mode i386)
dd header_end - header_start ; Header length
dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))  ; Checksum

; Framebuffer tag
dw 5                         ; Type: framebuffer
dw 1                         ; Flags: optional
dd 20                        ; Size (20 bytes for this tag)
dd 0                         ; Width (0 for default)
dd 0                         ; Height (0 for default)
dd 0                         ; Depth (0 for default)

; End tag
align 8
dw 0                         ; Type
dw 0                         ; Flags
dd 8                         ; Size

header_end:

global start
global stack_top
global stack_bottom

section .text
bits 32

start:
extern kmain
    mov esp, stack_top       ; Set up stack pointer
    push ebx                 ; Push multiboot info pointer as parameter
    call kmain               ; Call kernel main function
    hlt                      ; Halt the CPU

section .bss
align 4096
stack_bottom:                ; Bottom of stack
    resb 8192
stack_top:                   ; Top of stack
