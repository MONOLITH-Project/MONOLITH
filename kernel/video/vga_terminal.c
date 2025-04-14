/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/input/ps2_keyboard.h>
#include <kernel/video/vga.h>
#include <kernel/video/vga_terminal.h>

static void _vga_flush_callback(terminal_t *term)
{
    for (size_t i = 0; i < term->index; i++)
        vga_putchar(term->buffer[i]);
}

static char _vga_read_callback(terminal_t *)
{
    ps2_event_t event = wait_for_ps2_event();
    ps2_action_t action = get_ps2_event_action(event);

    char c = is_ps2_capslock_on() ? keyboard_layouts[KB_LAYOUT_US].shifted_keymap[event.scancode]
                                  : keyboard_layouts[KB_LAYOUT_US].keymap[event.scancode];
    if (action == KEYBOARD_HOLD) {
        return c;
    }
    return 0x00;
}

void init_vga_terminal(terminal_t *term)
{
    init_ps2_keyboard();
    term_init(term, _vga_flush_callback, _vga_read_callback);

    vga_clear();
    vga_set_fg_color(VGA_COLOR_GREEN);
    vga_puts("Welcome to MONOLITH!\nMake yourself at home.");
    vga_set_fg_color(VGA_COLOR_LIGHT_GREY);
}
