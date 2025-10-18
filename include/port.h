// include/port.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Display configuration provided to the port at init time.
typedef struct {
    uint8_t  i2c_addr;      // 0x3C or 0x3D for SSD1306 modules
    uint16_t width;         // 128
    uint16_t height;        // 64 or 32
} port_display_cfg_t;

/**
 * Initialize the platform layer (GPIO/I2C, timers).
 * Must be called before any other port_* or SSD1306 functions.
 * Returns 0 on success, <0 on error.
 */
int  port_init(const port_display_cfg_t *cfg);

/** Shut down the platform layer (optional). */
void port_shutdown(void);

/** Millisecond delay helper provided by the platform. */
void port_delay_ms(uint32_t ms);

/**
 * Write a single SSD1306 command byte.
 * Returns 0 on success or <0 on error.
 */
int  port_write_cmd(uint8_t cmd);

/**
 * Write N data bytes to the display (0x40 control prefix handled inside).
 * Safe to pass large buffers; the port may chunk them as needed.
 * Returns 0 on success or <0 on error.
 */
int  port_write_data(const uint8_t *data, size_t len);

/** Read-only access to the display configuration used by the port. */
const port_display_cfg_t* port_get_cfg(void);

#ifdef __cplusplus
}
#endif