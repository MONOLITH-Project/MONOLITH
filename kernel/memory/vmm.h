/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

void vmm_init();
void vmm_map(void *virt, void *phys, size_t flags);
void vmm_unmap(void *virt);
