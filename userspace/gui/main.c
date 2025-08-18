/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <microui.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "./framebuffer.h"
#include "./input.h"
#include "./renderer.h"

static char logbuf[64000];
static int logbuf_updated = 0;
static float bg[3] = {90, 95, 100};

static int mouse_pos_x = 0;
static int mouse_pos_y = 0;
static bool mouse_right_click = false;
static bool mouse_left_click = false;

static mu_Context ctx;
framebuffer_t fb;
uint32_t framebuffer[2073600];

static void write_log(const char *text)
{
    if (logbuf[0]) {
        strcat(logbuf, "\n");
    }
    strcat(logbuf, text);
    logbuf_updated = 1;
}

static void log_window(mu_Context *ctx)
{
    if (mu_begin_window(ctx, "Log Window", mu_rect(350, 40, 300, 200))) {
        /* output text panel */
        mu_layout_row(ctx, 1, (int[]) {-1}, -25);
        mu_begin_panel(ctx, "Log Output");
        mu_Container *panel = mu_get_current_container(ctx);
        mu_layout_row(ctx, 1, (int[]) {-1}, -1);
        mu_text(ctx, logbuf);
        mu_end_panel(ctx);
        if (logbuf_updated) {
            panel->scroll.y = panel->content_size.y;
            logbuf_updated = 0;
        }

        /* input textbox + submit button */
        static char buf[128];
        int submitted = 0;
        mu_layout_row(ctx, 2, (int[]) {-70, -1}, 0);
        if (mu_textbox(ctx, buf, sizeof(buf)) & MU_RES_SUBMIT) {
            mu_set_focus(ctx, ctx->last_id);
            submitted = 1;
        }
        if (mu_button(ctx, "Submit")) {
            submitted = 1;
        }
        if (submitted) {
            write_log(buf);
            buf[0] = '\0';
        }

        mu_end_window(ctx);
    }
}

