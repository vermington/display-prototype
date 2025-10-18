// src/app_render_hello.c
#include "app_render_hello.h"
#include "gfx.h"
#include "ssd1306.h"

void app_render_hello(ssd1306_t *dev) {
    if (!dev) return;

    ssd1306_clear(dev);

    // "HELLO WORLD!" is 12 chars. At 6 px advance each → 72 px total.
    int text_px = 12 * 6;
    int x = (int)(dev->width  - text_px) / 2;
    int y = (int)(dev->height / 2) - 4;  // vertically center-ish for 7px glyphs

    gfx_draw_text(dev, x, y, "HELLO WORLD!");
    ssd1306_update_full(dev);
}