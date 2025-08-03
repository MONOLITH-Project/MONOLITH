/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/input/ps2_keyboard.h>
#include <kernel/memory/heap.h>
#include <kernel/video/console.h>
#include <kernel/video/framebuffer/fb_terminal.h>
#include <libs/flanterm/src/flanterm_backends/fb.h>
#include <libs/flanterm/src/flanterm.h>

#define FLANTERM_IN_FLANTERM
#include <libs/flanterm/src/flanterm_private.h>

static struct flanterm_context *_fb_ctx;

static void _fb_flush_callback(terminal_t *term)
{
    flanterm_write(_fb_ctx, term->buffer, term->index);
}

static char _fb_read_callback(terminal_t *)
{
    ps2_event_t event = ps2_wait_for_event();
    ps2_action_t action = ps2_get_event_action(event);

    char c = ps2_is_capslock_on() || ps2_is_key_down(KEY_RSHIFT) || ps2_is_key_down(KEY_LSHIFT)
                 ? keyboard_layouts[KB_LAYOUT_US].shifted_keymap[event.scancode]
                 : keyboard_layouts[KB_LAYOUT_US].keymap[event.scancode];
    if (action == KEYBOARD_HOLD) {
        return c;
    }
    return 0x00;
}

void fb_init_terminal(terminal_t *term, framebuffer_t *fb)
{
    _fb_ctx = flanterm_fb_init(
        (void *) kmalloc,
        (void *) kfree,
        fb->framebuffer,
        fb->width,
        fb->height,
        fb->pitch,
        fb->redmask_size,
        fb->redmask_shift,
        fb->greenmask_size,
        fb->greenmask_shift,
        fb->bluemask_size,
        fb->bluemask_shift,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        0,
        1,
        0,
        0,
        0);
    term_init(term, _fb_flush_callback, _fb_read_callback);
}

void fb_destroy_terminal()
{
    flanterm_deinit(_fb_ctx, (void *) kfree);
}
