// include/gfx.h
#pragma once
#include <stdint.h>
#include "ssd1306.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Set/clear a pixel in the framebuffer. */
void gfx_set_pixel(ssd1306_t *dev, int x, int y, int on);

/** Draw a 5x7 ASCII character at pixel (x,y). */
void gfx_draw_char(ssd1306_t *dev, int x, int y, char c);

/** Draw a null-terminated string starting at (x,y). 6 px advance per char. */
void gfx_draw_text(ssd1306_t *dev, int x, int y, const char *s);

#ifdef __cplusplus
}
#endif