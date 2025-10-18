// include/app_render_hello.h
#pragma once
#include "ssd1306.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Render the Hello World screen. */
void app_render_hello(ssd1306_t *dev);

#ifdef __cplusplus
}
#endif