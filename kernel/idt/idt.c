/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "idt.h"
#include "../klibc/memory.h"
#include "../klibc/io.h"

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
    uint64_t isrNumber;
    uint64_t errorCode;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

typedef struct
{
    uint16_t offset0;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset1;
    uint32_t offset2;
    uint32_t zero;
} __attribute__((packed)) IDTEntry;

struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtPointer;

IDTEntry idtEntries[256];

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

void init_idt()
{
    idtPointer.limit = sizeof(idtEntries) - 1;
    idtPointer.base = (uint64_t) &idtEntries;
    memset(&idtEntries, 0, sizeof(idtEntries));

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

    set_idt_gate(0, (void *) isr0);
    set_idt_gate(1, (void *) isr1);
    set_idt_gate(2, (void *) isr2);
    set_idt_gate(3, (void *) isr3);
    set_idt_gate(4, (void *) isr4);
    set_idt_gate(5, (void *) isr5);
    set_idt_gate(6, (void *) isr6);
    set_idt_gate(7, (void *) isr7);
    set_idt_gate(8, (void *) isr8);
    set_idt_gate(9, (void *) isr9);
    set_idt_gate(10, (void *) isr10);
    set_idt_gate(11, (void *) isr11);
    set_idt_gate(12, (void *) isr12);
    set_idt_gate(13, (void *) isr13);
    set_idt_gate(14, (void *) isr14);
    set_idt_gate(15, (void *) isr15);
    set_idt_gate(16, (void *) isr16);
    set_idt_gate(17, (void *) isr17);
    set_idt_gate(18, (void *) isr18);
    set_idt_gate(19, (void *) isr19);
    set_idt_gate(20, (void *) isr20);
    set_idt_gate(21, (void *) isr21);
    set_idt_gate(22, (void *) isr22);
    set_idt_gate(23, (void *) isr23);
    set_idt_gate(24, (void *) isr24);
    set_idt_gate(25, (void *) isr25);
    set_idt_gate(26, (void *) isr26);
    set_idt_gate(27, (void *) isr27);
    set_idt_gate(28, (void *) isr28);
    set_idt_gate(29, (void *) isr29);
    set_idt_gate(30, (void *) isr30);
    set_idt_gate(31, (void *) isr31);

    set_idt_gate(32, (void *) irq0);
    set_idt_gate(33, (void *) irq1);
    set_idt_gate(34, (void *) irq2);
    set_idt_gate(35, (void *) irq3);
    set_idt_gate(36, (void *) irq4);
    set_idt_gate(37, (void *) irq5);
    set_idt_gate(38, (void *) irq6);
    set_idt_gate(39, (void *) irq7);
    set_idt_gate(40, (void *) irq8);
    set_idt_gate(41, (void *) irq9);
    set_idt_gate(42, (void *) irq10);
    set_idt_gate(43, (void *) irq11);
    set_idt_gate(44, (void *) irq12);
    set_idt_gate(45, (void *) irq13);
    set_idt_gate(46, (void *) irq14);
    set_idt_gate(47, (void *) irq15);

    flush_idt();
}

void set_idt_gate(uint8_t num, void *handler)
{
    uint64_t p = (uint64_t) handler;
    idtEntries[num].offset0 = (uint16_t) p;
    idtEntries[num].selector = 0x08;
    idtEntries[num].ist = 0;
    idtEntries[num].flags = 0x8E;
    idtEntries[num].offset1 = (uint16_t) (p >> 16);
    idtEntries[num].offset2 = (uint32_t) (p >> 32);
    idtEntries[num].zero = 0;
}

void flush_idt()
{
    __asm__ volatile("lidtq %0" : : "m"(idtPointer));
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
    if (regs->isrNumber < 32) {
        // TODO: add panic handling
    }
}

void *irqRoutines[16] = {NULL};

void register_irq_handler(const uint8_t irq, void *handler)
{
    irqRoutines[irq] = handler;
}

void unregister_irq_handler(const uint8_t irq)
{
    irqRoutines[irq] = NULL;
}

void irq_handler(struct interrupt_registers *reg)
{
    void (*handler)(struct interrupt_registers *) = irqRoutines[reg->isrNumber - 32];
    if (handler) {
        handler(reg);
    }

    if (reg->isrNumber >= 40) {
        outb(0x20, 0xA0);
    }
    outb(0x20, 0x20);
}
