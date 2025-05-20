;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

section .multiboot_header
align 8
header_start:
dd 0xE85250D6                ; Multiboot2 magic
dd 0                         ; Architecture (i386)
dd header_end - header_start ; Header length
dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start)) ; Checksum

; End tag
align 8
dw 0                         ; End tag
dw 0
dd 8
header_end:

section .boot
bits 32

align 4096
global PML4T
PML4T:  resb 4096           ; Page Map Level 4
PDPT:   resb 4096           ; Page Directory Pointer Table (Level 3)
PDT:    resb 4096           ; Page Directory Table (Level 2)
PT0:    resb 4096           ; Page Table 0 (Level 1)
PT1:    resb 4096           ; Page Table 1 (Level 1)

gdt64:
  dq 0                                          ; Null descriptor
.code: equ $ - gdt64
  dq (1<<44)|(1<<47)|(1<<41)|(1<<43)|(1<<53)    ; Code
.data: equ $ - gdt64
  dq (1<<44)|(1<<47)|(1<<41)                    ; Data
.pointer:
  dw $ - gdt64 - 1                              ; Limit
  dq gdt64                                      ; Base

global start
start:
    mov esp, stack_top_32
    call init_page_tables
    call init_paging
    lgdt [gdt64.pointer]

    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax

    jmp gdt64.code:init      ; Jump to 64-bit

init_page_tables:
    ; Initialize all page tables to zero
    mov edi, PML4T
    xor eax, eax
    mov ecx, (PT1 + 4096 - PML4T) / 4
    rep stosd

    ; Identity mapping (Low half)
    mov eax, PDPT               ; Load PDPT address
    or eax, 0x03                ; Flags=Present,RW
    mov [PML4T], eax            ; PML4[0] -> PDP

    mov eax, PDT                ; Load PDT address
    or eax, 0x03                ; Flags=Present,RW
    mov [PDPT], eax             ; PDP[0] -> PD

    ; Higher-half mapping (0xFFFF800000000000)
    mov eax, PDPT               ; Load PDPT address
    or eax, 0x03                ; Flags=Present,RW
    mov [PML4T + 511 * 8], eax  ; PML4[511] -> PDP

    mov eax, PDT                ; Load PDT address
    or eax, 0x03                ; Flags=Present,RW
    mov [PDPT + 510 * 8], eax   ; PDP[510] -> PD

    ; Configure PD entries to point to PTs
    mov ecx, 0                  ; PD index
.map_PD:
    mov eax, PT0                ; Base PT address
    mov edx, ecx
    imul edx, 4096              ; Offset to PT (4096 per PT)
    add eax, edx                ; Physical address of PT
    or eax, 0x03                ; Flags=Present,RW
    mov [PDT + ecx * 8], eax    ; Store in PD

    inc ecx
    cmp ecx, 2                  ; Map first 4MB (2 PTs)
    jne .map_PD

    ; Fill PT0 with 4KB entries (0-2MB)
    mov ecx, 0
.fill_PT0:
    mov eax, ecx
    imul eax, 0x1000            ; 4KB pages
    or eax, 0x03                ; Flags=Present,RW
    mov [PT0 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .fill_PT0

    ; Fill PT1 with 4KB entries (2-4MB)
    mov ecx, 0
.fill_PT1:
    mov eax, ecx
    imul eax, 0x1000            ; 4KB pages
    add eax, 0x200000           ; Start at 2MB
    or eax, 0x03                ; Flags=Present,RW
    mov [PT1 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .fill_PT1

    ret

init_paging:
    ; Load the Page Map Table
    mov eax, PML4T
    mov cr3, eax

    ; Enable PAE + PGE
    mov eax, cr4
    or eax, (1 << 5) | (1 << 7)
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

align 16
stack_bottom_32:
  resb 4096
stack_top_32:

bits 64
init:
    extern kmain
    mov rdi, rbx            ; Pass Multiboot info
    call kmain
    hlt

section .bss
global stack_top
global stack_bottom
stack_bottom:
    resb 16384 * 4
stack_top:
