/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/input/ps2_keyboard.h>
#include <kernel/video/vga/vga.h>
#include <kernel/video/vga/vga_terminal.h>

static void _vga_flush_callback(terminal_t *term)
{
    for (size_t i = 0; i < term->index; i++)
        vga_putchar(term->buffer[i]);
}

static char _vga_read_callback(terminal_t *)
{
    ps2_event_t event = ps2_wait_for_event();
    ps2_action_t action = ps2_get_event_action(event);

    char c = ps2_is_capslock_on() ? keyboard_layouts[KB_LAYOUT_US].shifted_keymap[event.scancode]
                                  : keyboard_layouts[KB_LAYOUT_US].keymap[event.scancode];
    if (action == KEYBOARD_HOLD) {
        return c;
    }
    return 0x00;
}

void vga_init_terminal(terminal_t *term)
{
    term_init(term, _vga_flush_callback, _vga_read_callback);
    vga_clear();
}
