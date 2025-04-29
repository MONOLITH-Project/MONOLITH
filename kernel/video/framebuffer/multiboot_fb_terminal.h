/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <kernel/multiboot2.h>
#include <kernel/terminal/terminal.h>
#include <kernel/video/console.h>
#include <stdint.h>

struct multiboot_tag_framebuffer *find_fb_tag(struct multiboot_tag *tag);
void multiboot_fb_init_terminal(terminal_t *term, struct multiboot_tag_framebuffer *tag, framebuffer_t *fb);
