;
; Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
; SPDX-License-Identifier: GPL-3.0
;

global gdt_flush_with
gdt_flush_with:
	lgdt [rdi]
	push 8
	lea rax, [rel .flush]
	push rax
	retfq
.flush:
	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	ret

global gdt_flush_tss
gdt_flush_tss:
	mov ax, 0x28
	ltr ax
	ret
