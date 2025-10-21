#include <stdint.h>
#include "io.h"
#include "ads1115.h"
#ifdef ADS115_TWI_PORT
static uint16_t config;
static volatile bool rdy;
static bool connected = false;
static uint8_t current = 0;
static uint16_t inputs[4];
bool writeRegister(uint8_t reg, uint16_t val)
{
    uint8_t data[] = {(uint8_t)((val >> 8) & 0xFF), (uint8_t)(val & 0xFF)};
    return twi_writeToPointer(ADS115_TWI_PORT, ADS1115_ADDRESS_ADDR_GND, reg, sizeof(data), data);
}
uint16_t readRegister(uint8_t reg)
{
    uint8_t data[2] = {};
    if (!twi_readFromPointer(ADS115_TWI_PORT, ADS1115_ADDRESS_ADDR_GND, reg, sizeof(data), data))
    {
        return 0;
    }
    return (data[0] << 8) + data[1];
}
void tick()
{
    if (!connected)
    {
        init();
        return;
    }
    config = readRegister(ADS1115_RA_CONFIG);
    if (rdy)
    {
        rdy = false;
        inputs[current] = readRegister(ADS1115_RA_CONVERSION) << 1;
        current++;
        if (current > 3)
        {
            current = 0;
        }
        config &= ~(0xF000);
        config |= (0x4000 + (current * 0x1000));
        config |= 1<<15;
        connected = writeRegister(ADS1115_RA_CONFIG, config);

    }
}
void cb(uint gpio, uint32_t event_mask) {
    rdy = true;
}
void init()
{
    if (!readRegister(ADS1115_RA_CONFIG))
    {
        connected = false;
        return;
    }
    connected = true;
    current = 0;
    gpio_init(ADS115_ALERT);
    gpio_set_dir(ADS115_ALERT, false);
    gpio_pull_up(ADS115_ALERT);
    
    gpio_set_irq_enabled_with_callback(ADS115_ALERT, GPIO_IRQ_EDGE_FALL, true, &cb);
    config = ADS1115_REG_RESET_VAL;
    connected = writeRegister(ADS1115_RA_CONFIG, ADS1115_REG_RESET_VAL);
    connected = writeRegister(ADS1115_RA_LO_THRESH, (0 << 15));
    connected = writeRegister(ADS1115_RA_HI_THRESH, (1 << 15));
}
#endif