/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void apic_init(void *rsdp);
void apic_init_local(void);
uint32_t apic_get_lapic_id(void);
size_t apic_get_processor_count(void);
uint32_t apic_get_processor_lapic_id(size_t index);
void apic_eoi(void);
void apic_send_init_ipi(uint32_t lapic_id);
void apic_send_startup_ipi(uint32_t lapic_id, uint8_t vector);
void apic_set_irq_mask(uint8_t irq, bool masked);
void apic_timer_mask(void);
void apic_timer_prepare_calibration(void);
uint32_t apic_timer_current_count(void);
void apic_timer_start_periodic(uint8_t vector, uint32_t initial_count);
bool apic_is_initialized(void);
