#pragma once
// Fake pico/i2c_slave.h: remembers the handler so a test can play the Wiimote
#include "hardware/i2c.h"

typedef enum i2c_slave_event_t
{
    I2C_SLAVE_RECEIVE,
    I2C_SLAVE_REQUEST,
    I2C_SLAVE_FINISH,
} i2c_slave_event_t;
typedef void (*i2c_slave_handler_t)(i2c_inst_t *i2c, i2c_slave_event_t event);

void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler);
void i2c_slave_deinit(i2c_inst_t *i2c);

namespace fake_i2c
{
// Per block, null when the block isn't a slave
extern i2c_slave_handler_t slave_handler[NUM_I2CS];
extern uint8_t slave_address[NUM_I2CS];
}
