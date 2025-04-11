/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdint.h>

struct interrupt_registers;

/*
 * Initialize the Interrupt Descriptor Table
 * https://wiki.osdev.org/Interrupt_Descriptor_Table
 */
void init_idt(void);

/*
 * Set the IDT gate
 * https://wiki.osdev.org/Interrupts_Tutorial#Assembling
 */
void set_idt_gate(uint8_t num, void *handler);

/*
 * Flush the IDT
 * https://wiki.osdev.org/Interrupt_Descriptor_Table
 */
void flush_idt();

/*
 * Interrupt Service Routine Handler
 * https://wiki.osdev.org/Interrupt_Service_Routines
 */
void isr_handler(struct interrupt_registers *);

/*
 * Interrupt Service Routine Handler
 * https://wiki.osdev.org/Interrupt_Service_Routines
 */
void irq_handler(struct interrupt_registers *);

/*
 * Register an IRQ Service Routine
 * https://wiki.osdev.org/Interrupts
 */
void register_irq_handler(uint8_t num, void *handler);

/*
 * Unregister IRQ Service Routine
 * https://wiki.osdev.org/Interrupts
 */
void unregister_irq_handler(uint8_t num);
