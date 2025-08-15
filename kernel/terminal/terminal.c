/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/input/ps2_keyboard.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/memstat.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <libs/flanterm/src/flanterm_backends/fb.h>
#include <stdarg.h>
#include <stdint.h>

static int _index = 0;
static char _buffer[TERM_BUFFER_SIZE];
bool _term_initialized = false;

extern struct flanterm_context *_fb_ctx;

void kputc(char c)
{
    if (c == '\n') {
        if (_index >= TERM_BUFFER_SIZE) {
            kflush();
        }
        _buffer[_index++] = '\n';
        kflush();
        return;
    } else if (_index >= TERM_BUFFER_SIZE) {
        kflush();
    }
    _buffer[_index++] = c;
}

void kputs(const char *str)
{
    while (*str)
        kputc(*str++);
}

static inline void _term_printd(int d)
{
    char buffer[16];
    int i = 0, is_negative = 0;

    if (d < 0) {
        is_negative = 1;
        d = -d;
    }

    if (d == 0) {
        buffer[i++] = '0';
    } else {
        while (d > 0) {
            buffer[i++] = '0' + (d % 10);
            d /= 10;
        }

        if (is_negative)
            buffer[i++] = '-';
    }

    /* Reverse the string */
    for (int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    buffer[i] = '\0';

    kputs(buffer);
}

static inline void _term_printx(size_t x)
{
    char buffer[16];
    int i = 0;

    if (x == 0) {
        buffer[i++] = '0';
    } else {
        while (x > 0) {
            uint8_t digit = x % 16;
            if (digit < 10)
                buffer[i++] = '0' + digit;
            else
                buffer[i++] = 'a' + (digit - 10);
            x /= 16;
        }
    }

    /* Reverse the string */
    for (int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    buffer[i] = '\0';

    kputs(buffer);
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;
            /* TODO: Add more format specifiers */
            switch (*fmt) {
            case 's':
                kputs(va_arg(args, const char *));
                break;
            case 'c':
                kputc(va_arg(args, int));
                break;
            case 'd':
                _term_printd(va_arg(args, int));
                break;
            case 'x':
                _term_printx(va_arg(args, uint64_t));
                break;
            case '%':
                kputc('%');
                break;
            }
        } else {
            kputc(*fmt);
        }
        fmt++;
    }

    va_end(args);
}

static char _read_keyboard()
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

char kgetc()
{
    kflush();
    char c;
    while ((c = _read_keyboard()) == 0x00)
        ;
    return c;
}

void kflush()
{
    if (_index > 0) {
        flanterm_write(_fb_ctx, _buffer, _index);
        _index = 0;
    }
}

void term_init(struct limine_framebuffer_response *response)
{
    if (_fb_ctx != NULL)
        flanterm_deinit(_fb_ctx, NULL);

    struct limine_framebuffer *fb = response->framebuffers[0];
    _fb_ctx = flanterm_fb_init(
        (void *) kmalloc,
        (void *) kfree,
        fb->address,
        fb->width,
        fb->height,
        fb->pitch,
        fb->red_mask_size,
        fb->red_mask_shift,
        fb->green_mask_size,
        fb->green_mask_shift,
        fb->blue_mask_size,
        fb->blue_mask_shift,
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
    _index = 0;

    kshell_init();
    memstat_init_cmds();
    _term_initialized = true;
    kshell_launch();
}
