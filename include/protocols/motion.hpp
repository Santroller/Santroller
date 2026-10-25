#pragma once
#include <stdint.h>
#include <math.h>
#include "input_enums.pb.h"

// Every motion source and output meets in one fixed scale, so any source can drive any output.
// Motion travels through mappings like any other axis, with MOTION_ZERO (the centre of a stick)
// as no acceleration or no rotation.
//
// The axes are SDL's: holding the controller in front of you, X points right, Y points up out of
// its face and Z points towards you. Acceleration includes gravity, so a controller lying flat
// reads +1 g on Y. Gyro is the rate of turn around each axis, positive counter-clockwise when
// seen from the positive end of that axis (X pitch, Y yaw, Z roll).
inline constexpr int32_t MOTION_ZERO = UINT16_MAX / 2;
// ±8 g, as a Switch Pro controller reports it
inline constexpr float MOTION_ACCEL_PER_G = 4096.0f;
// ±2000 °/s, the full range of the MPU-6050, DualShock 4, DualSense and Switch Pro gyros
inline constexpr float MOTION_GYRO_PER_DPS = 32768.0f / 2000.0f;

enum MotionAxis : uint8_t
{
    MotionAccelX,
    MotionAccelY,
    MotionAccelZ,
    MotionGyroX,
    MotionGyroY,
    MotionGyroZ,
    MotionAxisCount
};

// Motion for one report, collected from the mappings. Axes nothing was mapped to keep the controller
// lying still and flat, and set says which ones a mapping wrote.
struct MotionState
{
    int32_t axes[MotionAxisCount] = {0, (int32_t)MOTION_ACCEL_PER_G, 0, 0, 0, 0};
    uint8_t set = 0;
    // Set by outputs that read guitar tilt from motion, as other consoles have a tilt value of their own
    bool tilt_as_motion = false;

    void write(MotionAxis axis, int32_t value)
    {
        axes[axis] = value;
        set |= 1 << axis;
    }
    bool has(MotionAxis axis) const { return set & (1 << axis); }
    float accel_g(int axis) const { return axes[MotionAccelX + axis] / MOTION_ACCEL_PER_G; }
    float gyro_dps(int axis) const { return axes[MotionGyroX + axis] / MOTION_GYRO_PER_DPS; }
};

// Between an axis value and a signed value in the standard scale
inline int32_t motion_from_axis(uint32_t value)
{
    return (int32_t)value - MOTION_ZERO;
}
inline uint16_t motion_to_axis(float value)
{
    float v = roundf(value) + MOTION_ZERO;
    if (v < 0)
        return 0;
    if (v > UINT16_MAX)
        return UINT16_MAX;
    return (uint16_t)v;
}
inline uint16_t motion_accel_axis(float g)
{
    return motion_to_axis(g * MOTION_ACCEL_PER_G);
}
inline uint16_t motion_gyro_axis(float dps)
{
    return motion_to_axis(dps * MOTION_GYRO_PER_DPS);
}

// For hosts: a Gamepad_Accel / Gamepad_Gyro output's value, from a reading in g and °/s
inline bool is_motion_axis(GamepadAxisType axis)
{
    return axis >= Gamepad_AccelX && axis <= Gamepad_GyroZ;
}
inline uint16_t motion_host_axis(GamepadAxisType axis, const float accel_g[3], const float gyro_dps[3])
{
    switch (axis)
    {
    case Gamepad_AccelX:
        return motion_accel_axis(accel_g[0]);
    case Gamepad_AccelY:
        return motion_accel_axis(accel_g[1]);
    case Gamepad_AccelZ:
        return motion_accel_axis(accel_g[2]);
    case Gamepad_GyroX:
        return motion_gyro_axis(gyro_dps[0]);
    case Gamepad_GyroY:
        return motion_gyro_axis(gyro_dps[1]);
    case Gamepad_GyroZ:
        return motion_gyro_axis(gyro_dps[2]);
    default:
        return MOTION_ZERO;
    }
}

inline int16_t motion_clamp16(float value)
{
    value = roundf(value);
    if (value < INT16_MIN)
        return INT16_MIN;
    if (value > INT16_MAX)
        return INT16_MAX;
    return (int16_t)value;
}

inline uint16_t motion_clamp10(float value)
{
    value = roundf(value);
    if (value < 0)
        return 0;
    if (value > 1023)
        return 1023;
    return (uint16_t)value;
}

