#include <gtest/gtest.h>
#include "psx_poll_command.hpp"

TEST(PsxPollCommand, MotorsStayOffUntilMapped)
{
    const std::array<uint8_t, 5> expected = {0x01, 0x42, 0x00, 0x00, 0x00};
    for (auto type : {PS2ControllerTypeDualshock, PS2ControllerTypeDualshock2})
    {
        EXPECT_EQ(psx_poll_command(type, false, 255, 255), expected);
    }
}

TEST(PsxPollCommand, UnsupportedControllersNeverReceiveMotorValues)
{
    const std::array<uint8_t, 5> expected = {0x01, 0x42, 0x00, 0x00, 0x00};
    for (auto type : {PS2ControllerTypeUnknown, PS2ControllerTypeDigital,
                      PS2ControllerTypeGuitar, PS2ControllerTypeFlightStick,
                      PS2ControllerTypeNegCon, PS2ControllerTypeJogCon,
                      PS2ControllerTypeGunCon, PS2ControllerTypeMouse,
                      PS2ControllerTypeTaiko, PS2ControllerTypePopNMusic, PS2ControllerTypeDrumMania,
                      PS2ControllerTypeGuitarFreaks})
    {
        EXPECT_EQ(psx_poll_command(type, true, 255, 255), expected);
    }
}

TEST(PsxPollCommand, LeftMotorRetainsIntensityAndRightMotorIsBinary)
{
    for (auto type : {PS2ControllerTypeDualshock, PS2ControllerTypeDualshock2})
    {
        for (unsigned left = 0; left <= 255; ++left)
        {
            for (unsigned right = 0; right <= 255; ++right)
            {
                const std::array<uint8_t, 5> expected = {
                    0x01, 0x42, 0x00, uint8_t(right ? 1 : 0), uint8_t(left)};
                EXPECT_EQ(psx_poll_command(type, true, left, right), expected);
            }
        }
    }
}

TEST(PsxPollCommand, ZeroValuesStopBothMotors)
{
    const std::array<uint8_t, 5> expected = {0x01, 0x42, 0x00, 0x00, 0x00};
    for (auto type : {PS2ControllerTypeDualshock, PS2ControllerTypeDualshock2})
    {
        EXPECT_EQ(psx_poll_command(type, true, 0, 0), expected);
    }
}
