/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdint.h>

struct interrupt_registers;

void init_idt(void);
void set_idt_gate(uint8_t num, void *handler);
void flush_idt();
void isr_handler(struct interrupt_registers *);
void register_irq_handler(uint8_t num, void *handler);
void unregister_irq_handler(uint8_t num);
void irq_handler(struct interrupt_registers *);
