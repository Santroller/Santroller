#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "emulation/usb/usb_devices.h"
#include "protocols/motion.hpp"

#define SWITCH_PRO_CON_FULL_REPORT_ID 0x30
#define SWITCH_PRO_CON_SIMPLE_REPORT_ID 0x3F
#define SWITCH_PRO_CON_SUBCMD_REPORT_ID 0x01

// Match the factory and user stick calibration advertised by the Switch device.
inline constexpr uint16_t SWITCH_STICK_CENTER = 0x800;
inline constexpr uint16_t SWITCH_STICK_TRAVEL = 0x6A4;

inline uint16_t switch_stick(uint32_t value)
{
    int32_t offset = static_cast<int32_t>(value) - UINT16_MAX / 2;
    int32_t extent = offset < 0 ? UINT16_MAX / 2 : UINT16_MAX - UINT16_MAX / 2;
    return SWITCH_STICK_CENTER + offset * SWITCH_STICK_TRAVEL / extent;
}

// Output report 0x01: packet counter, 8 bytes of rumble (neutral here), then the subcommand and its arguments.
// Subcommand 0x03 sets the input report mode, 0x30 being the full report with 12 bit sticks. Over Bluetooth
// the controller starts out in the simple 0x3F mode until it gets this.
static const uint8_t SWITCH_SET_FULL_REPORT_MODE[] = {SWITCH_PRO_CON_SUBCMD_REPORT_ID, 0x00,
                                                      0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40,
                                                      0x03, 0x30};

// Subcommand 0x40 turns the IMU on, so full reports carry motion
static const uint8_t SWITCH_ENABLE_IMU[] = {SWITCH_PRO_CON_SUBCMD_REPORT_ID, 0x01,
                                            0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40,
                                            0x40, 0x01};

// Subcommand 0x10 reads SPI flash: a little endian address, then the length. These read the factory
// motion calibration at 0x6020, and the user calibration at 0x8026 (a 0xA1B2 magic, then the same layout).
#define SWITCH_SUBCMD_SPI_READ 0x10
#define SWITCH_SPI_IMU_FACTORY 0x6020
#define SWITCH_SPI_IMU_USER 0x8026
#define SWITCH_SPI_IMU_USER_MAGIC 0xA1B2
#define SWITCH_IMU_CALIBRATION_LEN 24
static const uint8_t SWITCH_READ_IMU_FACTORY_CALIBRATION[] = {SWITCH_PRO_CON_SUBCMD_REPORT_ID, 0x02,
                                                              0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40,
                                                              SWITCH_SUBCMD_SPI_READ, 0x20, 0x60, 0x00, 0x00, SWITCH_IMU_CALIBRATION_LEN};
static const uint8_t SWITCH_READ_IMU_USER_CALIBRATION[] = {SWITCH_PRO_CON_SUBCMD_REPORT_ID, 0x03,
                                                           0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40,
                                                           SWITCH_SUBCMD_SPI_READ, 0x26, 0x80, 0x00, 0x00, SWITCH_IMU_CALIBRATION_LEN + 2};

// ---------------------------------------------------------------------------
// Rumble and player LEDs, shared by the USB and Bluetooth hosts
// ---------------------------------------------------------------------------
#define SWITCH_RUMBLE_REPORT_ID 0x10
#define SWITCH_SUBCMD_SET_PLAYER_LIGHTS 0x30
#define SWITCH_RUMBLE_REPORT_LEN 10
#define SWITCH_PLAYER_LED_REPORT_LEN 12
#define SWITCH_OUTPUT_MAX_LEN SWITCH_PLAYER_LED_REPORT_LEN
// Same timings as SDL: rumble writes at most every 30ms, and a running rumble has to be refreshed
// every 50ms or the motors stop
#define SWITCH_OUTPUT_MIN_INTERVAL_MS 30
#define SWITCH_RUMBLE_REFRESH_MS 50

enum class SwitchOutputKind : uint8_t
{
    None,
    Rumble,
    PlayerLed
};

