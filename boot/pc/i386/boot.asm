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
    ; Load the Page Directory Table
    mov ecx, (PDT - 0xC0000000)
    mov cr3, ecx

    ; Enable paging
    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    jmp higher_half

section .text
higher_half:
extern kmain
    mov esp, stack_top
    push ebx                        ; multiboot2 info pointer
    xor ebp, ebp
    call kmain                      ; Call kernel main function
    hlt                             ; Halt the CPU

section .data
align 4096
global PDT
PDT:                                ; Page Directory Table (4 KB Pages, 1024 entries)
    dd (PT0 - 0xC0000000) + 0x03    ; Entry 0:          Identity map, Flags=Present,RW
    times 767 dd 0                  ; Entry 1-767:      Unused
    dd (PT768 - 0xC0000000) + 0x03  ; Entry 768:        0xC0000000->0x00000000, Flags=Present,RW
    dd (PT769 - 0xC0000000) + 0x03  ; Entry 769:        0xC0400000->0x00400000, Flags=Present,RW
    dd (PT770 - 0xC0000000) + 0x03  ; Entry 770:        0xC0800000->0x00800000, Flags=Present,RW
    dd (PT771 - 0xC0000000) + 0x03  ; Entry 771:        0xC0C00000->0x00C00000, Flags=Present,RW
    times 251 dd 0                  ; Entry 772-1022:   Unused
    dd (PT1023 - 0xC0000000) + 0x03 ; Entry 1023:       Framebuffer mapping via PT1023
PT0:
    %assign i 0
    %rep 1024
        dd (i * 0x1000) | 0x03      ; Present, RW
        %assign i i+1
    %endrep
PT768:
    %assign i 0
    %rep 1024
        dd (i * 0x1000) | 0x03      ; Present, RW
        %assign i i+1
    %endrep
PT769:
    %assign i 1024
    %rep 1024
        dd (i * 0x1000) | 0x03      ; Present, RW
        %assign i i+1
    %endrep
PT770:
    %assign i 2048
    %rep 1024
        dd (i * 0x1000) | 0x03      ; Present, RW
        %assign i i+1
    %endrep
PT771:
    %assign i 3072
    %rep 1024
        dd (i * 0x1000) | 0x03      ; Present, RW
        %assign i i+1
    %endrep
PT1023:
    %assign i 0
    %rep 1024
        dd (0xFD000000 + (i * 0x1000)) | 0x03
        %assign i i+1
    %endrep

section .bss
align 16
global stack_top
global stack_bottom
stack_bottom:
    resb 16384 * 4
stack_top:
