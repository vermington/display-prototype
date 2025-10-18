// src/port_wiringpi.c
// Raspberry Pi port using WiringPi + Linux I2C. Replace this file for other MCUs.

#include "port.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#ifndef PORT_I2C_CHUNK
#define PORT_I2C_CHUNK  16   // bytes per I2C write burst (plus 1 control byte)
#endif

static int                 s_fd   = -1;      // I2C file descriptor
static port_display_cfg_t  s_cfg;            // active config

int port_init(const port_display_cfg_t *cfg) {
    if (!cfg) return -1;
    s_cfg = *cfg;

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPiSetup failed\n");
        return -2;
    }
    s_fd = wiringPiI2CSetup(s_cfg.i2c_addr);
    if (s_fd < 0) {
        fprintf(stderr, "I2C open failed (addr 0x%02X)\n", s_cfg.i2c_addr);
        return -3;
    }
    return 0;
}

void port_shutdown(void) {
    // Nothing specific for WiringPi; left for symmetry / other ports.
}

void port_delay_ms(uint32_t ms) { delay(ms); }

int port_write_cmd(uint8_t cmd) {
    // Control byte 0x00 indicates "command"
    int rc = wiringPiI2CWriteReg8(s_fd, 0x00, cmd);
    return (rc == -1) ? -1 : 0;
}

int port_write_data(const uint8_t *data, size_t len) {
    if (!data || len == 0) return 0;
    // Write as repeated bursts: [0x40, d0..dN]
    uint8_t buf[1 + PORT_I2C_CHUNK];
    buf[0] = 0x40; // control byte for "data"
    size_t written = 0;

    while (written < len) {
        size_t chunk = len - written;
        if (chunk > PORT_I2C_CHUNK) chunk = PORT_I2C_CHUNK;
        memcpy(&buf[1], &data[written], chunk);
        ssize_t n = write(s_fd, buf, (unsigned)(1 + chunk));
        if (n < 0) return -1;
        written += chunk;
    }
    return 0;
}

const port_display_cfg_t* port_get_cfg(void) {
    return &s_cfg;
}