ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    /* Kernel starts at 1MB */
    . = 1M;

    .boot ALIGN(4K) : {
        /* ensure that the multiboot header is at the beginning */
        KEEP(*(.multiboot_header))
    } :code

    .text ALIGN(4K) : {
        *(.text*)
    } :code

    .rodata ALIGN(4K) : {
        *(.rodata*)
        *(.eh_frame)
    } :rodata

    .data ALIGN(4K) : {
        *(.data*)
    } :data

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss*)
    } :data

    kernel_end = .;
}

PHDRS {
    code PT_LOAD FLAGS(5);    /* R-X */
    rodata PT_LOAD FLAGS(4);  /* R-- */
    data PT_LOAD FLAGS(6);    /* RW- */
}
