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

section .boot
bits 32
global start
start:
mov ecx, (initial_page_dir - 0xC0000000)
    mov cr3, ecx

    mov ecx, cr4
    or ecx, 0x10
    mov cr4, ecx

    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    jmp higher_half

section .text
higher_half:
extern kmain
    mov esp, stack_top
    push ebx                 ; multiboot2 info pointer
    xor ebp, ebp
    call kmain               ; Call kernel main function
    hlt                      ; Halt the CPU

section .data
align 4096
global initial_page_dir
initial_page_dir:
    dd 10000011b                 ; Entry 0: Identity map physical 0x00000000
    times 767 dd 0               ; Entries 1-767: Unmapped
    dd (0 << 22) | 10000011b     ; Entry 768: Virtual 0xC0000000 -> Physical 0x00000000
    dd (1 << 22) | 10000011b     ; Entry 769: Virtual 0xC0400000 -> Physical 0x00400000
    dd (2 << 22) | 10000011b     ; Entry 770: Virtual 0xC0800000 -> Physical 0x00800000
    dd (3 << 22) | 10000011b     ; Entry 771: Virtual 0xC0C00000 -> Physical 0x00C00000
    times 251 dd 0               ; Entries 772-1022: Unmapped
    dd (0xFD000000) | 10000011b  ; Entry 1023: Virtual 0xFFC00000 -> Physical 0xFD000000 (for the framebuffer)

section .bss
align 16
global stack_top
global stack_bottom
stack_bottom:                ; Bottom of stack
    resb 16384 * 4
stack_top:                   ; Top of stack
