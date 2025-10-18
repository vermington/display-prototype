// src/app_hello.c
#include "port.h"
#include "ssd1306.h"
#include "gfx.h"

static void app_render_hello(ssd1306_t *dev) {
    ssd1306_clear(dev);
    // 11 chars * 6 px = 66 px → center horizontally on 128
    int x = (int)(dev->width - 66) / 2;
    int y = (int)(dev->height / 2) - 4;
    gfx_draw_text(dev, x, y, "HELLO WORLD!");
    ssd1306_update_full(dev);
}

int main(void) {
    // All config lives here (and is consumed by the port). Change this once per platform.
    const port_display_cfg_t cfg = {
        .i2c_addr = 0x3C,   // change to 0x3D if needed
        .width    = 128,
        .height   = 64
    };

    if (port_init(&cfg) != 0) return 1;

    ssd1306_t dev = {0};
    if (ssd1306_init(&dev) != 0) return 2;

    app_render_hello(&dev);
    port_delay_ms(5000);

    ssd1306_deinit(&dev);
    port_shutdown();
    return 0;
}