// Output bookkeeping, the transport specific hosts only do the actual send. build() fills in the next
// report (report id first) without changing anything, then sent() or failed() once the outcome is known.
struct SwitchOutput
{
    uint8_t strong = 0;
    uint8_t weak = 0;
    uint8_t player = 0;
    bool rumble_dirty = true;
    bool player_dirty = true;
    uint8_t counter = 0;
    uint32_t last_sent_ms = 0;

    void set_rumble(uint8_t left, uint8_t right)
    {
        if (left != strong || right != weak)
            rumble_dirty = true;
        strong = left;
        weak = right;
    }

    void set_player(uint8_t p)
    {
        if (p != player)
            player_dirty = true;
        player = p;
    }

    // Defined in the USB Switch host, which the Bluetooth host shares
    uint8_t build(uint32_t now, uint8_t *out, SwitchOutputKind &kind) const;

    void sent(SwitchOutputKind kind, uint32_t now)
    {
        if (kind == SwitchOutputKind::None)
            return;
        counter++;
        last_sent_ms = now;
        rumble_dirty = false;
        if (kind == SwitchOutputKind::PlayerLed)
            player_dirty = false;
    }

    void failed(SwitchOutputKind kind)
    {
        if (kind == SwitchOutputKind::None)
            return;
        rumble_dirty = true;
        if (kind == SwitchOutputKind::PlayerLed)
            player_dirty = true;
    }
};


typedef struct
{
    uint8_t connectionInfo : 4;
    uint8_t batteryLevel : 4;

    // byte 00
    uint8_t y : 1;
    uint8_t x : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t buttonRightSR : 1;
    uint8_t buttonRightSL : 1;
    uint8_t rightShoulder : 1;
    uint8_t rightTrigger : 1;

    uint8_t back : 1;            // minus
    uint8_t start : 1;           // plus
    uint8_t rightThumbClick : 1; // r3
    uint8_t leftThumbClick : 1;  // l3
    uint8_t guide : 1;           // home
    uint8_t capture : 1;
    uint8_t dummy2 : 1;
    uint8_t chargingGrip : 1;

    uint8_t dpadDown : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;

    uint8_t buttonLeftSR : 1;
    uint8_t buttonLeftSL : 1;
    uint8_t leftShoulder : 1;
    uint8_t leftTrigger : 1;

    uint16_t leftStickX : 12;
    uint16_t leftStickY : 12;
    uint16_t rightStickX : 12;
    uint16_t rightStickY : 12;
} __attribute__((packed)) SwitchInputReport;

typedef struct
{
    uint8_t reportID;
    uint8_t timestamp;
    SwitchInputReport inputs;
    uint8_t rumbleReport;
    // Three samples 5ms apart, the newest first
    SwitchImuSample imuData[3];
    uint8_t padding[15];
} __attribute__((packed)) SwitchProGamepad_Data_t;

// Motion from a real controller. Subcommand replies (0x21) carry the reply where full reports have
// motion, so the last full report's sample is kept, along with the calibration read from SPI flash.
struct SwitchImuState
{
    SwitchImuSample sample = {};
    // Like SDL, a sample with no acceleration at all means the IMU isn't on yet
    bool has_sample = false;
    SwitchImuCalibration calibration;
    // The factory calibration, as the user calibration only replaces its origins
    uint8_t calibration_data[SWITCH_IMU_CALIBRATION_LEN] = {};
    bool has_calibration = false;

