#pragma once
// Fake hardware/i2c.h: just the types and raw byte calls the Wii code uses. Bytes the "Wiimote"
// writes to the emulated extension come from fake_i2c::rx, bytes it answers go to fake_i2c::tx
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <deque>
#include <vector>
#include "pico/time.h"

// The Pico SDK defines this in pico/platform
#ifndef __unused
#define __unused __attribute__((unused))
#endif

typedef unsigned int uint;
typedef void (*irq_handler_t)(void);
typedef struct i2c_inst
{
    int index;
} i2c_inst_t;
#define NUM_I2CS 2
extern i2c_inst_t fake_i2c_insts[NUM_I2CS];
#define i2c0 (&fake_i2c_insts[0])
#define i2c1 (&fake_i2c_insts[1])

uint i2c_init(i2c_inst_t *i2c, uint baudrate);
uint8_t i2c_read_byte_raw(i2c_inst_t *i2c);
void i2c_write_byte_raw(i2c_inst_t *i2c, uint8_t value);

namespace fake_i2c
{
extern std::deque<uint8_t> rx;
extern std::vector<uint8_t> tx;
}