// ---------------------------------------------------------------------------
// Switch Pro controller
// ---------------------------------------------------------------------------
// The scale comes from the motion calibration the emulated controller serves from SPI flash at 0x6020.
// With every origin at 0, acceleration is 4 / 0x4000 g per count and gyro 936 / 0x343B °/s per count.
// Its axes differ from SDL's, SDL_hidapi_switch.c turns them into SDL's with
// x = -switch y, y = switch z and z = -switch x.
inline constexpr float SWITCH_IMU_ACCEL_PER_G = 0x4000 / 4.0f;
inline constexpr float SWITCH_IMU_GYRO_PER_DPS = 0x343B / 936.0f;

struct SwitchImuSample
{
    int16_t accX, accY, accZ;
    int16_t gyroX, gyroY, gyroZ;
} __attribute__((packed));

inline SwitchImuSample switch_imu_encode(const MotionState &motion)
{
    return {
        motion_clamp16(-motion.accel_g(2) * SWITCH_IMU_ACCEL_PER_G),
        motion_clamp16(-motion.accel_g(0) * SWITCH_IMU_ACCEL_PER_G),
        motion_clamp16(motion.accel_g(1) * SWITCH_IMU_ACCEL_PER_G),
        motion_clamp16(-motion.gyro_dps(2) * SWITCH_IMU_GYRO_PER_DPS),
        motion_clamp16(-motion.gyro_dps(0) * SWITCH_IMU_GYRO_PER_DPS),
        motion_clamp16(motion.gyro_dps(1) * SWITCH_IMU_GYRO_PER_DPS),
    };
}

// A real controller's calibration, used to read its samples
struct SwitchImuCalibration
{
    float accel_origin[3] = {0, 0, 0};
    float accel_per_g[3] = {SWITCH_IMU_ACCEL_PER_G, SWITCH_IMU_ACCEL_PER_G, SWITCH_IMU_ACCEL_PER_G};
    float gyro_origin[3] = {0, 0, 0};
    float gyro_per_dps[3] = {SWITCH_IMU_GYRO_PER_DPS, SWITCH_IMU_GYRO_PER_DPS, SWITCH_IMU_GYRO_PER_DPS};
};

// Parses the 24 bytes of factory motion calibration at 0x6020, or user calibration at 0x8028.
// Each coefficient is the reading for 4 g (accel) or 936 °/s (gyro) above its origin.
inline SwitchImuCalibration switch_imu_parse_calibration(const uint8_t *data)
{
    SwitchImuCalibration cal;
    auto load = [&](int i)
    { return (float)(int16_t)(data[i * 2] | (data[i * 2 + 1] << 8)); };
    for (int i = 0; i < 3; i++)
    {
        float accel_origin = load(i);
        float accel_coeff = load(3 + i);
        float gyro_origin = load(6 + i);
        float gyro_coeff = load(9 + i);
        if (accel_coeff != accel_origin)
        {
            cal.accel_per_g[i] = (accel_coeff - accel_origin) / 4.0f;
        }
        if (gyro_coeff != gyro_origin)
        {
            cal.gyro_origin[i] = gyro_origin;
            cal.gyro_per_dps[i] = (gyro_coeff - gyro_origin) / 936.0f;
        }
    }
    return cal;
}

// Like SDL, acceleration ignores its origin, which only sets the scale
inline void switch_imu_decode(const SwitchImuSample &sample, const SwitchImuCalibration &cal, float accel_g[3], float gyro_dps[3])
{
    float accel[3] = {sample.accX / cal.accel_per_g[0], sample.accY / cal.accel_per_g[1], sample.accZ / cal.accel_per_g[2]};
    float gyro[3] = {(sample.gyroX - cal.gyro_origin[0]) / cal.gyro_per_dps[0],
                     (sample.gyroY - cal.gyro_origin[1]) / cal.gyro_per_dps[1],
                     (sample.gyroZ - cal.gyro_origin[2]) / cal.gyro_per_dps[2]};
    accel_g[0] = -accel[1];
    accel_g[1] = accel[2];
    accel_g[2] = -accel[0];
    gyro_dps[0] = -gyro[1];
    gyro_dps[1] = gyro[2];
    gyro_dps[2] = -gyro[0];
}

