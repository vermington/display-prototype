// src/main.c
#include "port.h"
#include "ssd1306.h"
#include "app_render_hello.h"

int main(void) {
    // Centralized platform/display configuration
    const port_display_cfg_t cfg = {
        .i2c_addr = 0x3C,  // change to 0x3D if your module uses it
        .width    = 128,
        .height   = 64
    };

    if (port_init(&cfg) != 0) return 1;

    ssd1306_t dev = {0};
    if (ssd1306_init(&dev) != 0) {
        port_shutdown();
        return 2;
    }

    // Call the app/layer you want to render
    app_render_hello(&dev);

    port_delay_ms(5000);

    ssd1306_deinit(&dev);
    port_shutdown();
    return 0;
}