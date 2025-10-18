// src/main.c
#include "port.h"
#include "ssd1306.h"
#include "app_calc.h"

int main(void) {
    const port_display_cfg_t cfg = {
        .i2c_addr = 0x3C,  // change to 0x3D if your panel uses it
        .width    = 128,
        .height   = 64
    };

    if (port_init(&cfg) != 0) return 1;

    ssd1306_t dev = {0};
    if (ssd1306_init(&dev) != 0) {
        port_shutdown();
        return 2;
    }

    // Run the calculator app (console-driven for now)
    app_run_calc(&dev);

    ssd1306_deinit(&dev);
    port_shutdown();
    return 0;
}