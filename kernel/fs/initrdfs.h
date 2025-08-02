/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libs/limine/limine.h>

/*
 * Mount a new initrd drive.
 * Returns the drive ID when successful, or -1 on error.
 */
int initrd_new_drive(void *data);

/*
 * Mount all available initrd drives from limine modules.
 */
void initrd_load_modules(struct limine_module_response *response);
