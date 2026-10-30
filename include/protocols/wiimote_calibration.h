#pragma once

// The emulated Wii remote's accelerometer calibration, as its EEPROM (eeprom.bin.h, from 0x16) tells the
// console: 0x85 at rest and 0xA0 at 1 g on every axis, in the top 8 of the 10 bits. Whatever the emulated
// remote reports has to use this scale, as it's what the console reads the values with.
#define WIIMOTE_EMULATED_ACCEL_ZERO (0x85 << 2)
#define WIIMOTE_EMULATED_ACCEL_ONE_G (0xA0 << 2)
#define WIIMOTE_EMULATED_ACCEL_PER_G (WIIMOTE_EMULATED_ACCEL_ONE_G - WIIMOTE_EMULATED_ACCEL_ZERO)