// ---------------------------------------------------------------------------
// DualShock 4 and DualSense
// ---------------------------------------------------------------------------
// Both use SDL's axes. A DualShock 4 is 1/16 °/s and 1/8192 g per count unless its calibration
// says otherwise. Third party controllers state their own scale in feature report 0x03: the emulated
// PS4 controller says 61/1024 °/s, and the emulated PS5 controller 2000/32768 °/s and 4/32768 g.
struct PSImuSample
{
    int16_t gyro[3];
    int16_t accel[3];
} __attribute__((packed));

inline constexpr float PS4_IMU_GYRO_PER_DPS = 1024.0f / 61.0f;
inline constexpr float PS4_IMU_ACCEL_PER_G = 8192.0f;
inline constexpr float PS5_IMU_GYRO_PER_DPS = 32768.0f / 2000.0f;
inline constexpr float PS5_IMU_ACCEL_PER_G = 32768.0f / 4.0f;

inline PSImuSample ps_imu_encode(const MotionState &motion, float gyro_per_dps, float accel_per_g)
{
    PSImuSample sample;
    for (int i = 0; i < 3; i++)
    {
        sample.gyro[i] = motion_clamp16(motion.gyro_dps(i) * gyro_per_dps);
        sample.accel[i] = motion_clamp16(motion.accel_g(i) * accel_per_g);
    }
    return sample;
}

// A real controller's calibration, used to read its samples
struct PSImuCalibration
{
    float gyro_bias[3] = {0, 0, 0};
    float gyro_per_dps[3] = {16, 16, 16};
    float accel_bias[3] = {0, 0, 0};
    float accel_per_g[3] = {8192, 8192, 8192};
};

// Parses calibration feature report 0x02 (DualShock 4 over USB) or 0x05 (DualShock 4 over Bluetooth
// and DualSense), report ID included. The DualShock 4 over Bluetooth lists every plus reading before
// the minus ones. A calibration that looks broken is ignored, as SDL does.
inline PSImuCalibration ps_imu_parse_calibration(const uint8_t *data, bool plus_then_minus)
{
    auto load = [&](int i)
    { return (float)(int16_t)(data[i] | (data[i + 1] << 8)); };
    PSImuCalibration cal;
    PSImuCalibration nominal;
    float plus[3], minus[3];
    for (int i = 0; i < 3; i++)
    {
        cal.gyro_bias[i] = load(1 + i * 2);
        plus[i] = load(plus_then_minus ? 7 + i * 2 : 7 + i * 4);
        minus[i] = load(plus_then_minus ? 13 + i * 2 : 9 + i * 4);
    }
    float speed = load(19) + load(21);
    for (int i = 0; i < 3; i++)
    {
        float span = fabsf(plus[i] - cal.gyro_bias[i]) + fabsf(minus[i] - cal.gyro_bias[i]);
        if (span == 0 || speed == 0)
            return nominal;
        cal.gyro_per_dps[i] = span / speed;
        float accel_plus = load(23 + i * 4);
        float accel_minus = load(25 + i * 4);
        if (accel_plus == accel_minus)
            return nominal;
        cal.accel_bias[i] = accel_plus - (accel_plus - accel_minus) / 2;
        cal.accel_per_g[i] = (accel_plus - accel_minus) / 2;
    }
    for (int i = 0; i < 3; i++)
    {
        if (fabsf(cal.gyro_bias[i]) > 1024 || fabsf(cal.accel_bias[i]) > 1024 ||
            fabsf(1 - cal.gyro_per_dps[i] / 16) > 0.5f || fabsf(1 - cal.accel_per_g[i] / 8192) > 0.5f)
        {
            return nominal;
        }
    }
    return cal;
}

// Third party controllers state their motion scale in capabilities feature report 0x03, as a gyro
// numerator / denominator in °/s and an accel one in g. caps points at the byte after the report ID
// and its 0x21 length byte (0x27 on a PS4, 0x28 on a PS5).
inline void ps_imu_parse_capability_scale(const uint8_t *caps, PSImuCalibration &cal)
{
    auto load = [&](int i)
    { return (float)(uint16_t)(caps[i] | (caps[i + 1] << 8)); };
    float gyro_num = load(8), gyro_den = load(10), accel_num = load(12), accel_den = load(14);
    for (int i = 0; i < 3; i++)
    {
        if (gyro_num && gyro_den)
            cal.gyro_per_dps[i] = gyro_den / gyro_num;
        if (accel_num && accel_den)
            cal.accel_per_g[i] = accel_den / accel_num;
    }
}

