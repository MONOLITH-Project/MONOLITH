/*
 * Copyright (c) 2025, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <microui.h>
#include <stdint.h>
#include <string.h>

#include "./atlas.inl"
#include "./framebuffer.h"
#include "./renderer.h"

extern framebuffer_t fb;
extern uint32_t framebuffer[2073600];

static inline uint32_t muColor2ARGB(mu_Color color)
{
    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

static void blit_quad(mu_Rect dst, mu_Rect src, mu_Color color)
{
    /* Prepack the tint color */
    for (int dy = 0; dy < dst.h; dy++) {
        size_t sy = src.y + (dy * src.h) / dst.h; /* scale if needed */
        size_t fy = dst.y + dy;
        if (fy < 0 || fy >= fb.height)
            continue;

        for (int dx = 0; dx < dst.w; dx++) {
            size_t sx = src.x + (dx * src.w) / dst.w; /* scale if needed */
            size_t fx = dst.x + dx;
            if (fx < 0 || fx >= fb.width)
                continue;

            uint8_t alpha = atlas_texture[sy * ATLAS_WIDTH + sx];
            if (alpha == 0)
                continue;

            /* Blend with framebuffer (simple alpha blend) */
            uint32_t *dst_px = &framebuffer[fy * fb.width + fx];
            uint32_t dst_col = *dst_px;

            uint8_t dr = (dst_col >> 16) & 0xFF;
            uint8_t dg = (dst_col >> 8) & 0xFF;
            uint8_t db = dst_col & 0xFF;
            uint8_t da = (dst_col >> 24) & 0xFF;

            uint8_t sr = color.r;
            uint8_t sg = color.g;
            uint8_t sb = color.b;
            uint8_t sa = (color.a * alpha) / 255;

            uint8_t out_a = sa + ((da * (255 - sa)) / 255);
            uint8_t out_r = (sr * sa + dr * (255 - sa)) / 255;
            uint8_t out_g = (sg * sa + dg * (255 - sa)) / 255;
            uint8_t out_b = (sb * sa + db * (255 - sa)) / 255;

            *dst_px = (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
        }
    }
}

void r_init()
{
    /* TODO */
}

void r_draw_rect(mu_Rect rect, mu_Color color)
{
    blit_quad(rect, atlas[ATLAS_WHITE], color);
}

void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color)
{
    mu_Rect dst = {pos.x, pos.y, 0, 0};
    for (const char *p = text; *p; p++) {
        if ((*p & 0xc0) == 0x80) {
            continue;
        }
        int chr = mu_min((unsigned char) *p, 127);
        mu_Rect src = atlas[ATLAS_FONT + chr];
        dst.w = src.w;
        dst.h = src.h;
        blit_quad(dst, src, color);
        dst.x += dst.w;
    }
}

void r_draw_icon(int id, mu_Rect rect, mu_Color color)
{
    mu_Rect src = atlas[id];
    int x = rect.x + (rect.w - src.w) / 2;
    int y = rect.y + (rect.h - src.h) / 2;
    blit_quad(mu_rect(x, y, src.w, src.h), src, color);
}

int r_get_text_width(const char *text, int len)
{
    int res = 0;
    for (const char *p = text; *p && len--; p++) {
        if ((*p & 0xc0) == 0x80)
            continue;
        int chr = mu_min((unsigned char) *p, 127);
        res += atlas[ATLAS_FONT + chr].w;
    }
    return res;
}

int r_get_text_height(void)
{
    return 18;
}

void r_set_clip_rect(mu_Rect rect)
{
    /* TODO */
}

void r_clear(mu_Color clr)
{
    uint32_t color = muColor2ARGB(clr);
    for (size_t y = 0; y < fb.height; y++) {
        for (size_t x = 0; x < fb.width; x++) {
            framebuffer[y * fb.width + x] = color;
        }
    }
}

void r_present(void)
{
    memcpy(fb.framebuffer, framebuffer, sizeof(uint32_t) * fb.width * fb.height);
}
