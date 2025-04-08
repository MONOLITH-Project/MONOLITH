/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/idt/idt.h>
#include <kernel/klibc/io.h>
#include <kernel/klibc/memory.h>
#include <kernel/serial.h>

struct interrupt_registers
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t core;
    uint64_t isr_number;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

/*
 * IDT Gate Descriptor
 * https://wiki.osdev.org/Interrupt_Descriptor_Table#Example_Code_2
 */
typedef struct
{
    uint16_t offset0;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset1;
    uint32_t offset2;
    uint32_t zero;
} __attribute__((packed)) idt_entry_t;

/*
 * IDTR
 * https://wiki.osdev.org/Interrupt_Descriptor_Table#IDTR
 */
static struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) _idtr;

/*
 * Interrupt Descriptor Table
 * https://wiki.osdev.org/Interrupt_Descriptor_Table#Table_2
 */
static idt_entry_t _idt_entries[256];

static void *_irq_routines[16] = {NULL};

extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();

extern void _irq0();
extern void _irq1();
extern void _irq2();
extern void _irq3();
extern void _irq4();
extern void _irq5();
extern void _irq6();
extern void _irq7();
extern void _irq8();
extern void _irq9();
extern void _irq10();
extern void _irq11();
extern void _irq12();
extern void _irq13();
extern void _irq14();
extern void _irq15();

void init_idt()
{
    debug_log("[*] Initializing the IDT...\n");

    _idtr.limit = sizeof(_idt_entries) - 1;
    _idtr.base = (uint64_t) &_idt_entries;
    memset(&_idt_entries, 0, sizeof(_idt_entries));

    // Remap the PIC
    outb(0x11, 0x20);
    outb(0x11, 0xA0);
    outb(0x20, 0x21);
    outb(0x28, 0xA1);
    outb(0x04, 0x21);
    outb(0x02, 0xA1);
    outb(0x01, 0x21);
    outb(0x01, 0xA1);
    outb(0x00, 0x21);
    outb(0x00, 0xA1);

    set_idt_gate(0, (void *) _isr0);
    set_idt_gate(1, (void *) _isr1);
    set_idt_gate(2, (void *) _isr2);
    set_idt_gate(3, (void *) _isr3);
    set_idt_gate(4, (void *) _isr4);
    set_idt_gate(5, (void *) _isr5);
    set_idt_gate(6, (void *) _isr6);
    set_idt_gate(7, (void *) _isr7);
    set_idt_gate(8, (void *) _isr8);
    set_idt_gate(9, (void *) _isr9);
    set_idt_gate(10, (void *) _isr10);
    set_idt_gate(11, (void *) _isr11);
    set_idt_gate(12, (void *) _isr12);
    set_idt_gate(13, (void *) _isr13);
    set_idt_gate(14, (void *) _isr14);
    set_idt_gate(15, (void *) _isr15);
    set_idt_gate(16, (void *) _isr16);
    set_idt_gate(17, (void *) _isr17);
    set_idt_gate(18, (void *) _isr18);
    set_idt_gate(19, (void *) _isr19);
    set_idt_gate(20, (void *) _isr20);
    set_idt_gate(21, (void *) _isr21);
    set_idt_gate(22, (void *) _isr22);
    set_idt_gate(23, (void *) _isr23);
    set_idt_gate(24, (void *) _isr24);
    set_idt_gate(25, (void *) _isr25);
    set_idt_gate(26, (void *) _isr26);
    set_idt_gate(27, (void *) _isr27);
    set_idt_gate(28, (void *) _isr28);
    set_idt_gate(29, (void *) _isr29);
    set_idt_gate(30, (void *) _isr30);
    set_idt_gate(31, (void *) _isr31);

    set_idt_gate(32, (void *) _irq0);
    set_idt_gate(33, (void *) _irq1);
    set_idt_gate(34, (void *) _irq2);
    set_idt_gate(35, (void *) _irq3);
    set_idt_gate(36, (void *) _irq4);
    set_idt_gate(37, (void *) _irq5);
    set_idt_gate(38, (void *) _irq6);
    set_idt_gate(39, (void *) _irq7);
    set_idt_gate(40, (void *) _irq8);
    set_idt_gate(41, (void *) _irq9);
    set_idt_gate(42, (void *) _irq10);
    set_idt_gate(43, (void *) _irq11);
    set_idt_gate(44, (void *) _irq12);
    set_idt_gate(45, (void *) _irq13);
    set_idt_gate(46, (void *) _irq14);
    set_idt_gate(47, (void *) _irq15);

    debug_log("[*] Flushing the IDT...\n");
    flush_idt();
    debug_log("[+] IDT initialized\n");
}

void set_idt_gate(uint8_t num, void *handler)
{
    uint64_t address = (uint64_t) handler;
    _idt_entries[num].offset0 = (uint16_t) address;
    _idt_entries[num].selector = 0x08;
    _idt_entries[num].ist = 0;
    _idt_entries[num].flags = 0x8E;
    _idt_entries[num].offset1 = (uint16_t) (address >> 16);
    _idt_entries[num].offset2 = (uint32_t) (address >> 32);
    _idt_entries[num].zero = 0;
}

void flush_idt()
{
    __asm__ volatile("lidtq %0" : : "m"(_idtr));
    __asm__ volatile("sti");
}

static const char *error_messages[] = {
    "Divide by zero",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double fault",
    "Co-processor Segment Overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-Segment Fault",
    "GPF",
    "Page Fault",
    "Reserved",
    "x87 Floating Point Exception",
    "alignment check",
    "Machine check",
    "SIMD floating-point exception",
    "Virtualization Exception",
    "Deadlock",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved",
    "Triple Fault",
    "FPU error",
};

void isr_handler(struct interrupt_registers *regs)
{
    if (regs->isr_number < 32) {
        debug_log_fmt("[-] System panic!\n");
        debug_log_fmt("[-] Error: %s\n", error_messages[regs->isr_number]);
        while (1)
            __asm__("hlt");
    }
}

void register_irq_handler(const uint8_t irq, void *handler)
{
    _irq_routines[irq] = handler;
}

void unregister_irq_handler(const uint8_t irq)
{
    _irq_routines[irq] = NULL;
}

void irq_handler(struct interrupt_registers *reg)
{
    void (*handler)(struct interrupt_registers *) = _irq_routines[reg->isr_number - 32];
    if (handler) {
        handler(reg);
    }

    if (reg->isr_number >= 40) {
        outb(0x20, 0xA0);
    }
    outb(0x20, 0x20);
}