inline void ps_imu_decode(const PSImuSample &sample, const PSImuCalibration &cal, float accel_g[3], float gyro_dps[3])
{
    for (int i = 0; i < 3; i++)
    {
        gyro_dps[i] = (sample.gyro[i] - cal.gyro_bias[i]) / cal.gyro_per_dps[i];
        accel_g[i] = (sample.accel[i] - cal.accel_bias[i]) / cal.accel_per_g[i];
    }
}

// The calibration the emulated controllers hand out, so their samples read back unchanged: no bias,
// ±8640 counts for ±540 °/s and ±8192 counts for ±1 g, the same layout as a real controller's. Real
// controllers' plus readings are around 8640 for 540 °/s too, which keeps it within the range SDL
// accepts.
inline void ps_imu_write_calibration(uint8_t *data, bool plus_then_minus, float gyro_per_dps, float accel_per_g)
{
    auto store = [&](int i, int16_t v)
    {
        data[i] = v & 0xFF;
        data[i + 1] = (uint16_t)v >> 8;
    };
    const int16_t speed = 540;
    const int16_t gyro = (int16_t)roundf(speed * gyro_per_dps);
    const int16_t accel = (int16_t)roundf(accel_per_g);
    for (int i = 0; i < 3; i++)
    {
        store(1 + i * 2, 0);
        store(plus_then_minus ? 7 + i * 2 : 7 + i * 4, gyro);
        store(plus_then_minus ? 13 + i * 2 : 9 + i * 4, -gyro);
        store(23 + i * 4, accel);
        store(25 + i * 4, -accel);
    }
    store(19, speed);
    store(21, speed);
}

// ---------------------------------------------------------------------------
// Sixaxis (DualShock 3 and PS3 third party controllers)
// ---------------------------------------------------------------------------
// 10 bit values centred on 512, at about 113 counts per g (hid-sony's SIXAXIS_ACC_RES_PER_G).
// The single gyro is yaw, at 123 counts per 90 °/s, the scale RPCS3 uses as nothing official is known.
// The PS3 calls the three accelerometer values X, Z and Y in report order. SDL_hidapi_ps3.c reads a
// DualShock 3 as x = X, y = -Y and z = -Z, and a third party controller as x = -X, y = -Z, z = -Y
// in report order, so the axis taking gravity is the last on a DualShock 3 and the middle one otherwise.
inline constexpr float PS3_IMU_CENTER = 512.0f;
inline constexpr float PS3_IMU_ACCEL_PER_G = 113.0f;
inline constexpr float PS3_IMU_GYRO_PER_DPS = 123.0f / 90.0f;

// The four values in report order, before any byte swapping
struct PS3ImuSample
{
    uint16_t accel[3];
    uint16_t gyro;
};

inline PS3ImuSample ds3_imu_encode(const MotionState &motion)
{
    return {{motion_clamp10(PS3_IMU_CENTER + motion.accel_g(0) * PS3_IMU_ACCEL_PER_G),
             motion_clamp10(PS3_IMU_CENTER - motion.accel_g(2) * PS3_IMU_ACCEL_PER_G),
             motion_clamp10(PS3_IMU_CENTER - motion.accel_g(1) * PS3_IMU_ACCEL_PER_G)},
            motion_clamp10(PS3_IMU_CENTER + motion.gyro_dps(1) * PS3_IMU_GYRO_PER_DPS)};
}

inline PS3ImuSample ps3_third_party_imu_encode(const MotionState &motion)
{
    return {{motion_clamp10(PS3_IMU_CENTER - motion.accel_g(0) * PS3_IMU_ACCEL_PER_G),
             motion_clamp10(PS3_IMU_CENTER - motion.accel_g(1) * PS3_IMU_ACCEL_PER_G),
             motion_clamp10(PS3_IMU_CENTER - motion.accel_g(2) * PS3_IMU_ACCEL_PER_G)},
            motion_clamp10(PS3_IMU_CENTER + motion.gyro_dps(1) * PS3_IMU_GYRO_PER_DPS)};
}

