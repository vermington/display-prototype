// include/gfx.h
#pragma once
#include <stdint.h>
#include "ssd1306.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- Text row/char metrics ----
#define GFX_CHAR_ADVANCE     6   // 5px glyph + 1px spacing
#define GFX_CHAR_HEIGHT      7   // 7px tall font
// One "text row" is 8 pixels high (fits 7px font + 1px breathing room)

// ---- Alignment keywords (use negative sentinels so int can also be an x position) ----
#define GFX_ALIGN_LEFT      (-1)
#define GFX_ALIGN_CENTER    (-2)
#define GFX_ALIGN_CENTRE     GFX_ALIGN_CENTER
#define GFX_ALIGN_RIGHT     (-3)

// ---- Convenience: query rows and text width ----
int  gfx_text_rows(const ssd1306_t *dev);     // e.g., 8 rows on 128×64, 4 rows on 128×32
int  gfx_text_width(const char *s);           // pixels = len * GFX_CHAR_ADVANCE

/**
 * Print 'text' on a logical text row.
 * - 'row' is 0..gfx_text_rows(dev)-1 (page index; each row is 8 px tall).
 * - 'alignment_or_x':
 *     * GFX_ALIGN_LEFT / GFX_ALIGN_CENTRE / GFX_ALIGN_RIGHT
 *     * OR any non-negative integer x position (pixels from left).
 * Draws into the framebuffer only. Call ssd1306_update_full(dev) to push.
 * Returns 0 on success, <0 on error (e.g., row out of range).
 */
int  gfx_print_line(ssd1306_t *dev, const char *text, int row, int alignment_or_x);

/** Clear a single text row (8px tall band). Does NOT push to display. */
int  gfx_clear_line(ssd1306_t *dev, int row);

/** Set/clear a pixel in the framebuffer. */
void gfx_set_pixel(ssd1306_t *dev, int x, int y, int on);

/** Draw a 5x7 ASCII character at pixel (x,y). */
void gfx_draw_char(ssd1306_t *dev, int x, int y, char c);

/** Draw a null-terminated string starting at (x,y). 6 px advance per char. */
void gfx_draw_text(ssd1306_t *dev, int x, int y, const char *s);

// Draw a filled axis-aligned rectangle; 'on' = 1 sets pixels, 0 clears pixels.
void gfx_fill_rect(ssd1306_t *dev, int x, int y, int w, int h, int on);

// Draw just the rectangle outline.
void gfx_draw_rect(ssd1306_t *dev, int x, int y, int w, int h, int on);

#ifdef __cplusplus
}
#endif