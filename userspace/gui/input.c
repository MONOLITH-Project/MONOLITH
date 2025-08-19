/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <microui.h>
#include <stddef.h>

#include "./framebuffer.h"
#include "./input.h"
#include "./renderer.h"

static int _mouse_pos_x = 0;
static int _mouse_pos_y = 0;
static bool _mouse_right_click = false;
static bool _mouse_left_click = false;
static bool _is_shifted = false;
static bool _is_capslock = false;

static const char _key_map[256] = {
    [KEY_LSHIFT] = MU_KEY_SHIFT,
    [KEY_RSHIFT] = MU_KEY_SHIFT,
    [KEY_CTRL] = MU_KEY_CTRL,
    [KEY_ALT] = MU_KEY_ALT,
    [KEY_RETURN] = MU_KEY_RETURN,
    [KEY_BACKSPACE] = MU_KEY_BACKSPACE,
};

static const char _char_map[256] = {0x0, 0x0, '1', '2',  '3',  '4', '5',  '6',  '7', '8', '9',
                                    '0', '-', '=', '\b', '\t', 'q', 'w',  'e',  'r', 't', 'y',
                                    'u', 'i', 'o', 'p',  '[',  ']', '\n', 0x0,  'a', 's', 'd',
                                    'f', 'g', 'h', 'j',  'k',  'l', ';',  '\'', '`', 0x0, '\\',
                                    'z', 'x', 'c', 'v',  'b',  'n', 'm',  ',',  '.', '/', 0x0,
                                    '*', 0x0, ' ', 0x0,  0x0,  0x0, 0x0,  0x0,  0x0, 0x0, 0x0,
                                    0x0, 0x0, 0x0, 0x0,  0x0,  '7', '8',  '9',  '-', '4', '5',
                                    '6', '+', '1', '2',  '3',  '0', '.',  0x0,  0x0, 0x0};
static const char _char_map_shifted[256]
    = {0x0, 0x0, '!', '@', '#', '$', '%', '^', '&', '*', '(',  ')', '_', '+', '\b', '\t', 'Q', 'W',
       'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0x0, 'A', 'S', 'D',  'F',  'G', 'H',
       'J', 'K', 'L', ':', '"', '~', 0x0, '|', 'Z', 'X', 'C',  'V', 'B', 'N', 'M',  '<',  '>', '?',
       0x0, '*', 0x0, ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0,
       0x0, 0x0, '-', 0x0, 0x0, 0x0, '+', 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0};

extern framebuffer_t fb;
extern mu_Context ctx;

int register_mouse_event_handler(mouse_event_handler_t handler)
{
    int result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(0x02), "D"(handler) : "memory");
    return result;
}

int register_keyboard_event_handler(keyboard_event_handler_t handler)
{
    int result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(0x03), "D"(handler) : "memory");
    return result;
}

static void _mouse_handler(mouse_event_t event)
{
    int delta_x = event.x_sign ? (event.x_movement | 0xFFFFFF00) : event.x_movement;
    int delta_y = event.y_sign ? (event.y_movement | 0xFFFFFF00) : event.y_movement;
    int new_mouse_x = _mouse_pos_x + delta_x;
    int new_mouse_y = _mouse_pos_y - delta_y;
    if (new_mouse_x >= 0 && (size_t) new_mouse_x < fb.width)
        _mouse_pos_x = new_mouse_x;
    if (new_mouse_y >= 0 && (size_t) new_mouse_y < fb.height)
        _mouse_pos_y = new_mouse_y;

    mu_input_mousemove(&ctx, _mouse_pos_x, _mouse_pos_y);

    if (event.right_button != _mouse_right_click) {
        if (event.right_button)
            mu_input_mousedown(&ctx, _mouse_pos_x, _mouse_pos_y, MU_MOUSE_RIGHT);
        else
            mu_input_mouseup(&ctx, _mouse_pos_x, _mouse_pos_y, MU_MOUSE_RIGHT);
    }
    if (event.left_button != _mouse_left_click) {
        if (event.left_button)
            mu_input_mousedown(&ctx, _mouse_pos_x, _mouse_pos_y, MU_MOUSE_LEFT);
        else
            mu_input_mouseup(&ctx, _mouse_pos_x, _mouse_pos_y, MU_MOUSE_LEFT);
    }
    _mouse_right_click = event.right_button;
    _mouse_left_click = event.left_button;
}

static void _keyboard_handler(keyboard_event_t event)
{
    void (*key_action)(mu_Context *ctx, int key) = event.action == KEYBOARD_PRESSED
                                                           || event.action == KEYBOARD_HOLD
                                                       ? mu_input_keydown
                                                       : mu_input_keyup;
    if (event.scancode == KEY_LSHIFT || event.scancode == KEY_RSHIFT)
        _is_shifted = event.action == KEYBOARD_PRESSED || event.action == KEYBOARD_HOLD;
    if (event.action == KEYBOARD_PRESSED && event.scancode == KEY_CAPSLOCK)
        _is_capslock = !_is_capslock;

    if (_key_map[event.scancode])
        key_action(&ctx, _key_map[event.scancode]);
    else if (
        _char_map[event.scancode]
        && (event.action == KEYBOARD_PRESSED || event.action == KEYBOARD_HOLD)) {
        char text[2]
            = {_is_shifted || _is_capslock ? _char_map_shifted[event.scancode]
                                           : _char_map[event.scancode],
               '\0'};
        mu_input_text(&ctx, text);
    }
}

void init_input()
{
    register_mouse_event_handler(_mouse_handler);
    register_keyboard_event_handler(_keyboard_handler);
}

void draw_cursor()
{
    r_draw_icon(
        MU_ICON_CLOSE,
        (mu_Rect) {.x = _mouse_pos_x, .y = _mouse_pos_y},
        (mu_Color) {.a = 255, .r = 0, .g = 0, .b = 0});
}