inline void ds3_imu_decode(const PS3ImuSample &sample, float accel_g[3], float gyro_dps[3])
{
    accel_g[0] = (sample.accel[0] - PS3_IMU_CENTER) / PS3_IMU_ACCEL_PER_G;
    accel_g[1] = -(sample.accel[2] - PS3_IMU_CENTER) / PS3_IMU_ACCEL_PER_G;
    accel_g[2] = -(sample.accel[1] - PS3_IMU_CENTER) / PS3_IMU_ACCEL_PER_G;
    gyro_dps[0] = 0;
    gyro_dps[1] = (sample.gyro - PS3_IMU_CENTER) / PS3_IMU_GYRO_PER_DPS;
    gyro_dps[2] = 0;
}

inline void ps3_third_party_imu_decode(const PS3ImuSample &sample, float accel_g[3], float gyro_dps[3])
{
    for (int i = 0; i < 3; i++)
    {
        accel_g[i] = -(sample.accel[i] - PS3_IMU_CENTER) / PS3_IMU_ACCEL_PER_G;
    }
    gyro_dps[0] = 0;
    gyro_dps[1] = (sample.gyro - PS3_IMU_CENTER) / PS3_IMU_GYRO_PER_DPS;
    gyro_dps[2] = 0;
}

// ---------------------------------------------------------------------------
// Wii remote
// ---------------------------------------------------------------------------
// 10 bit values centred on 0x200 at about 100 counts per g. SDL_hidapi_wii.c reads them as
// x = -wii x, y = wii z and z = wii y.
inline constexpr float WIIMOTE_IMU_CENTER = 0x200;
inline constexpr float WIIMOTE_IMU_ACCEL_PER_G = 100.0f;

inline void wiimote_accel_encode(const MotionState &motion, uint16_t out[3])
{
    out[0] = motion_clamp10(WIIMOTE_IMU_CENTER - motion.accel_g(0) * WIIMOTE_IMU_ACCEL_PER_G);
    out[1] = motion_clamp10(WIIMOTE_IMU_CENTER + motion.accel_g(2) * WIIMOTE_IMU_ACCEL_PER_G);
    out[2] = motion_clamp10(WIIMOTE_IMU_CENTER + motion.accel_g(1) * WIIMOTE_IMU_ACCEL_PER_G);
}

inline void wiimote_accel_decode(const uint16_t in[3], float accel_g[3])
{
    accel_g[0] = -(in[0] - WIIMOTE_IMU_CENTER) / WIIMOTE_IMU_ACCEL_PER_G;
    accel_g[1] = (in[2] - WIIMOTE_IMU_CENTER) / WIIMOTE_IMU_ACCEL_PER_G;
    accel_g[2] = (in[1] - WIIMOTE_IMU_CENTER) / WIIMOTE_IMU_ACCEL_PER_G;
}

// ---------------------------------------------------------------------------
// Steam Controller
// ---------------------------------------------------------------------------
// Raw IMU mode sends ±2 g and ±2000 °/s over the full int16 range. SDL_hidapi_steam.c reads
// acceleration as x = x, y = z, z = -y and gyro as x = x, y = z, z = y.
inline constexpr float STEAM_IMU_ACCEL_PER_G = 32768.0f / 2.0f;
inline constexpr float STEAM_IMU_GYRO_PER_DPS = 32768.0f / 2000.0f;

inline void steam_imu_decode(const int16_t accel[3], const int16_t gyro[3], float accel_g[3], float gyro_dps[3])
{
    accel_g[0] = accel[0] / STEAM_IMU_ACCEL_PER_G;
    accel_g[1] = accel[2] / STEAM_IMU_ACCEL_PER_G;
    accel_g[2] = -accel[1] / STEAM_IMU_ACCEL_PER_G;
    gyro_dps[0] = gyro[0] / STEAM_IMU_GYRO_PER_DPS;
    gyro_dps[1] = gyro[2] / STEAM_IMU_GYRO_PER_DPS;
    gyro_dps[2] = gyro[1] / STEAM_IMU_GYRO_PER_DPS;
}

// The 2026 Steam Controller (Triton) has the same scale, but SDL_hidapi_steam_triton.c reads both as
// x = x, y = z, z = -y
inline void steam_triton_imu_decode(const int16_t accel[3], const int16_t gyro[3], float accel_g[3], float gyro_dps[3])
{
    accel_g[0] = accel[0] / STEAM_IMU_ACCEL_PER_G;
    accel_g[1] = accel[2] / STEAM_IMU_ACCEL_PER_G;
    accel_g[2] = -accel[1] / STEAM_IMU_ACCEL_PER_G;
    gyro_dps[0] = gyro[0] / STEAM_IMU_GYRO_PER_DPS;
    gyro_dps[1] = gyro[2] / STEAM_IMU_GYRO_PER_DPS;
    gyro_dps[2] = -gyro[1] / STEAM_IMU_GYRO_PER_DPS;
}

