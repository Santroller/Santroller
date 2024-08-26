#include <stddef.h>

#include "Arduino.h"
#include "config.h"
#include "io.h"
#include "ps2.h"
#include "util.h"
#ifdef TICK_PS2

uint8_t configMode = 0;
uint8_t mode = 0x41;
#if SUPPORTS_PICO
uint8_t receiveCommand() {
    // The pi pico has a odd bug, where the first byte is essentially garbage when using SPI slave.
    // Since the PS2 requires the first byte be 0xFF, we just override whatever nonsense
    // The SPI hardware is trying to output, and force all zeros.
    // It gets worse though. The garbage byte receives the first response, which
    // Misaligns all reads and writes, so we have to then misalign all our reads and writes
    // In the other direction to make things come out correctly.
    gpio_set_oeover(PS2_SPI_MOSI, GPIO_OVERRIDE_LOW);
    uint8_t modeByte = configMode ? 0xF3 : mode;
    PS2_OUTPUT_ACK_SET();
    uint8_t ret = spi_transfer(PS2_OUTPUT_SPI_PORT, modeByte);
    PS2_OUTPUT_ACK_CLEAR();
    sleep_us(2);
    PS2_OUTPUT_ACK_SET();
    if (ret != 1) {
        return 0;
    }
    gpio_set_oeover(PS2_SPI_MOSI, GPIO_OVERRIDE_NORMAL);
    ret = spi_transfer(PS2_OUTPUT_SPI_PORT, 0x5A);
    PS2_OUTPUT_ACK_CLEAR();
    return ret;
}
uint8_t receiveAll(uint8_t* data, uint8_t len) {
    for (int i = 0; i < len - 1; i++) {
        PS2_OUTPUT_ACK_SET();
        if (!PS2_OUTPUT_ATT_READ()) {
            return 0;
        }
        data[i] = spi_transfer(PS2_OUTPUT_SPI_PORT, data[i + 1]);
        PS2_OUTPUT_ACK_CLEAR();
        sleep_us(2);
    }
    PS2_OUTPUT_ACK_SET();
    spi_transfer(PS2_OUTPUT_SPI_PORT, data[len - 1]);
    return len;
}
uint8_t receiveAll(uint8_t* data0, uint8_t* data1, uint8_t len) {
    uint8_t* data = data0;
    for (int i = 0; i < len - 1; i++) {
        PS2_OUTPUT_ACK_SET();
        data0[i] = spi_transfer(PS2_OUTPUT_SPI_PORT, data[i + 1]);
        if (data0[1] == 1) {
            data = data1;
        }
        PS2_OUTPUT_ACK_CLEAR();
        sleep_us(2);
    }
    PS2_OUTPUT_ACK_SET();
    spi_transfer(PS2_OUTPUT_SPI_PORT, data[len - 1]);
    return len;
}
#else
uint8_t receiveCommand() {
    uint8_t modeByte = configMode ? 0xF3 : mode;
    PS2_OUTPUT_ACK_SET();
    uint8_t ret = spi_transfer(PS2_OUTPUT_SPI_PORT, 0xFF);
    PS2_OUTPUT_ACK_CLEAR();
    sleep_us(2);
    PS2_OUTPUT_ACK_SET();
    if (ret != 1) {
        return 0;
    }
    ret = spi_transfer(PS2_OUTPUT_SPI_PORT, mode);
    PS2_OUTPUT_ACK_CLEAR();
    return ret;
}
uint8_t receiveAll(uint8_t* data, uint8_t len) {
    for (int i = 0; i < len - 1; i++) {
        PS2_OUTPUT_ACK_SET();
        data[i] = spi_transfer(PS2_OUTPUT_SPI_PORT, data[i]);
        PS2_OUTPUT_ACK_CLEAR();
        sleep_us(2);
    }
    PS2_OUTPUT_ACK_SET();
    data[len - 1] = spi_transfer(PS2_OUTPUT_SPI_PORT, data[len - 1]);
    return len;
}
uint8_t receiveAll(uint8_t* data0, uint8_t* data1, uint8_t len) {
    uint8_t* data = data0;
    for (int i = 0; i < len - 1; i++) {
        PS2_OUTPUT_ACK_SET();
        data0[i] = spi_transfer(PS2_OUTPUT_SPI_PORT, data[i]);
        if (data0[1] == 1) {
            data = data1;
        }
        PS2_OUTPUT_ACK_CLEAR();
    }
    PS2_OUTPUT_ACK_SET();
    data[len - 1] = spi_transfer(PS2_OUTPUT_SPI_PORT, data[len - 1]);
    return len;
}
#endif
int counter = 0;

uint8_t buttons1 = 0xff;
uint8_t buttons2 = 0xff;

uint8_t motorSetting1 = 0xff;
uint8_t motorSetting2 = 0xff;

bool analogEnabled = 0;

