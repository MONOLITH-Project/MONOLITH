/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <kernel/terminal/terminal.h>
#include <kernel/video/console.h>

/*
 * Initializes a framebuffer terminal.
 */
void fb_init_terminal(terminal_t *term, framebuffer_t *fb);

void fb_destroy_terminal();
