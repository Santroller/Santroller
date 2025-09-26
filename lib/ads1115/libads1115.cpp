#include "libads1115.hpp"
#include "main.hpp"
void ADS1115::tick()
{
    if (!connected)
    {
        init();
        return;
    }
    for (int i = 0; i < 4; i++) {
        config[1] &= ~(0b111 << 4);
        config[1] |= (i+4) << 4;
        connected = interface.writeRegister(ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONFIG, sizeof(config), (uint8_t*)&config);
        while (connected && gpio_get(17)) {
            tight_loop_contents();
        }
        uint8_t val[2];
        connected = interface.readRegister(ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONVERSION, sizeof(val), val);
        inputs[i] = val[0] << 8 | val[1];
    }
    
}

void ADS1115::init()
{
    connected = interface.readRegister(ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONFIG, sizeof(config), config);
    gpio_init(17);
    gpio_set_dir(17, false);
    
}

// void ADS1115::tick()
// {
//     if (!connected)
//     {
//         init();
//         return;
//     }
//     if (!gpio_get(17))
//     {
//         uint8_t val[2];
//         connected = interface.readRegister(ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONVERSION, sizeof(val), val);
//         inputs[current] = val[0] << 8 | val[1];
//         current++;
//         if (current > 3) {
//             current = 0;
//         }
//         config[0] &= ~(0b111 << 4);
//         config[0] |= (current + 4) << 4;
//         connected = interface.writeRegister(ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONFIG, sizeof(config), (uint8_t *)&config);
//     }
// }

// void ADS1115::init()
// {
//     current = 0;
//     connected = interface.readRegister(ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONFIG, sizeof(config), config);
//     gpio_init(17);
//     gpio_set_dir(17, false);
//     config[0] &= ~(0b111 << 4);
//     config[0] |= (current + 4) << 4;
//     config[0] |= 1<<7;
//     connected = interface.writeRegister(ADS1115_ADDRESS_ADDR_GND, ADS1115_RA_CONFIG, sizeof(config), (uint8_t *)&config);
// }