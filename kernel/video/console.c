/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <kernel/input/ps2_keyboard.h>
#include <kernel/klibc/memory.h>
#include <kernel/klibc/string.h>
#include <kernel/memory/memstat.h>
#include <kernel/serial.h>
#include <kernel/terminal/kshell.h>
#include <kernel/terminal/terminal.h>
#include <kernel/video/console.h>
#include <kernel/video/framebuffer/fb_terminal.h>
#include <kernel/video/framebuffer/multiboot_fb_terminal.h>
#include <kernel/video/vesa/vesa.h>
#include <kernel/video/vesa/vesa_structures.h>
#include <kernel/video/vga/vga.h>
#include <kernel/video/vga/vga_terminal.h>
#include <stdint.h>

static display_mode_t _display_mode = DISPLAY_MODE_UNINITIALIZED;
static framebuffer_t _framebuffer;

#ifdef __i386__
static char _wait_for_input()
{
    ps2_event_t event = ps2_wait_for_event();
    ps2_action_t action = ps2_get_event_action(event);

    char c = ps2_is_capslock_on() ? keyboard_layouts[KB_LAYOUT_US].shifted_keymap[event.scancode]
                                  : keyboard_layouts[KB_LAYOUT_US].keymap[event.scancode];
    if (action == KEYBOARD_HOLD) {
        return c;
    }
    return 0x0;
}

static void _init_vga_text_mode(terminal_t *term)
{
    int86_regs_t regs;
    memset(&regs, 0, sizeof(regs));
    regs.ax = 0x03;
    int86(0x10, &regs);
    memset(&_framebuffer, 0, sizeof(framebuffer_t));

    vga_init_terminal(term);
    _display_mode = DISPLAY_MODE_VGA_TEXT;
}

static void _read_input(terminal_t *term, char *buffer)
{
    char c;
    size_t index = 0;
    term_puts(term, "\n> ");
    while ((c = term_getc(term)) != '\n') {
        if (c == '\b') {
            if (index > 0) {
                term_putc(term, c);
                index--;
                buffer[index] = '\0';
            }
        } else {
            term_putc(term, c);
            buffer[index] = c;
            index++;
        }
    }
    buffer[index] = '\0';
}

static bool _parse_resolution(const char *str, uint16_t *width, uint16_t *height)
{
    const char *p = str;
    char buffer[16];
    int i;

    /* Parse width */
    i = 0;
    while (*p >= '0' && *p <= '9' && i < 15) {
        buffer[i++] = *p++;
    }
    buffer[i] = '\0';
    if (*p != 'x' || i == 0)
        return false;
    *width = atoul(buffer);
    p++; /* Skip 'x' */

    /* Parse height */
    i = 0;
    while (*p >= '0' && *p <= '9' && i < 15) {
        buffer[i++] = *p++;
    }
    buffer[i] = '\0';
    if (i == 0)
        return false;
    *height = atoul(buffer);

    return *p == '\0';
}

static void _select_vbe_resolution(
    terminal_t *term, vbe_info_t *info, struct multiboot_tag_framebuffer *tag)
{
    char buffer[128];
    uint16_t width, height;
    term_printf(
        term,
        "[*] Enter your screen resolution (default: %dx%d):",
        tag->common.framebuffer_width,
        tag->common.framebuffer_height);
    while (1) {
        _read_input(term, buffer);
        if (buffer[0] == '\0') {
            return;
        } else if (_parse_resolution(buffer, &width, &height)) {
            uint16_t mode = vbe_find_mode(info, width, height, 32);
            if (mode != 0xFFFF) {
                vbe_mode_info_t mode_info = vbe_get_mode_info(mode);
                _framebuffer = (framebuffer_t) {
                    .framebuffer = (uint32_t *) mode_info.framebuffer,
                    .width = mode_info.width,
                    .height = mode_info.height,
                    .pitch = mode_info.pitch,
                    .redmask_size = mode_info.red_mask,
                    .redmask_shift = mode_info.red_position,
                    .greenmask_size = mode_info.green_mask,
                    .greenmask_shift = mode_info.green_position,
                    .bluemask_size = mode_info.blue_mask,
                    .bluemask_shift = mode_info.blue_position,
                };
                vbe_set_mode(mode);
                fb_init_terminal(term, &_framebuffer);
            } else {
                term_printf(
                    term, "\n[-] The screen resolution %dx%d is not supported.", width, height);
                continue;
            }
            break;
        } else {
            term_puts(term, "\n[-] Invalid resolution format!");
        }
    }
}
#endif

void console_init(struct multiboot_tag *tag)
{
    terminal_t terminal;
    ps2_init_keyboard();

#ifdef __i386__
    struct multiboot_tag_framebuffer *fb_tag = find_fb_tag(tag);
    if (fb_tag == NULL) {
        // TODO: do proper error handling
        while (true)
            __asm__("hlt");
    }
    multiboot_fb_init_terminal(&terminal, fb_tag, &_framebuffer);
    _display_mode = DISPLAY_MODE_FRAMEBUFFER;
    term_puts(&terminal, "[*] Select a video mode:\n");
    term_puts(&terminal, "1. VGA Text Mode\n");
    term_puts(&terminal, "2. VESA Mode\n");

    char c;
    vbe_info_t info;
    while (true) {
        c = _wait_for_input();
        switch (c) {
        case '1':
            _init_vga_text_mode(&terminal);
            goto end;
            break;
        case '2':
            fb_destroy_terminal();
            info = vbe_get_info();
            _select_vbe_resolution(&terminal, &info, fb_tag);
            goto end;
            break;
        default:
            break;
        }
    }

end:
#else
    _display_mode = DISPLAY_MODE_VGA_TEXT;
    vga_init_terminal(&terminal);
#endif
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
