// include/ssd1306.h
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t width;     // pixels
    uint16_t height;    // pixels
    uint8_t  pages;     // height / 8
    uint8_t *buffer;    // width * pages bytes, owned by the driver
} ssd1306_t;

/** Initialize driver: allocates framebuffer, configures panel, turns display ON. */
int  ssd1306_init(ssd1306_t *dev);

/** Deinitialize driver: frees framebuffer; does not power-cycle the bus. */
void ssd1306_deinit(ssd1306_t *dev);

/** Clear framebuffer to 0 (off). */
void ssd1306_clear(ssd1306_t *dev);

/** Push entire framebuffer to the panel (full refresh). */
int  ssd1306_update_full(const ssd1306_t *dev);

/** Optional helpers */
int  ssd1306_set_contrast(uint8_t value);   // 0x00..0xFF
int  ssd1306_set_invert(bool enable);       // invert pixels
int  ssd1306_display_on(bool on);           // true=ON, false=OFF

#ifdef __cplusplus
}
#endif