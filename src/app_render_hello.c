// src/app_render_hello.c
#include "app_render_hello.h"
#include "gfx.h"
#include "ssd1306.h"

void app_render_hello(ssd1306_t *dev) {
    if (!dev) return;

    ssd1306_clear(dev);

    // Choose the vertical center "text row": each row is 8 px tall.
    const int rows = gfx_text_rows(dev);               // 128x64 → 8 rows
    const int center_row = (rows - 1) / 2;

    // Print centered
    gfx_print_line(dev, "HELLO WORLD!", center_row, GFX_ALIGN_CENTER);

    // Row indices are 0..gfx_text_rows(&dev)-1 (64px tall → 0..7)
    gfx_print_line(dev, "LEFT ALIGNED",   0, GFX_ALIGN_LEFT);
    gfx_print_line(dev, "CENTRE",         1, GFX_ALIGN_CENTER);   // alias: GFX_ALIGN_CENTRE
    gfx_print_line(dev, "RIGHT ALIGNED",          2, GFX_ALIGN_RIGHT);

    // Explicit x start (pixels from left)
    gfx_print_line(dev, "X=12",           3, 12);

    // Push to the display once
    ssd1306_update_full(dev);
}