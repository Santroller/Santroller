// #include "secondary_pico.hpp"
// // TODO: for this stuff, is there a more reasonable protocol to use here?
// void secondaryPinMode(uint8_t pin, uint8_t pinMode) {
//     if (!secondary_initted) {
//         return;
//     }
//     for (int i = 0; i < RETRY_COUNT; i++) {
//         uint8_t payload[] = {pin, pinMode};
//         secondary_initted = twi_writeToPointer(secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_SET_PINMODE, sizeof(payload), payload);
//         if (secondary_initted) {
//             break;
//         }
//     }
// }

// uint32_t secondaryReadDigital() {
//     if (!secondary_initted) {
//         return 0;
//     }
//     for (int i = 0; i < RETRY_COUNT; i++) {
//         uint32_t payload = 0;
//         secondary_initted = twi_readFromPointer(secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_GET_DIGITAL, sizeof(payload), (uint8_t*)&payload);
//         if (secondary_initted) {
//             return payload;
//         }
//     }
//     return 0;
// }

// uint8_t secondaryReadDigital(uint8_t port, uint8_t mask) {
//     if (!secondary_initted) {
//         return 0;
//     }
//     for (int i = 0; i < RETRY_COUNT; i++) {
//         uint8_t payload2[2] = {port, mask};
//         secondary_initted = twi_writeToPointer(secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_GET_DIGITAL_PIN_2, sizeof(payload2), payload2);
//         if (!secondary_initted) {
//             continue;
//         }
//         uint32_t payload = 0;
//         secondary_initted = twi_readFromPointer(secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_GET_DIGITAL, sizeof(payload), (uint8_t*)&payload);
//         if (!secondary_initted) {
//             continue;
//         }
//         PIN_INIT_PERIPHERAL;
//         return (payload >> (port * 8)) & 0xFF;
//     }
//     return 0;
// }

// void secondaryWriteDigital(uint8_t pin, bool output) {
//     if (!secondary_initted) {
//         return;
//     }
//     for (int i = 0; i < RETRY_COUNT; i++) {
//         uint8_t payload[] = {pin, output};
//         secondary_initted = (secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_SET_PIN, sizeof(payload), payload);
//         if (secondary_initted) {
//             return;
//         }
//     }
// }

// void secondaryInitLED(uint8_t instance) {
//     if (!secondary_initted) {
//         return;
//     }
//     for (int i = 0; i < RETRY_COUNT; i++) {
//         secondary_initted = twi_writeSingleToPointer(secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_INIT_SPI, instance);
//         if (secondary_initted) {
//             return;
//         }
//     }
// }

// void secondaryWriteLED(uint8_t data) {
//     if (!secondary_initted) {
//         return;
//     }
//     for (int i = 0; i < RETRY_COUNT; i++) {
//         secondary_initted = twi_writeSingleToPointer(secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_WRITE_SPI, data);
//         if (secondary_initted) {
//             return;
//         }
//     }
// }

// bool secondaryInit() {
//     uint8_t data;
//     twi_readFromPointer(secondary_TWI_PORT, secondary_ADDR, secondary_COMMAND_INITIALISE, sizeof(data), &data);
//     return data == secondary_COMMAND_INITIALISE;
// }