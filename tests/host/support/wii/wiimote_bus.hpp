#pragma once
// Plays the Wii Remote's side of the extension I2C bus against the emulated extension, through
// the I2C slave handler the emulation registered with the fake pico/i2c_slave.h
#include <stdint.h>
#include <initializer_list>
#include <vector>
#include <gtest/gtest.h>
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"

struct WiimoteBus
{
    uint8_t block = 0;

    i2c_slave_handler_t handler() const { return fake_i2c::slave_handler[block]; }
    i2c_inst_t *inst() const { return &fake_i2c_insts[block]; }

    // One write transaction: the register address, then the data
    void write(uint8_t reg, std::initializer_list<uint8_t> data) { write(reg, std::vector<uint8_t>(data)); }
    void write(uint8_t reg, const std::vector<uint8_t> &data)
    {
        ASSERT_NE(handler(), nullptr) << "nothing is answering on the bus";
        fake_i2c::rx.push_back(reg);
        handler()(inst(), I2C_SLAVE_RECEIVE);
        for (uint8_t byte : data)
        {
            fake_i2c::rx.push_back(byte);
            handler()(inst(), I2C_SLAVE_RECEIVE);
        }
        handler()(inst(), I2C_SLAVE_FINISH);
    }

    // Set the register pointer, then read len bytes in a second transaction
    std::vector<uint8_t> read(uint8_t reg, size_t len)
    {
        std::vector<uint8_t> result;
        if (!handler())
        {
            ADD_FAILURE() << "nothing is answering on the bus";
            return result;
        }
        fake_i2c::rx.push_back(reg);
        handler()(inst(), I2C_SLAVE_RECEIVE);
        handler()(inst(), I2C_SLAVE_FINISH);
        fake_i2c::tx.clear();
        for (size_t i = 0; i < len; i++)
        {
            handler()(inst(), I2C_SLAVE_REQUEST);
        }
        handler()(inst(), I2C_SLAVE_FINISH);
        result = fake_i2c::tx;
        fake_i2c::tx.clear();
        return result;
    }

    // wiibrew's "new way" of initialising an extension, which leaves it unencrypted
    void init_new_way()
    {
        write(0xF0, {0x55});
        write(0xFB, {0x00});
    }

    // The 16 byte key at 0x40, in the 6, 6 and 4 byte blocks wiibrew says real extensions need
    void write_key(const uint8_t key[16])
    {
        write(0x40, std::vector<uint8_t>(key, key + 6));
        write(0x46, std::vector<uint8_t>(key + 6, key + 12));
        write(0x4C, std::vector<uint8_t>(key + 12, key + 16));
    }
};

inline std::vector<uint8_t> bytes(std::initializer_list<uint8_t> list) { return std::vector<uint8_t>(list); }
