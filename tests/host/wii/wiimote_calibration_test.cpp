// The emulated Wii remote reports acceleration in the scale its EEPROM gives the console (wiibrew:
// accelerometer calibration at 0x16, X0 Y0 Z0 then XG YG ZG as bits 9-2, with their low bits in the byte
// after each set), so the console reads the motion that was mapped.
#include <gtest/gtest.h>
#include "eeprom.bin.h"
#include "protocols/motion.hpp"

TEST(WiimoteCalibration, MatchesTheEmulatedEeprom)
{
    for (int axis = 0; axis < 3; axis++)
    {
        EXPECT_EQ(eeprom_bin_1[0x16 + axis] << 2, WIIMOTE_EMULATED_ACCEL_ZERO);
        EXPECT_EQ(eeprom_bin_1[0x1A + axis] << 2, WIIMOTE_EMULATED_ACCEL_ONE_G);
    }
    // No low bits
    EXPECT_EQ(eeprom_bin_1[0x19], 0);
    EXPECT_EQ(eeprom_bin_1[0x1D], 0);
}

TEST(WiimoteCalibration, ConsoleReadsTheMappedMotion)
{
    // Turned a quarter clockwise around Z from lying flat, like a tilted guitar: gravity along -X in SDL's
    // axes, which the remote has on its own +X
    MotionState motion;
    motion.write(MotionAccelX, motion_clamp16(-1.0f * MOTION_ACCEL_PER_G));
    motion.write(MotionAccelY, 0);
    motion.write(MotionAccelZ, 0);
    uint16_t accel[3];
    wiimote_accel_encode(motion, accel);
    // How the console works it out from the calibration
    auto read = [](uint16_t value)
    {
        return (value - (float)WIIMOTE_EMULATED_ACCEL_ZERO) /
               (WIIMOTE_EMULATED_ACCEL_ONE_G - WIIMOTE_EMULATED_ACCEL_ZERO);
    };
    EXPECT_NEAR(read(accel[0]), 1.0f, 0.01f);
    EXPECT_NEAR(read(accel[1]), 0.0f, 0.01f);
    EXPECT_NEAR(read(accel[2]), 0.0f, 0.01f);
}
