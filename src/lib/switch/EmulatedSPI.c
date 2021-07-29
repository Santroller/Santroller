#include "EmulatedSPI.h"

uint8_t controller_color[] = {0x0A, 0xB9, 0xE6}; // Neon blue
uint8_t button_color[] = {0xDD, 0xDD, 0xDD}; // Light gray
uint8_t left_grip_color[] = {0xAA, 0xAA, 0xAA}; // Dark gray
uint8_t right_grip_color[] = {0xAA, 0xAA, 0xAA}; // Dark gray

static size_t min_size(size_t s1, size_t s2) {
    return s1 > s2 ? s2 : s1;
}

/*
 * Read 'size' bytes starting with 'address' and save them in 'buf'.
 * See https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/spi_flash_notes.md
 */
void spi_read(SPI_Address_t address, size_t size, uint8_t buf[]) {
    memset(buf, 0xFF, size);
    switch (address) {
        default:
        case ADDRESS_SERIAL_NUMBER: {
            // All 0xFF, leave buf as it is
            break;
        }
        case ADDRESS_CONTROLLER_COLOR: {
            if (size >= 3) {
                memcpy(&buf[0], &controller_color[0], sizeof(controller_color));
            }
            if (size >= 6) {
                memcpy(&buf[3], &button_color[0], sizeof(button_color));
            }
            if (size >= 9) {
                memcpy(&buf[6], &left_grip_color[0], sizeof(left_grip_color));
            }
            if (size >= 12) {
                memcpy(&buf[9], &right_grip_color[0], sizeof(right_grip_color));
            }
            break;
        }
        case ADDRESS_FACTORY_PARAMETERS_1: {
            uint8_t factory_parameters_1[] = {0x50, 0xfd, 0x00, 0x00, 0xc6, 0x0f, 0x0f, 0x30, 0x61, 0x96, 0x30, 0xf3,
                                              0xd4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c, 0x33, 0x36, 0x63};
            memcpy(&buf[0], &factory_parameters_1[0], min_size(size, sizeof(factory_parameters_1)));
            break;
        }
        case ADDRESS_FACTORY_PARAMETERS_2: {
            uint8_t factory_parameters_2[] = {0x0f, 0x30, 0x61, 0x96, 0x30, 0xf3, 0xd4, 0x14, 0x54,
                                              0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c, 0x33, 0x36, 0x63};
            memcpy(&buf[0], &factory_parameters_2[0], min_size(size, sizeof(factory_parameters_2)));
            break;
        }
        case ADDRESS_FACTORY_CALIBRATION_1: {
            uint8_t factory_calibration_1[] = {0xE6, 0xFF, 0x3A, 0x00, 0x39, 0x00, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
                                               0xF7, 0xFF, 0xFC, 0xFF, 0x00, 0x00, 0xE7, 0x3B, 0xE7, 0x3B, 0xE7, 0x3B};
            memcpy(&buf[0], &factory_calibration_1[0], min_size(size, sizeof(factory_calibration_1)));
            break;
        }
        case ADDRESS_FACTORY_CALIBRATION_2: {
            uint8_t factory_calibration_2[] = {0xba, 0x15, 0x62, 0x11, 0xb8, 0x7f, 0x29, 0x06, 0x5b, 0xff, 0xe7, 0x7e,
                                               0x0e, 0x36, 0x56, 0x9e, 0x85, 0x60, 0xff, 0x32, 0x32, 0x32, 0xff, 0xff,
                                               0xff};
            memcpy(&buf[0], &factory_calibration_2[0], min_size(size, sizeof(factory_calibration_2)));
            break;
        }
        case ADDRESS_STICKS_CALIBRATION: {
            if (size > 22) {
                buf[22] = 0xB2;
            }
            if (size > 23) {
                buf[23] = 0xA1;
            }
            // spi_response(data[11:13], bytes.fromhex('ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff b2 a1'))
            break;
        }
        case ADDRESS_IMU_CALIBRATION: {
            uint8_t imu_calibration[] = {0xbe, 0xff, 0x3e, 0x00, 0xf0, 0x01, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
                                         0xfe, 0xff, 0xfe, 0xff, 0x08, 0x00, 0xe7, 0x3b, 0xe7, 0x3b, 0xe7, 0x3b};
            memcpy(&buf[0], &imu_calibration[0], min_size(size, sizeof(imu_calibration)));
            break;
        }
    }
}