    // Called with every input report, report ID included
    void parse(const uint8_t *buf, uint16_t len)
    {
        if (buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID && len >= offsetof(SwitchProGamepad_Data_t, padding))
        {
            const SwitchImuSample &newest = ((const SwitchProGamepad_Data_t *)buf)->imuData[0];
            if (newest.accX || newest.accY || newest.accZ)
            {
                sample = newest;
                has_sample = true;
            }
            return;
        }
        // 0x21: the inputs, an ack byte, the subcommand, then its reply. A SPI read replies with the
        // address and length it was asked for, then the data.
        if (buf[0] != 0x21 || len < 20 || buf[14] != SWITCH_SUBCMD_SPI_READ)
        {
            return;
        }
        uint32_t address = buf[15] | (buf[16] << 8) | (buf[17] << 16) | ((uint32_t)buf[18] << 24);
        uint8_t size = buf[19];
        const uint8_t *data = buf + 20;
        if (address == SWITCH_SPI_IMU_FACTORY && size >= SWITCH_IMU_CALIBRATION_LEN && len >= 20 + SWITCH_IMU_CALIBRATION_LEN)
        {
            memcpy(calibration_data, data, SWITCH_IMU_CALIBRATION_LEN);
            has_calibration = true;
            calibration = switch_imu_parse_calibration(calibration_data);
        }
        else if (address == SWITCH_SPI_IMU_USER && has_calibration && size >= SWITCH_IMU_CALIBRATION_LEN + 2 &&
                 len >= 22 + SWITCH_IMU_CALIBRATION_LEN && (data[0] | (data[1] << 8)) == SWITCH_SPI_IMU_USER_MAGIC)
        {
            uint8_t combined[SWITCH_IMU_CALIBRATION_LEN];
            memcpy(combined, calibration_data, sizeof(combined));
            // accel origins, then gyro origins
            memcpy(combined, data + 2, 6);
            memcpy(combined + 12, data + 2 + 12, 6);
            calibration = switch_imu_parse_calibration(combined);
        }
    }
};

typedef struct
{
    uint8_t reportId; // 0x0A
    uint8_t packetId; // counts up for each packet
    uint8_t status;   // 0x20
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t y : 1;
    uint8_t x : 1;
    uint8_t r2 : 1;
    uint8_t rightShoulder : 1;
    uint8_t start : 1;
    uint8_t rightThumbClick : 1;

    uint8_t dpadDown : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadUp : 1;
    uint8_t l2 : 1;
    uint8_t leftShoulder : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;

    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t gl : 1;
    uint8_t gr : 1;
    uint8_t c : 1;
    uint8_t : 3;

    uint16_t leftStickX : 12;
    uint16_t leftStickY : 12;
    uint16_t rightStickX : 12;
    uint16_t rightStickY : 12;
    uint8_t vibrationCode;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t imuLength;
    uint8_t motionBuffer[10];
} __attribute__((packed)) Switch2ProGamepadGC_Data_t;

typedef struct
{
    uint8_t reportId; // 0x09
    uint8_t packetId; // counts up for each packet
    uint8_t status;   // 0x20
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t y : 1;
    uint8_t x : 1;
    uint8_t rightShoulder : 1;
    uint8_t r2 : 1;
    uint8_t start : 1;
    uint8_t rightThumbClick : 1;

    uint8_t dpadDown : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadUp : 1;
    uint8_t leftShoulder : 1;
    uint8_t l2 : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;

    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t gl : 1;
    uint8_t gr : 1;
    uint8_t c : 1;
    uint8_t : 3;

    uint16_t leftStickX : 12;
    uint16_t leftStickY : 12;
    uint16_t rightStickX : 12;
    uint16_t rightStickY : 12;
    uint8_t vibrationCode;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t imuLength;
    uint8_t motionBuffer[10];
} __attribute__((packed)) Switch2ProGamepad_Data_t;

typedef struct
{
    // Button bits
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t x : 1;

    uint8_t leftShoulder : 1;  // l1
    uint8_t rightShoulder : 1; // r1
    uint8_t l2 : 1;            // l2
    uint8_t r2 : 1;            // r2

    uint8_t back : 1; // select
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;  // l3
    uint8_t rightThumbClick : 1; // r3

    uint8_t guide : 1;   // ps
    uint8_t capture : 1; // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    uint8_t unused2[12];
    uint16_t unused3[4];

} __attribute__((packed)) SwitchGamepad_Data_t;

typedef struct
{
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t x : 1;
    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t l2 : 1; // l2 rimLeft
    uint8_t r2 : 1; // r2 rimRight

    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;  // l3 centerLeft
    uint8_t rightThumbClick : 1; // r3 centerRight
    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t : 2;

    uint8_t dpad;
    uint8_t sticks[4]; // always 0x80
    uint8_t : 8;

} __attribute__((packed)) SwitchTaiko_Data_t;