/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stdint.h>

void init_gdt();
void set_gdt_gate(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void load_tss(void *);