// ---------------------------------------------------------------------------
// Switch 2 controllers
// ---------------------------------------------------------------------------
// ±8 g, and a gyro whose full scale SDL_hidapi_switch2.c puts at 34.8 rad/s (40 on firmware whose
// motion timestamps aren't in µs). SDL reads both as x = x, y = z, z = -y, taking its gyro bias (from
// flash, in rad/s) off afterwards.
inline constexpr float SWITCH2_IMU_ACCEL_PER_G = 32767.0f / 8.0f;
inline constexpr float SWITCH2_IMU_GYRO_PER_DPS = 32767.0f / (34.8f * 180.0f / (float)M_PI);

inline void switch2_imu_decode(const int16_t accel[3], const int16_t gyro[3], const float gyro_bias_rad[3], float accel_g[3], float gyro_dps[3])
{
    const float rad_to_deg = 180.0f / (float)M_PI;
    accel_g[0] = accel[0] / SWITCH2_IMU_ACCEL_PER_G;
    accel_g[1] = accel[2] / SWITCH2_IMU_ACCEL_PER_G;
    accel_g[2] = -accel[1] / SWITCH2_IMU_ACCEL_PER_G;
    gyro_dps[0] = gyro[0] / SWITCH2_IMU_GYRO_PER_DPS - gyro_bias_rad[0] * rad_to_deg;
    gyro_dps[1] = gyro[2] / SWITCH2_IMU_GYRO_PER_DPS - gyro_bias_rad[2] * rad_to_deg;
    gyro_dps[2] = -gyro[1] / SWITCH2_IMU_GYRO_PER_DPS + gyro_bias_rad[1] * rad_to_deg;
}

// ---------------------------------------------------------------------------
// Tilt as motion
// ---------------------------------------------------------------------------
// For outputs that read tilt from motion, such as a guitar on a Switch. A tilt axis turns the
// controller around Z from lying flat, with the centre as level and either end as 90° each way.
// The angle follows the tilt at up to MOTION_TILT_MAX_DPS, so a digital tilt switch turns into a
// quick but believable movement instead of an instant jump, and gyro reports that movement.
inline constexpr float MOTION_TILT_MAX_DEGREES = 90.0f;
inline constexpr float MOTION_TILT_MAX_DPS = 720.0f;

class TiltMotion
{
public:
    void update(uint32_t tilt_axis, uint64_t now_us)
    {
        float target = motion_from_axis(tilt_axis) / (float)(UINT16_MAX - MOTION_ZERO) * MOTION_TILT_MAX_DEGREES;
        if (!m_started)
        {
            m_started = true;
            m_angle = target;
            m_last_us = now_us;
            m_rate = 0;
            return;
        }
        float dt = (now_us - m_last_us) / 1000000.0f;
        if (dt <= 0)
            return;
        m_last_us = now_us;
        float step = target - m_angle;
        float max_step = MOTION_TILT_MAX_DPS * dt;
        if (step > max_step)
            step = max_step;
        if (step < -max_step)
            step = -max_step;
        m_angle += step;
        m_rate = step / dt;
    }
    void apply(MotionState &motion) const
    {
        float radians = m_angle * (float)M_PI / 180.0f;
        // Turning the controller counter-clockwise around Z swings gravity towards its +X side
        motion.write(MotionAccelX, motion_clamp16(sinf(radians) * MOTION_ACCEL_PER_G));
        motion.write(MotionAccelY, motion_clamp16(cosf(radians) * MOTION_ACCEL_PER_G));
        motion.write(MotionAccelZ, 0);
        motion.write(MotionGyroX, 0);
        motion.write(MotionGyroY, 0);
        motion.write(MotionGyroZ, motion_clamp16(m_rate * MOTION_GYRO_PER_DPS));
    }
    float angle() const { return m_angle; }

private:
    bool m_started = false;
    float m_angle = 0;
    float m_rate = 0;
    uint64_t m_last_us = 0;
};
