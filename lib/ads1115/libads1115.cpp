#include "libads1115.hpp"
#include "main.hpp"
#include "stdio.h"

bool ADS1115::writeRegister(uint8_t reg, uint16_t val)
{
    uint8_t data[] = {(uint8_t)((val >> 8) & 0xFF), (uint8_t)(val & 0xFF)};
    return interface.writeRegister(ADS1115_ADDRESS_ADDR_GND, reg, sizeof(data), data);
}
uint16_t ADS1115::readRegister(uint8_t reg)
{
    uint8_t data[2] = {};
    if (!interface.readRegister(ADS1115_ADDRESS_ADDR_GND, reg, sizeof(data), data))
    {
        return 0;
    }
    return (data[0] << 8) + data[1];
}
static volatile bool rdy[NUM_BANK0_GPIOS];
void ADS1115::tick()
{
    if (!connected)
    {
        init();
        return;
    }
    if (rdy[alert])
    {
        rdy[alert] = false;
        inputs[current] = readRegister(ADS1115_RA_CONVERSION) << 1;
        current++;
        if (current > 3)
        {
            current = 0;
        }
        config &= ~(0xF000);
        config |= (0x4000 + (current * 0x1000));
        config |= 1<<15;
        connected = writeRegister(ADS1115_RA_LO_THRESH, 0x0000);
        connected = writeRegister(ADS1115_RA_HI_THRESH, 1 << 15);
        connected = writeRegister(ADS1115_RA_CONFIG, config);
    }
}
void cb(uint gpio, uint32_t event_mask) {
    rdy[gpio] = true;
}
void ADS1115::init()
{
    if (alert == 0xFF) {
        return;
    }
    if (!readRegister(ADS1115_RA_CONFIG))
    {
        connected = false;
        return;
    }
    connected = true;
    current = 0;
    gpio_init(alert);
    gpio_set_dir(alert, false);
    gpio_pull_up(alert);
    
    gpio_set_irq_enabled_with_callback(alert, GPIO_IRQ_EDGE_FALL, true, cb);
    config = ADS1115_REG_RESET_VAL;
    connected = writeRegister(ADS1115_RA_LO_THRESH, 0x0000);
    connected = writeRegister(ADS1115_RA_HI_THRESH, 1 << 15);
    connected = writeRegister(ADS1115_RA_CONFIG, config);
    printf("configured %d\r\n", alert);
}