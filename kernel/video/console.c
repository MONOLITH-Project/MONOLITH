/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/input/ps2_keyboard.h>
#include <kernel/memory/memstat.h>
#include <kernel/terminal/kshell.h>
#include <kernel/video/console.h>
#include <kernel/video/framebuffer/fb_terminal.h>

static display_mode_t _display_mode = DISPLAY_MODE_UNINITIALIZED;
static framebuffer_t _framebuffer;

void console_init(struct limine_framebuffer_response *fb_response)
{
    terminal_t terminal;
    ps2_init_keyboard();
    _display_mode = DISPLAY_MODE_FRAMEBUFFER;
    struct limine_framebuffer *fb = fb_response->framebuffers[0];
    _framebuffer = (framebuffer_t) {
        .framebuffer = fb->address,
        .width = fb->width,
        .height = fb->height,
        .redmask_shift = fb->red_mask_shift,
        .greenmask_shift = fb->green_mask_shift,
        .bluemask_shift = fb->blue_mask_shift,
        .redmask_size = fb->red_mask_size,
        .greenmask_size = fb->green_mask_size,
        .bluemask_size = fb->blue_mask_size,
        .pitch = fb->pitch,
    };
    fb_init_terminal(&terminal, &_framebuffer);
    kshell_init();
    memstat_init_cmds();
    kshell_launch(&terminal);
}

display_mode_t console_get_mode()
{
    return _display_mode;
}

framebuffer_t console_get_framebuffer()
{
    return _framebuffer;
}
