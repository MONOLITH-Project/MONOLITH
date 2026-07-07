/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libs/limine-protocol/include/limine.h>
#include <stddef.h>

#define SMP_MAX_CPUS 64

void smp_init(struct limine_mp_response *mp_response);
void smp_start_scheduler();
size_t smp_current_cpu_index();
size_t smp_online_cpu_count();