static void test_window(mu_Context *ctx)
{
    /* do window */
    if (mu_begin_window(ctx, "Demo Window", mu_rect(40, 40, 300, 450))) {
        mu_Container *win = mu_get_current_container(ctx);
        win->rect.w = mu_max(win->rect.w, 240);
        win->rect.h = mu_max(win->rect.h, 300);

        /* window info */
        if (mu_header(ctx, "Window Info")) {
            mu_Container *win = mu_get_current_container(ctx);
            char buf[64];
            mu_layout_row(ctx, 2, (int[]) {54, -1}, 0);
            mu_label(ctx, "Position:");
            sprintf(buf, "%d, %d", win->rect.x, win->rect.y);
            mu_label(ctx, buf);
            mu_label(ctx, "Size:");
            sprintf(buf, "%d, %d", win->rect.w, win->rect.h);
            mu_label(ctx, buf);
        }

        /* labels + buttons */
        if (mu_header_ex(ctx, "Test Buttons", MU_OPT_EXPANDED)) {
            mu_layout_row(ctx, 3, (int[]) {86, -110, -1}, 0);
            mu_label(ctx, "Test buttons 1:");
            if (mu_button(ctx, "Button 1")) {
                write_log("Pressed button 1");
            }
            if (mu_button(ctx, "Button 2")) {
                write_log("Pressed button 2");
            }
            mu_label(ctx, "Test buttons 2:");
            if (mu_button(ctx, "Button 3")) {
                write_log("Pressed button 3");
            }
            if (mu_button(ctx, "Popup")) {
                mu_open_popup(ctx, "Test Popup");
            }
            if (mu_begin_popup(ctx, "Test Popup")) {
                mu_button(ctx, "Hello");
                mu_button(ctx, "World");
                mu_end_popup(ctx);
            }
        }

        /* tree */
        if (mu_header_ex(ctx, "Tree and Text", MU_OPT_EXPANDED)) {
            mu_layout_row(ctx, 2, (int[]) {140, -1}, 0);
            mu_layout_begin_column(ctx);
            if (mu_begin_treenode(ctx, "Test 1")) {
                if (mu_begin_treenode(ctx, "Test 1a")) {
                    mu_label(ctx, "Hello");
                    mu_label(ctx, "world");
                    mu_end_treenode(ctx);
                }
                if (mu_begin_treenode(ctx, "Test 1b")) {
                    if (mu_button(ctx, "Button 1")) {
                        write_log("Pressed button 1");
                    }
                    if (mu_button(ctx, "Button 2")) {
                        write_log("Pressed button 2");
                    }
                    mu_end_treenode(ctx);
                }
                mu_end_treenode(ctx);
            }
            if (mu_begin_treenode(ctx, "Test 2")) {
                mu_layout_row(ctx, 2, (int[]) {54, 54}, 0);
                if (mu_button(ctx, "Button 3")) {
                    write_log("Pressed button 3");
                }
                if (mu_button(ctx, "Button 4")) {
                    write_log("Pressed button 4");
                }
                if (mu_button(ctx, "Button 5")) {
                    write_log("Pressed button 5");
                }
                if (mu_button(ctx, "Button 6")) {
                    write_log("Pressed button 6");
                }
                mu_end_treenode(ctx);
            }
            if (mu_begin_treenode(ctx, "Test 3")) {
                static int checks[3] = {1, 0, 1};
                mu_checkbox(ctx, "Checkbox 1", &checks[0]);
                mu_checkbox(ctx, "Checkbox 2", &checks[1]);
                mu_checkbox(ctx, "Checkbox 3", &checks[2]);
                mu_end_treenode(ctx);
            }
            mu_layout_end_column(ctx);

            mu_layout_begin_column(ctx);
            mu_layout_row(ctx, 1, (int[]) {-1}, 0);
            mu_text(
                ctx,
                "Lorem ipsum dolor sit amet, consectetur adipiscing "
                "elit. Maecenas lacinia, sem eu lacinia molestie, mi risus faucibus "
                "ipsum, eu varius magna felis a nulla.");
            mu_layout_end_column(ctx);
        }

        /* background color sliders */
        if (mu_header_ex(ctx, "Background Color", MU_OPT_EXPANDED)) {
            mu_layout_row(ctx, 2, (int[]) {-78, -1}, 74);
            /* sliders */
            mu_layout_begin_column(ctx);
            mu_layout_row(ctx, 2, (int[]) {46, -1}, 0);
            mu_label(ctx, "Red:");
            mu_slider(ctx, &bg[0], 0, 255);
            mu_label(ctx, "Green:");
            mu_slider(ctx, &bg[1], 0, 255);
            mu_label(ctx, "Blue:");
            mu_slider(ctx, &bg[2], 0, 255);
            mu_layout_end_column(ctx);
            /* color preview */
            mu_Rect r = mu_layout_next(ctx);
            mu_draw_rect(ctx, r, mu_color(bg[0], bg[1], bg[2], 255));
            char buf[32];
            sprintf(buf, "#%02X%02X%02X", (int) bg[0], (int) bg[1], (int) bg[2]);
            mu_draw_control_text(ctx, buf, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
        }

        mu_end_window(ctx);
    }
}

static int uint8_slider(mu_Context *ctx, unsigned char *value, int low, int high)
{
    static float tmp;
    mu_push_id(ctx, &value, sizeof(value));
    tmp = *value;
    int res = mu_slider_ex(ctx, &tmp, low, high, 0, "%.0f", MU_OPT_ALIGNCENTER);
    *value = tmp;
    mu_pop_id(ctx);
    return res;
}

static void style_window(mu_Context *ctx)
{
    static struct
    {
        const char *label;
        int idx;
    } colors[]
        = {{"text:", MU_COLOR_TEXT},
           {"border:", MU_COLOR_BORDER},
           {"windowbg:", MU_COLOR_WINDOWBG},
           {"titlebg:", MU_COLOR_TITLEBG},
           {"titletext:", MU_COLOR_TITLETEXT},
           {"panelbg:", MU_COLOR_PANELBG},
           {"button:", MU_COLOR_BUTTON},
           {"buttonhover:", MU_COLOR_BUTTONHOVER},
           {"buttonfocus:", MU_COLOR_BUTTONFOCUS},
           {"base:", MU_COLOR_BASE},
           {"basehover:", MU_COLOR_BASEHOVER},
           {"basefocus:", MU_COLOR_BASEFOCUS},
           {"scrollbase:", MU_COLOR_SCROLLBASE},
           {"scrollthumb:", MU_COLOR_SCROLLTHUMB},
           {NULL}};

    if (mu_begin_window(ctx, "Style Editor", mu_rect(350, 250, 300, 240))) {
        int sw = mu_get_current_container(ctx)->body.w * 0.14;
        mu_layout_row(ctx, 6, (int[]) {80, sw, sw, sw, sw, -1}, 0);
        for (int i = 0; colors[i].label; i++) {
            mu_label(ctx, colors[i].label);
            uint8_slider(ctx, &ctx->style->colors[i].r, 0, 255);
            uint8_slider(ctx, &ctx->style->colors[i].g, 0, 255);
            uint8_slider(ctx, &ctx->style->colors[i].b, 0, 255);
            uint8_slider(ctx, &ctx->style->colors[i].a, 0, 255);
            mu_draw_rect(ctx, mu_layout_next(ctx), ctx->style->colors[i]);
        }
        mu_end_window(ctx);
    }
}

static void draw_cursor()
{
    r_draw_icon(
        MU_ICON_CLOSE,
        (mu_Rect) {.x = mouse_pos_x, .y = mouse_pos_y},
        (mu_Color) {.a = 255, .r = 0, .g = 0, .b = 0});
}

static void process_frame(mu_Context *ctx)
{
    mu_begin(ctx);
    style_window(ctx);
    log_window(ctx);
    test_window(ctx);
    mu_end(ctx);
}

static int text_width(mu_Font font, const char *text, int len)
{
    if (len == -1) {
        len = strlen(text);
    }
    return r_get_text_width(text, len);
}

static int text_height(mu_Font font)
{
    return r_get_text_height();
}

static void mouse_handler(mouse_event_t event)
{
    int delta_x = event.x_sign ? (event.x_movement | 0xFFFFFF00) : event.x_movement;
    int delta_y = event.y_sign ? (event.y_movement | 0xFFFFFF00) : event.y_movement;
    int new_mouse_x = mouse_pos_x + delta_x;
    int new_mouse_y = mouse_pos_y - delta_y;
    if (new_mouse_x >= 0 && (size_t) new_mouse_x < fb.width)
        mouse_pos_x = new_mouse_x;
    if (new_mouse_y >= 0 && (size_t) new_mouse_y < fb.height)
        mouse_pos_y = new_mouse_y;

    mu_input_mousemove(&ctx, mouse_pos_x, mouse_pos_y);

    if (event.right_button != mouse_right_click) {
        if (event.right_button)
            mu_input_mousedown(&ctx, mouse_pos_x, mouse_pos_y, MU_MOUSE_RIGHT);
        else
            mu_input_mouseup(&ctx, mouse_pos_x, mouse_pos_y, MU_MOUSE_RIGHT);
    }
    if (event.left_button != mouse_left_click) {
        if (event.left_button)
            mu_input_mousedown(&ctx, mouse_pos_x, mouse_pos_y, MU_MOUSE_LEFT);
        else
            mu_input_mouseup(&ctx, mouse_pos_x, mouse_pos_y, MU_MOUSE_LEFT);
    }
    mouse_right_click = event.right_button;
    mouse_left_click = event.left_button;
}

static const char key_map[256] = {
    [KEY_LSHIFT] = MU_KEY_SHIFT,
    [KEY_RSHIFT] = MU_KEY_SHIFT,
    [KEY_CTRL] = MU_KEY_CTRL,
    [KEY_ALT] = MU_KEY_ALT,
    [KEY_RETURN] = MU_KEY_RETURN,
    [KEY_BACKSPACE] = MU_KEY_BACKSPACE,
};

static const char char_map[256] = {0x0, 0x0, '1', '2',  '3',  '4', '5',  '6',  '7', '8', '9',
                                   '0', '-', '=', '\b', '\t', 'q', 'w',  'e',  'r', 't', 'y',
                                   'u', 'i', 'o', 'p',  '[',  ']', '\n', 0x0,  'a', 's', 'd',
                                   'f', 'g', 'h', 'j',  'k',  'l', ';',  '\'', '`', 0x0, '\\',
                                   'z', 'x', 'c', 'v',  'b',  'n', 'm',  ',',  '.', '/', 0x0,
                                   '*', 0x0, ' ', 0x0,  0x0,  0x0, 0x0,  0x0,  0x0, 0x0, 0x0,
                                   0x0, 0x0, 0x0, 0x0,  0x0,  '7', '8',  '9',  '-', '4', '5',
                                   '6', '+', '1', '2',  '3',  '0', '.',  0x0,  0x0, 0x0};
static const char char_map_shifted[256] = {0x0, 0x0, '!', '@',  '#',  '$', '%',  '^', '&', '*', '(',
                                           ')', '_', '+', '\b', '\t', 'Q', 'W',  'E', 'R', 'T', 'Y',
                                           'U', 'I', 'O', 'P',  '{',  '}', '\n', 0x0, 'A', 'S', 'D',
                                           'F', 'G', 'H', 'J',  'K',  'L', ':',  '"', '~', 0x0, '|',
                                           'Z', 'X', 'C', 'V',  'B',  'N', 'M',  '<', '>', '?', 0x0,
                                           '*', 0x0, ' ', 0x0,  0x0,  0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
                                           0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0,  0x0, '-', 0x0, 0x0,
                                           0x0, '+', 0x0, 0x0,  0x0,  0x0, 0x0,  0x0, 0x0, 0x0};

static bool is_shifted = false;
static bool is_capslock = false;
static void keyboard_handler(keyboard_event_t event)
{
    void (*key_action)(mu_Context *ctx, int key) = event.action == KEYBOARD_PRESSED
                                                           || event.action == KEYBOARD_HOLD
                                                       ? mu_input_keydown
                                                       : mu_input_keyup;
    if (event.scancode == KEY_LSHIFT || event.scancode == KEY_RSHIFT)
        is_shifted = event.action == KEYBOARD_PRESSED || event.action == KEYBOARD_HOLD;
    if (event.action == KEYBOARD_PRESSED && event.scancode == KEY_CAPSLOCK)
        is_capslock = !is_capslock;

    if (key_map[event.scancode])
        key_action(&ctx, key_map[event.scancode]);
    else if (
        char_map[event.scancode]
        && (event.action == KEYBOARD_PRESSED || event.action == KEYBOARD_HOLD)) {
        char text[2]
            = {is_shifted || is_capslock ? char_map_shifted[event.scancode]
                                         : char_map[event.scancode],
               '\0'};
        mu_input_text(&ctx, text);
    }
}

int main()
{
    int result;
    __asm__ volatile("int $0x80" : "=a"(result) : "a"(0x01), "D"(&fb) : "memory");
    for (size_t i = 0; i < (fb.height * fb.width); i++)
        framebuffer[i] = 0xff000000;
    register_mouse_event_handler(mouse_handler);
    register_keyboard_event_handler(keyboard_handler);

    r_init();

    /* init microui */
    mu_init(&ctx);
    ctx.text_width = text_width;
    ctx.text_height = text_height;

    /* main loop */
    for (;;) {
        /* process frame */
        process_frame(&ctx);

        /* render */
        r_clear(mu_color(bg[0], bg[1], bg[2], 255));
        mu_Command *cmd = NULL;
        while (mu_next_command(&ctx, &cmd)) {
            switch (cmd->type) {
            case MU_COMMAND_TEXT:
                r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color);
                break;
            case MU_COMMAND_RECT:
                r_draw_rect(cmd->rect.rect, cmd->rect.color);
                break;
            case MU_COMMAND_ICON:
                r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
                break;
            case MU_COMMAND_CLIP:
                r_set_clip_rect(cmd->clip.rect);
                break;
            }
        }
        draw_cursor();
        r_present();
    }
}
