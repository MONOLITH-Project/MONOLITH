;
; IA-32 GDT loading.
;

extern gdtr

global gdt_flush
gdt_flush:
    lgdt [gdtr]
    jmp 0x08:gdt_flush_common

global gdt_flush_with
gdt_flush_with:
    mov eax, [esp + 4]
    lgdt [eax]
    jmp 0x08:gdt_flush_common
gdt_flush_common:
    mov ax, 0x10
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
