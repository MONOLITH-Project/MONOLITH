/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <stddef.h>

#define SMP_MAX_CPUS 64

void smp_init(void);
void smp_start_scheduler(void);
size_t smp_current_cpu_index(void);
size_t smp_online_cpu_count(void);
