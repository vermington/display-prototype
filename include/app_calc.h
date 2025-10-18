#pragma once
#include "ssd1306.h"

#ifdef __cplusplus
extern "C" {
#endif

// Runs the calculator: reads tokens from stdin and renders on the OLED.
// Tokens now: hex number (0x... or plain hex), operator '+', 'c' to clear, 'q' to quit.
// All arithmetic is 64-bit unsigned; result wraps on overflow.
void app_run_calc(ssd1306_t *dev);

#ifdef __cplusplus
}
#endif