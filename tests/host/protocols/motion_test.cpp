#include <gtest/gtest.h>
#include "protocols/motion.hpp"
#include "protocols/switch.hpp"

// Each console's motion format, checked against how SDL reads that controller back into its axes
// (X right, Y up out of the face, Z towards the player).
namespace
{
MotionState lying_flat()
{
    return MotionState();
}

MotionState turning(MotionAxis axis, float dps)
{
    MotionState motion;
    motion.write(axis, (int32_t)roundf(dps * MOTION_GYRO_PER_DPS));
    return motion;
}

TEST(Motion, AxisValuesCentreOnZero)
{
    EXPECT_EQ(motion_accel_axis(0), MOTION_ZERO);
    EXPECT_EQ(motion_accel_axis(1), MOTION_ZERO + 4096);
    EXPECT_EQ(motion_accel_axis(-8), 0);
    EXPECT_EQ(motion_gyro_axis(2000), UINT16_MAX);
    EXPECT_EQ(motion_from_axis(motion_gyro_axis(-250)), (int32_t)roundf(-250 * MOTION_GYRO_PER_DPS));
}

TEST(Motion, NothingMappedLiesFlatAndStill)
{
    MotionState motion = lying_flat();
    EXPECT_EQ(motion.set, 0);
    EXPECT_FLOAT_EQ(motion.accel_g(1), 1.0f);
    EXPECT_FLOAT_EQ(motion.accel_g(0), 0.0f);
    EXPECT_FLOAT_EQ(motion.gyro_dps(1), 0.0f);
}

// SDL: x = -switch y, y = switch z, z = -switch x, at 4096 per g with the emulated calibration
TEST(Motion, SwitchLyingFlatHasGravityOnZ)
{
    SwitchImuSample sample = switch_imu_encode(lying_flat());
    EXPECT_EQ(sample.accX, 0);
    EXPECT_EQ(sample.accY, 0);
    EXPECT_EQ(sample.accZ, 4096);
}

TEST(Motion, SwitchYawIsGyroZ)
{
    SwitchImuSample sample = switch_imu_encode(turning(MotionGyroY, 100));
    // 0x343B / 936 counts per °/s
    EXPECT_NEAR(sample.gyroZ, 1428.5, 1);
    EXPECT_EQ(sample.gyroX, 0);
    EXPECT_EQ(sample.gyroY, 0);
    // pitch and roll come back negated
    EXPECT_LT(switch_imu_encode(turning(MotionGyroX, 100)).gyroY, 0);
    EXPECT_LT(switch_imu_encode(turning(MotionGyroZ, 100)).gyroX, 0);
}

TEST(Motion, SwitchRoundTrips)
{
    MotionState motion;
    motion.write(MotionAccelX, 1000);
    motion.write(MotionAccelY, -2000);
    motion.write(MotionAccelZ, 3000);
    motion.write(MotionGyroX, -4000);
    motion.write(MotionGyroY, 5000);
    motion.write(MotionGyroZ, -6000);
    float accel[3], gyro[3];
    switch_imu_decode(switch_imu_encode(motion), SwitchImuCalibration(), accel, gyro);
    for (int i = 0; i < 3; i++)
    {
        EXPECT_NEAR(accel[i], motion.accel_g(i), 0.001f);
        EXPECT_NEAR(gyro[i], motion.gyro_dps(i), 0.1f);
    }
}

TEST(Motion, SwitchCalibrationMatchesTheEmulatedController)
{
    // what the emulated controller serves at 0x6020: zero origins, 0x4000 for 4 g and 0x343B for 936 °/s
    const uint8_t data[24] = {0, 0, 0, 0, 0, 0, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
                              0, 0, 0, 0, 0, 0, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34};
    SwitchImuCalibration cal = switch_imu_parse_calibration(data);
    for (int i = 0; i < 3; i++)
    {
        EXPECT_FLOAT_EQ(cal.accel_per_g[i], SWITCH_IMU_ACCEL_PER_G);
        EXPECT_FLOAT_EQ(cal.gyro_per_dps[i], SWITCH_IMU_GYRO_PER_DPS);
        EXPECT_FLOAT_EQ(cal.gyro_origin[i], 0);
    }
}

TEST(Motion, SwitchCalibrationUsesItsOrigins)
{
    // a real controller's factory calibration (as the emulated one used to serve): accel origins
    // -29, -199, 493 and gyro origins 9, -22, -95
    const uint8_t data[24] = {0xE3, 0xFF, 0x39, 0xFF, 0xED, 0x01, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
                              0x09, 0x00, 0xEA, 0xFF, 0xA1, 0xFF, 0x3B, 0x34, 0x3B, 0x34, 0x3B, 0x34};
    SwitchImuCalibration cal = switch_imu_parse_calibration(data);
    // like SDL, 4 / (coefficient - origin) g per count
    EXPECT_FLOAT_EQ(cal.accel_per_g[0], (0x4000 + 29) / 4.0f);
    EXPECT_FLOAT_EQ(cal.gyro_origin[1], -22);
    EXPECT_FLOAT_EQ(cal.gyro_per_dps[2], (0x343B + 95) / 936.0f);
    // the gyro origin is its bias
    SwitchImuSample still = {0, 0, 4096, 9, -22, -95};
    float accel[3], gyro[3];
    switch_imu_decode(still, cal, accel, gyro);
    for (int i = 0; i < 3; i++)
    {
        EXPECT_FLOAT_EQ(gyro[i], 0);
    }
}

// A subcommand reply: report 0x21, timer, the inputs, ack, subcommand, then the SPI address, length and data
TEST(Motion, SwitchImuStateReadsCalibrationReplies)
{
    uint8_t reply[64] = {0x21};
    reply[13] = 0x90;
    reply[14] = SWITCH_SUBCMD_SPI_READ;
    reply[15] = 0x20;
    reply[16] = 0x60;
    reply[19] = 24;
    // accel coefficients of 0x2000 halve the counts per g
    for (int i = 0; i < 3; i++)
    {
        reply[20 + 6 + i * 2 + 1] = 0x20;
        reply[20 + 18 + i * 2] = 0x3B;
        reply[20 + 18 + i * 2 + 1] = 0x34;
    }
    SwitchImuState state;
    state.parse(reply, sizeof(reply));
    EXPECT_TRUE(state.has_calibration);
    EXPECT_FLOAT_EQ(state.calibration.accel_per_g[0], 0x2000 / 4.0f);

    // the user calibration only moves the gyro origin here
    uint8_t user[64] = {0x21};
    user[14] = SWITCH_SUBCMD_SPI_READ;
    user[15] = 0x26;
    user[16] = 0x80;
    user[19] = 26;
    user[20] = 0xB2;
    user[21] = 0xA1;
    user[22 + 12] = 50;
    state.parse(user, sizeof(user));
    EXPECT_FLOAT_EQ(state.calibration.gyro_origin[0], 50);
    EXPECT_FLOAT_EQ(state.calibration.accel_per_g[0], 0x2000 / 4.0f);
}

TEST(Motion, SwitchImuStateKeepsTheLastFullReport)
{
    SwitchProGamepad_Data_t report = {};
    report.reportID = SWITCH_PRO_CON_FULL_REPORT_ID;
    SwitchImuState state;
    // an IMU that isn't on yet sends nothing
    state.parse((const uint8_t *)&report, sizeof(report));
    EXPECT_FALSE(state.has_sample);
    report.imuData[0] = {0, 0, 4096, 1, 2, 3};
    report.imuData[2] = {9, 9, 9, 9, 9, 9};
    state.parse((const uint8_t *)&report, sizeof(report));
    EXPECT_TRUE(state.has_sample);
    EXPECT_EQ(state.sample.gyroZ, 3);
    // a subcommand reply has no motion
    uint8_t reply[64] = {0x21};
    state.parse(reply, sizeof(reply));
    EXPECT_EQ(state.sample.accZ, 4096);
}

// SDL reads both with no axis changes. The PS4 scale is the one the emulated controller states in
// feature report 0x03, 61/1024 °/s per count, and the PS5 one 2000/32768 °/s.
TEST(Motion, PlayStationUsesSDLAxes)
{
    PSImuSample ps4 = ps_imu_encode(lying_flat(), PS4_IMU_GYRO_PER_DPS, PS4_IMU_ACCEL_PER_G);
    EXPECT_EQ(ps4.accel[0], 0);
    EXPECT_EQ(ps4.accel[1], 8192);
    EXPECT_EQ(ps4.accel[2], 0);
    PSImuSample yaw = ps_imu_encode(turning(MotionGyroY, 100), PS4_IMU_GYRO_PER_DPS, PS4_IMU_ACCEL_PER_G);
    EXPECT_NEAR(yaw.gyro[1], 100 * 1024 / 61.0f, 1);
    PSImuSample ps5 = ps_imu_encode(turning(MotionGyroZ, -100), PS5_IMU_GYRO_PER_DPS, PS5_IMU_ACCEL_PER_G);
    EXPECT_NEAR(ps5.gyro[2], -1638.4, 1);
}

// The calibration report the emulated PS5 controller serves, read back the way SDL does: gyro
// sensitivity (speed plus + speed minus) * 1024 / (plus - minus), in 1/1024 °/s
TEST(Motion, PlayStationCalibrationReadsBackItsScale)
{
    uint8_t data[41] = {0x05};
    ps_imu_write_calibration(data, false, PS5_IMU_GYRO_PER_DPS, PS5_IMU_ACCEL_PER_G);
    auto load = [&](int i)
    { return (int16_t)(data[i] | (data[i + 1] << 8)); };
    float sensitivity = (load(19) + load(21)) * 1024.0f / (load(11) - load(13));
    EXPECT_NEAR(1024.0f / sensitivity, PS5_IMU_GYRO_PER_DPS, 0.01f);
    // SDL's sanity check
    EXPECT_LT(fabsf(1.0f - sensitivity / 64.0f), 0.5f);
    EXPECT_EQ(load(27), 8192);
    EXPECT_EQ(load(29), -8192);

    for (bool plus_then_minus : {false, true})
    {
        uint8_t report[41] = {0x05};
        ps_imu_write_calibration(report, plus_then_minus, PS4_IMU_GYRO_PER_DPS, PS4_IMU_ACCEL_PER_G);
        PSImuCalibration cal = ps_imu_parse_calibration(report, plus_then_minus);
        for (int i = 0; i < 3; i++)
        {
            EXPECT_NEAR(cal.gyro_per_dps[i], PS4_IMU_GYRO_PER_DPS, 0.01f);
            EXPECT_FLOAT_EQ(cal.accel_per_g[i], 8192);
            EXPECT_FLOAT_EQ(cal.gyro_bias[i], 0);
        }
    }
}

TEST(Motion, BrokenPlayStationCalibrationIsIgnored)
{
    uint8_t report[41] = {0x05};
    PSImuCalibration cal = ps_imu_parse_calibration(report, false);
    EXPECT_FLOAT_EQ(cal.gyro_per_dps[0], 16);
    EXPECT_FLOAT_EQ(cal.accel_per_g[0], 8192);
}

TEST(Motion, PlayStationCapabilityScale)
{
    // the emulated PS5 controller's capabilities, from the 0x28 byte on
    const uint8_t caps[] = {0x28, 0x03, 0x91, 0x00, 0x2C, 0x56, 0x01, 0x00, 0xD0, 0x07, 0x00, 0x80, 0x04, 0x00, 0x00, 0x80};
    PSImuCalibration cal;
    ps_imu_parse_capability_scale(caps, cal);
    EXPECT_FLOAT_EQ(cal.gyro_per_dps[0], PS5_IMU_GYRO_PER_DPS);
    EXPECT_FLOAT_EQ(cal.accel_per_g[2], PS5_IMU_ACCEL_PER_G);
}

// SDL_hidapi_ps3.c: x = (X - 511) / 113, y = -(Y - 511) / 113 and z = -(Z - 511) / 113, with Y the last
// value and Z the middle one. RPCS3's resting DualShock 3 has 399 (512 - 113) for Y.
TEST(Motion, DualShock3LyingFlat)
{
    PS3ImuSample sample = ds3_imu_encode(lying_flat());
    EXPECT_EQ(sample.accel[0], 512);
    EXPECT_EQ(sample.accel[1], 512);
    EXPECT_EQ(sample.accel[2], 399);
    EXPECT_EQ(sample.gyro, 512);
    // RPCS3: 123 counts per 90 °/s of yaw
    EXPECT_EQ(ds3_imu_encode(turning(MotionGyroY, 90)).gyro, 635);
}

TEST(Motion, PS3ThirdPartyHasGravityInTheMiddle)
{
    PS3ImuSample sample = ps3_third_party_imu_encode(lying_flat());
    EXPECT_EQ(sample.accel[0], 512);
    EXPECT_EQ(sample.accel[1], 399);
    EXPECT_EQ(sample.accel[2], 512);
}

TEST(Motion, PS3RoundTrips)
{
    MotionState motion;
    motion.write(MotionAccelX, 2048);
    motion.write(MotionAccelY, -1024);
    motion.write(MotionAccelZ, 512);
    motion.write(MotionGyroY, (int32_t)(45 * MOTION_GYRO_PER_DPS));
    float accel[3], gyro[3];
    ds3_imu_decode(ds3_imu_encode(motion), accel, gyro);
    for (int i = 0; i < 3; i++)
    {
        EXPECT_NEAR(accel[i], motion.accel_g(i), 0.01f);
    }
    EXPECT_NEAR(gyro[1], 45, 1);
    ps3_third_party_imu_decode(ps3_third_party_imu_encode(motion), accel, gyro);
    for (int i = 0; i < 3; i++)
    {
        EXPECT_NEAR(accel[i], motion.accel_g(i), 0.01f);
    }
}

TEST(Motion, PS3Clamps)
{
    MotionState motion;
    motion.write(MotionAccelX, (int32_t)(8 * MOTION_ACCEL_PER_G));
    EXPECT_EQ(ds3_imu_encode(motion).accel[0], 1023);
}

// SDL_hidapi_wii.c: x = -wii x, y = wii z, z = wii y at 100 counts per g around 0x200
TEST(Motion, WiimoteLyingFlat)
{
    uint16_t accel[3];
    wiimote_accel_encode(lying_flat(), accel);
    EXPECT_EQ(accel[0], 0x200);
    EXPECT_EQ(accel[1], 0x200);
    EXPECT_EQ(accel[2], 0x200 + 100);
    float back[3];
    wiimote_accel_decode(accel, back);
    EXPECT_FLOAT_EQ(back[1], 1.0f);
}

// SDL_hidapi_steam.c: acceleration x = x, y = z, z = -y at 2 g, gyro x = x, y = z, z = y at 2000 °/s
TEST(Motion, SteamControllerAxes)
{
    const int16_t accel[3] = {0, -16384, 16384};
    const int16_t gyro[3] = {0, 1638, -1638};
    float a[3], g[3];
    steam_imu_decode(accel, gyro, a, g);
    EXPECT_FLOAT_EQ(a[1], 1.0f);
    EXPECT_FLOAT_EQ(a[2], 1.0f);
    EXPECT_NEAR(g[1], -100, 0.1f);
    EXPECT_NEAR(g[2], 100, 0.1f);
}

TEST(Motion, HostAxesUseTheStandardScale)
{
    const float accel[3] = {0.5f, 1, -1};
    const float gyro[3] = {10, -20, 30};
    EXPECT_EQ(motion_host_axis(Gamepad_AccelX, accel, gyro), MOTION_ZERO + 2048);
    EXPECT_EQ(motion_host_axis(Gamepad_AccelZ, accel, gyro), MOTION_ZERO - 4096);
    EXPECT_EQ(motion_host_axis(Gamepad_GyroY, accel, gyro), motion_gyro_axis(-20));
    EXPECT_TRUE(is_motion_axis(Gamepad_GyroZ));
    EXPECT_FALSE(is_motion_axis(Gamepad_RightTrigger));
}

// Tilt turns the controller around Z: level is flat, a full tilt stands it on its side
TEST(Motion, TiltAtRestIsFlat)
{
    TiltMotion tilt;
    tilt.update(MOTION_ZERO, 0);
    MotionState motion;
    tilt.apply(motion);
    EXPECT_EQ(motion.axes[MotionAccelX], 0);
    EXPECT_EQ(motion.axes[MotionAccelY], 4096);
    EXPECT_EQ(motion.axes[MotionGyroZ], 0);
}

TEST(Motion, TiltSwitchTurnsAtTheMaximumRate)
{
    TiltMotion tilt;
    tilt.update(MOTION_ZERO, 0);
    // a tilt switch jumps straight to full tilt, which takes 90 / 720 s to reach
    tilt.update(UINT16_MAX, 10000);
    EXPECT_NEAR(tilt.angle(), 7.2f, 0.01f);
    MotionState motion;
    tilt.apply(motion);
    EXPECT_NEAR(motion.gyro_dps(2), MOTION_TILT_MAX_DPS, 1);
    EXPECT_GT(motion.axes[MotionAccelX], 0);
    for (uint64_t t = 20000; t <= 200000; t += 10000)
    {
        tilt.update(UINT16_MAX, t);
    }
    EXPECT_NEAR(tilt.angle(), 90.0f, 0.01f);
    MotionState still;
    tilt.apply(still);
    EXPECT_EQ(still.axes[MotionGyroZ], 0);
    EXPECT_EQ(still.axes[MotionAccelX], 4096);
    EXPECT_NEAR(still.axes[MotionAccelY], 0, 1);
}
} // namespace
