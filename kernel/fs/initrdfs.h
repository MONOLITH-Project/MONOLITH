/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libs/limine/limine.h>

int initrd_new_drive(void *data);
void initrd_load_modules(struct limine_module_response *response);
