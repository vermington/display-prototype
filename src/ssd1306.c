// src/ssd1306.c
#include "ssd1306.h"
#include "port.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int ssd1306_cmd(uint8_t c)             { return port_write_cmd(c); }
static int ssd1306_data(const uint8_t* d, size_t n) { return port_write_data(d, n); }

static int ssd1306_configure_panel(const ssd1306_t *dev) {
    (void)dev;
    const port_display_cfg_t *cfg = port_get_cfg();
    const uint8_t com_pins = (cfg->height == 64) ? 0x12 : 0x02; // panel variant

    // Standard init sequence (horizontal addressing mode)
    if (ssd1306_cmd(0xAE) < 0) return -1;               // Display OFF
    if (ssd1306_cmd(0xD5) < 0 || ssd1306_cmd(0x80) < 0) return -1; // Clock
    if (ssd1306_cmd(0xA8) < 0 || ssd1306_cmd((uint8_t)(cfg->height - 1)) < 0) return -1; // Multiplex
    if (ssd1306_cmd(0xD3) < 0 || ssd1306_cmd(0x00) < 0) return -1; // Display offset
    if (ssd1306_cmd(0x40) < 0) return -1;               // Start line = 0
    if (ssd1306_cmd(0x8D) < 0 || ssd1306_cmd(0x14) < 0) return -1; // Charge pump on
    if (ssd1306_cmd(0x20) < 0 || ssd1306_cmd(0x00) < 0) return -1; // Horizontal addressing
    if (ssd1306_cmd(0xA1) < 0) return -1;               // Segment remap
    if (ssd1306_cmd(0xC8) < 0) return -1;               // COM scan dec
    if (ssd1306_cmd(0xDA) < 0 || ssd1306_cmd(com_pins) < 0) return -1; // COM pins
    if (ssd1306_cmd(0x81) < 0 || ssd1306_cmd(0x7F) < 0) return -1;     // Contrast
    if (ssd1306_cmd(0xD9) < 0 || ssd1306_cmd(0xF1) < 0) return -1;     // Pre-charge
    if (ssd1306_cmd(0xDB) < 0 || ssd1306_cmd(0x40) < 0) return -1;     // VCOM
    if (ssd1306_cmd(0xA4) < 0) return -1;               // Resume RAM content
    if (ssd1306_cmd(0xA6) < 0) return -1;               // Normal (non-inverted)
    if (ssd1306_cmd(0x2E) < 0) return -1;               // Deactivate scroll
    if (ssd1306_cmd(0xAF) < 0) return -1;               // Display ON
    return 0;
}

int ssd1306_init(ssd1306_t *dev) {
    if (!dev) return -1;
    const port_display_cfg_t *cfg = port_get_cfg();
    if (!cfg || cfg->width == 0 || cfg->height == 0) return -2;

    dev->width  = cfg->width;
    dev->height = cfg->height;
    dev->pages  = (uint8_t)(cfg->height / 8);
    size_t bytes = (size_t)dev->width * dev->pages;
    dev->buffer = (uint8_t*)malloc(bytes);
    if (!dev->buffer) return -3;

    memset(dev->buffer, 0, bytes);
    if (ssd1306_configure_panel(dev) < 0) return -4;
    return 0;
}

void ssd1306_deinit(ssd1306_t *dev) {
    if (dev && dev->buffer) { free(dev->buffer); dev->buffer = NULL; }
}

void ssd1306_clear(ssd1306_t *dev) {
    if (!dev || !dev->buffer) return;
    memset(dev->buffer, 0, (size_t)dev->width * dev->pages);
}

int ssd1306_update_full(const ssd1306_t *dev) {
    if (!dev || !dev->buffer) return -1;
    // Set window to full screen: columns 0..W-1, pages 0..P-1
    if (ssd1306_cmd(0x21) < 0) return -1;
    if (ssd1306_cmd(0x00) < 0) return -1;
    if (ssd1306_cmd((uint8_t)(dev->width - 1)) < 0) return -1;

    if (ssd1306_cmd(0x22) < 0) return -1;
    if (ssd1306_cmd(0x00) < 0) return -1;
    if (ssd1306_cmd((uint8_t)(dev->pages - 1)) < 0) return -1;

    size_t n = (size_t)dev->width * dev->pages;
    return ssd1306_data(dev->buffer, n);
}

int ssd1306_set_contrast(uint8_t value) {
    if (ssd1306_cmd(0x81) < 0) return -1;
    return ssd1306_cmd(value);
}

int ssd1306_set_invert(bool enable) {
    return ssd1306_cmd(enable ? 0xA7 : 0xA6);
}

int ssd1306_display_on(bool on) {
    return ssd1306_cmd(on ? 0xAF : 0xAE);
}