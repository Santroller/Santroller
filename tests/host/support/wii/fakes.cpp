// Link time fakes for the I2C, alarm and GPIO calls the Wii extension code makes, and for the
// MIDI device the extension reader passes Guitar Hero drum notes to
#include "fake_bus.hpp"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/i2c_slave.h"
#include "pico/time.h"
#include "i2c.hpp"
#include "devices/midi.hpp"

i2c_inst_t fake_i2c_insts[NUM_I2CS] = {{0}, {1}};

namespace fake_i2c
{
std::deque<uint8_t> rx;
std::vector<uint8_t> tx;
i2c_slave_handler_t slave_handler[NUM_I2CS] = {};
uint8_t slave_address[NUM_I2CS] = {};
}

uint i2c_init(i2c_inst_t *, uint baudrate) { return baudrate; }
uint8_t i2c_read_byte_raw(i2c_inst_t *)
{
    if (fake_i2c::rx.empty())
    {
        return 0;
    }
    uint8_t value = fake_i2c::rx.front();
    fake_i2c::rx.pop_front();
    return value;
}
void i2c_write_byte_raw(i2c_inst_t *, uint8_t value) { fake_i2c::tx.push_back(value); }

void i2c_slave_init(i2c_inst_t *i2c, uint8_t address, i2c_slave_handler_t handler)
{
    fake_i2c::slave_handler[i2c->index] = handler;
    fake_i2c::slave_address[i2c->index] = address;
}
void i2c_slave_deinit(i2c_inst_t *i2c) { fake_i2c::slave_handler[i2c->index] = nullptr; }

namespace fake_gpio
{
bool level[64] = {};
}
void gpio_init(unsigned int) {}
void gpio_set_function(unsigned int, enum gpio_function) {}
void gpio_pull_up(unsigned int) {}
void gpio_set_dir(unsigned int, bool) {}
void gpio_put(unsigned int gpio, bool value) { fake_gpio::level[gpio] = value; }

namespace fake_alarm
{
Pending pending;
static alarm_id_t next_id = 1;
bool fire()
{
    if (!pending.callback)
    {
        return false;
    }
    Pending alarm = pending;
    pending = Pending();
    alarm.callback(alarm.id, alarm.user_data);
    return true;
}
void reset()
{
    pending = Pending();
}
} // namespace fake_alarm

alarm_id_t add_alarm_in_us(uint64_t us, alarm_callback_t callback, void *user_data, bool)
{
    fake_alarm::pending = {callback, user_data, us, fake_alarm::next_id++};
    return fake_alarm::pending.id;
}
alarm_id_t add_alarm_in_ms(uint32_t ms, alarm_callback_t callback, void *user_data, bool fire_if_past)
{
    return add_alarm_in_us((uint64_t)ms * 1000, callback, user_data, fire_if_past);
}
bool cancel_alarm(alarm_id_t id)
{
    if (fake_alarm::pending.callback && fake_alarm::pending.id == id)
    {
        fake_alarm::pending = fake_alarm::Pending();
        return true;
    }
    return false;
}

namespace fake_i2c_master
{
std::vector<Transfer> transfers;
}

I2CMasterInterface::I2CMasterInterface(uint8_t block, int8_t sda, int8_t scl, uint32_t clock)
    : m_sda(sda), m_scl(scl), m_clock(clock)
{
    i2c = _hardwareBlocks[block];
}
I2CMasterInterface::~I2CMasterInterface() {}
void I2CMasterInterface::dmaInit(uint8_t, I2CDMAInterface *) {}
void I2CMasterInterface::dmaDeinit(uint8_t) {}
void I2CMasterInterface::tick() {}
void I2CMasterInterface::dmaWriteRead(uint8_t addr, const uint8_t *wbuf, size_t wbuf_len, uint8_t *rbuf,
                                      size_t rbuf_len)
{
    fake_i2c_master::transfers.push_back(
        {addr, std::vector<uint8_t>(wbuf, wbuf + wbuf_len), rbuf, rbuf_len});
}
void I2CMasterInterface::dmaWriteRead(uint8_t addr, const uint8_t *wbuf, size_t wbuf_len, uint8_t *rbuf,
                                      size_t rbuf_len, uint16_t *)
{
    dmaWriteRead(addr, wbuf, wbuf_len, rbuf, rbuf_len);
}

namespace fake_midi
{
std::vector<std::vector<uint8_t>> packets;
}

// Never touches the device, so the tests can pass any MidiDevice pointer
void MidiDevice::process_midi_data(uint8_t *data, uint16_t len)
{
    fake_midi::packets.emplace_back(data, data + len);
}

void reset_wii_fakes()
{
    fake_i2c::rx.clear();
    fake_i2c::tx.clear();
    fake_alarm::reset();
    fake_i2c_master::transfers.clear();
    fake_midi::packets.clear();
    for (bool &level : fake_gpio::level)
    {
        level = false;
    }
}
