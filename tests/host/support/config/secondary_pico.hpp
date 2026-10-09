#pragma once
// Fake secondary_pico.hpp, with just enough of hardware/i2c.h for it
#include "config_fakes.hpp"

typedef unsigned int uint;
struct i2c_inst_t
{
    int index;
};
inline i2c_inst_t fake_i2c_blocks[2] = {{0}, {1}};
#define i2c0 (&fake_i2c_blocks[0])
#define i2c1 (&fake_i2c_blocks[1])

inline void secondary_pico_slave_init(i2c_inst_t *, uint, uint, uint) { fake_loader::state.secondary_pico_inits++; }
