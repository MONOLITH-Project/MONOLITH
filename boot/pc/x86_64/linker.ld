ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_PHYS_BASE = 1M;
KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;

SECTIONS {
    . = KERNEL_PHYS_BASE;

    .multiboot_header ALIGN(4K): {
        *(.multiboot_header)
    }

    .boot ALIGN(4K) : {
        *(.boot)
    }

    . += KERNEL_VIRT_BASE;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRT_BASE) {
        *(.text*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE) {
        *(.rodata*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRT_BASE) {
        *(.data*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRT_BASE) {
        *(COMMON)
        *(.bss*)
    }

    kernel_end = .;
}