int timeoutCounter = 0;
uint8_t cmd;
uint8_t param1;
uint8_t param2;
uint8_t param3;
void ps2_emu_init() {
    PS2_OUTPUT_ACK_SET();
    cmd = 0;
    param1 = 0;
    param2 = 0;
    param3 = 0;
// Guitars need to hold dpad left
#if DEVICE_TYPE_IS_GUITAR
    buttons1 &= ~0x80;
#endif
}
bool ps2_emu_tick(PS2_REPORT* report) {
    if (PS2_OUTPUT_ATT_READ()) {
        timeoutCounter++;
        if (timeoutCounter >= 30000)  // we've been disconnected
        {
            mode = 0x41;
            motorSetting1 = 0xff;
            motorSetting2 = 0xff;
            analogEnabled = 0;
            configMode = 0;
            timeoutCounter = 0;
        }
        return false;
    }
    timeoutCounter = 0;
    while (1) {
        cmd = receiveCommand();
        if (!cmd)
            break;

        if (configMode) {
            switch (cmd) {
                case 0x40: {
                    uint8_t data[] = {0x5A, 0x00, 0x00, 0x02, 0x00, 0x00, 0x5A};
                    receiveAll(data, sizeof(data));
                    break;
                }
                case 0x41:  // check response data
                    if (analogEnabled == 1) {
                        // button pressures byte mask, appears after 0x44 is called
                        uint8_t data[] = {0x5A, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x5A};
                        receiveAll(data, sizeof(data));
                    } else {
                        uint8_t data[] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                        receiveAll(data, sizeof(data));
                    }
                    break;
                case 0x42: {
                    uint8_t data[] = {0x5A, buttons1, buttons2};
                    receiveAll(data, sizeof(data));
                    break;
                }
                case 0x43: {  // exit config mode
                    uint8_t data[] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                    receiveAll(data, sizeof(data));
                    if (data[1] == 0) {
                        configMode = 0;
                    }
                    break;
                }
                case 0x44: {  // turn on analog mode
                    uint8_t data[] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                    receiveAll(data, sizeof(data));
                    param1 = data[1];
                    if (param1 == 1) {
                        mode = 0x73;
                        analogEnabled = 1;
                    } else {
                        mode = 0x41;  // no analog button pressures
                        analogEnabled = 0;
                    }
                    break;
                }
                case 0x45: {  // query model
                    if (analogEnabled) {
                        uint8_t data[] = {0x5A, 0x01, 0x02, 0x01, 0x02, 0x01, 0x00};
                        receiveAll(data, sizeof(data));
                    } else {
                        uint8_t data[] = {0x5A, 0x01, 0x02, 0x00, 0x02, 0x01, 0x00};
                        receiveAll(data, sizeof(data));
                    }
                    break;
                }
                case 0x46: {
                    uint8_t data0[] = {0x5A, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0F};
                    uint8_t data1[] = {0x5A, 0x00, 0x00, 0x01, 0x01, 0x01, 0x0F};
                    receiveAll(data0, data1, sizeof(data0));
                    break;
                }
                case 0x47: {
                    uint8_t data[] = {0x5A, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00};
                    receiveAll(data, sizeof(data));
                    break;
                }
                case 0x4C: {
                    uint8_t data0[] = {0x5A, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00};
                    uint8_t data1[] = {0x5A, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00};
                    receiveAll(data0, data1, sizeof(data0));
                    break;
                }
                case 0x4D: {  // setup motors
                    uint8_t data[] = {0x5A, motorSetting1, motorSetting2, 0xFF, 0xFF, 0xFF, 0xFF};
                    receiveAll(data, sizeof(data));
                    motorSetting1 = data[1];
                    motorSetting2 = data[2];
                    break;
                }
                case 0x4F: {      // set pressure byte mask, extended pressures
                    mode = 0x79;  // analog button pressures
                    uint8_t data[] = {0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A};
                    receiveAll(data, sizeof(data));
                    break;
                }
            }
            while (!PS2_OUTPUT_ATT_READ()) {
                sleep_us(20);  // conservative so we aren't still in ATT low at the top of the loop
            }
            continue;
        } else {
            if (mode == 0x41)  // digital only
            {
                uint8_t data[3] = {0x5A};
                memcpy(data, report, sizeof(data));
                // Buttons are inverted
                data[1] ^= 0xFF;
                data[2] ^= 0xFF;
                receiveAll(data, sizeof(data));
                param1 = data[1];
                param2 = data[2];
            }
            if (mode == 0x73)  // digital buttons, analog joysticks
            {
                uint8_t data[7] = {0x5A};
                memcpy(data, report, sizeof(data));
                // Buttons are inverted
                data[1] ^= 0xFF;
                data[2] ^= 0xFF;
                receiveAll(data, sizeof(data));
                param1 = data[1];
            }
            if (mode == 0x79)  // analog joysticks, analog buttons
            {
                uint8_t data[19] = {0x5A};
                memcpy(data, report, sizeof(data));
                // Buttons are inverted
                data[1] ^= 0xFF;
                data[2] ^= 0xFF;
                receiveAll(data, sizeof(data));
                param1 = data[1];
            }
            // cmd should usually be 0x42 for polling
            if (cmd == 0x43) {
                if (param1 == 0x01) {
                    configMode = 1;
                    while (!PS2_OUTPUT_ATT_READ()) {
                        sleep_us(20);  // conservative so we aren't still in ATT low at the top of the loop
                    }
                    continue;
                }
            }
            break;
        }
    }
    PS2_OUTPUT_ACK_SET();

    while (!PS2_OUTPUT_ATT_READ()) {
        sleep_us(20);  // conservative so we aren't still in ATT low at the top of the loop
    }
    return true;
}
#endif
