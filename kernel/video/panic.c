/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "libs/flanterm/src/flanterm.h"
#include <kernel/klibc/string.h>
#include <kernel/memory/heap.h>
#include <kernel/video/panic.h>
#include <libs/flanterm/src/flanterm_backends/fb.h>

#define FLANTERM_IN_FLANTERM
#include <libs/flanterm/src/flanterm_private.h>

extern struct flanterm_context *_fb_ctx;

void panic(const char *message)
{
    flanterm_write(_fb_ctx, "\n[-] ", 5);
    flanterm_write(_fb_ctx, message, strlen(message));
}
