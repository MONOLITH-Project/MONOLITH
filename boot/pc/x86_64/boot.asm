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
    mov esp, stack_top       ; Set up stack pointer
    call init_page_tables    ; Initialize page tables for paging
    call init_paging         ; Enable paging
    lgdt [gdt64.pointer]     ; Load GDT

    mov ax, gdt64.data       ; Load segment registers
    mov ss, ax               ; Set stack segment
    mov ds, ax               ; Set data segment
    mov es, ax               ; Set extra segment

    jmp gdt64.code:init      ; Jump to 64-bit code

init_page_tables:
    ; Map first entry of PML4 to PDP
    mov eax, PDP_table
    or  eax, 0b11           ; Present + Writable
    mov [PML4_table], eax

    ; Map first entry of PDP to PD
    mov eax, PD_table
    or eax, 0b11            ; Present + Writable
    mov [PDP_table], eax

    mov ecx, 0              ; Counter variable

.map_PD:
    ; Map PD entries to 2MB pages
    mov eax, 0x200000       ; 2MB page size
    mul ecx                 ; Multiply by counter
    or eax, 0b10000011      ; Present + Writable + Huge
    mov [PD_table + ecx * 8], eax

    inc ecx                 ; Increment counter
    cmp ecx, 512            ; Check if all entries are mapped
    jne .map_PD             ; If not, continue mapping

    ret

init_paging:
    ; Load PML4 address into CR3
    mov eax, PML4_table
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080     ; EFER MSR
    rdmsr
    or  eax, 1 << 8         ; LME bit
    wrmsr

    ; Enable paging
    mov eax, cr0
    or  eax, 1 << 31
    mov cr0, eax

    ret

section .text
bits 64
init:
    extern kmain
    mov rdi, rbx
    call kmain            ; Call kernel main function
    hlt                   ; Halt the CPU

section .rodata
gdt64:
  dq 0                    ; Null descriptor
.code: equ $ - gdt64
  dq (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)  ; Code segment
.data: equ $ - gdt64
  dq (1<<44) | (1<<47) | (1<<41)                      ; Data segment
.pointer:
  dw $ - gdt64 -1         ; GDT size
  dq gdt64                ; GDT address

section .bss
align 4096
PML4_table:               ; Page Map Level 4 table
  resb 4096
PDP_table:                ; Page Directory Pointer table
  resb 4096
PD_table:                 ; Page Directory table
  resb 4096

stack_bottom:             ; Bottom of stack
  resb 8192
stack_top:                ; Top of stack
