/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/arch/pc/sse.h>
#include <kernel/serial.h>

extern void _init_sse();

void init_sse()
{
    debug_log("[*] Enabling SSE instructions...\n");
    _init_sse();
    debug_log("[+] Enabled SSE instructions\n");
}
