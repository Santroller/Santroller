#pragma once
#include <stdint.h>
#include <stddef.h>
#include "protocols/motion.hpp"

static const uint8_t ps4_colors[4][3] = {
    {0x00, 0x00, 0x40}, /* Blue */
    {0x40, 0x00, 0x00}, /* Red */
    {0x00, 0x40, 0x00}, /* Green */
    {0x20, 0x00, 0x20}  /* Pink */
};

typedef struct {
    uint8_t report_id; /* 0x5 */
    uint8_t valid_flag0;
    uint8_t valid_flag1;

    uint8_t reserved1;

    uint8_t motor_right;
    uint8_t motor_left;

    uint8_t lightbar_red;
    uint8_t lightbar_green;
    uint8_t lightbar_blue;
    uint8_t lightbar_blink_on;
    uint8_t lightbar_blink_off;
    uint8_t reserved[21];
} __attribute__((packed)) ps4_output_report;

typedef struct {
    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t dpad : 4;
    // 14 bits for buttons.
    uint8_t x : 1;  // square
    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t y : 1;  // triangle

    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1
    uint8_t l2 : 1;             // l2
    uint8_t r2 : 1;             // r2

    uint8_t back : 1;             // share
    uint8_t start : 1;            // options
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // touchpad click
    // 6 bit report counter.
    uint8_t reportCounter : 6;

    uint8_t leftTrigger : 8;
    uint8_t rightTrigger : 8;

    // in 16/3 µs units
    uint16_t sensorTimestamp;
    uint8_t sensorTemperature;
    PSImuSample imu;
    uint8_t mystery[10];
    uint8_t touchpadData[8];
    uint8_t mystery_2[21];
} __attribute__((packed)) PS4Dpad_Data_t;

typedef struct {
    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    // 14 bits for buttons.
    uint8_t x : 1;  // square
    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t y : 1;  // triangle

    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1
    uint8_t l2 : 1;             // l2
    uint8_t r2 : 1;             // r2

    uint8_t back : 1;             // share
    uint8_t start : 1;            // options
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // touchpad click
    // 6 bit report counter.
    uint8_t reportCounter : 6;

    uint8_t leftTrigger : 8;
    uint8_t rightTrigger : 8;

    // in 16/3 µs units
    uint16_t sensorTimestamp;
    uint8_t sensorTemperature;
    PSImuSample imu;
    uint8_t mystery[10];
    uint8_t touchpadData[8];
    uint8_t mystery_2[21];
} __attribute__((packed)) PS4Gamepad_Data_t;


typedef struct
{

    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t unused[2];

    // To make things easier we use bitfields here and them map to dpad later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    
    uint8_t x : 1;  // blue
    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t y : 1;  // yellow

    uint8_t leftShoulder : 1;   // orange
    uint8_t rightShoulder: 1;  
    uint8_t l2: 1;                // l2
    uint8_t r2: 1;                // r2

    uint8_t back : 1;             // share
    uint8_t start : 1;           // pause, options
    uint8_t solo : 1;            // l3, solo
    uint8_t : 1;            // r3, p1 on riffmaster

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // touchpad click
    uint8_t : 6;

    uint8_t leftTrigger : 8;
    uint8_t rightTrigger : 8;
    uint8_t unused2[20];

    uint8_t powerLevel : 4;
    uint8_t : 4;

    uint8_t unused3[12];

    uint8_t pickup;
    uint8_t whammy;
    uint8_t tilt;

    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t : 3;

    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;
    uint8_t soloOrange : 1;
    uint8_t : 3;

    uint8_t unused4[16];
} __attribute__((packed)) PS4RockBandGuitar_Data_t;

typedef struct
{

    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t unused[2];

    //     0
    //   7   1
    // 6   8   2
    //   5   3
    //     4
    // To make things easier we use bitfields here and them map to dpad later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    
    uint8_t x : 1;  // blue
    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t y : 1;  // yellow

    uint8_t kick1 : 1;   // kick1
    uint8_t kick2 : 1;  // kick2
    uint8_t l2: 1;
    uint8_t r2: 1;
    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t : 2;

    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t : 6;

    uint8_t leftTrigger : 8;
    uint8_t rightTrigger : 8;
    uint8_t unused2[20];

    uint8_t powerLevel : 4; // seems to range differently, packets listed in Minatsuki have 0x0C
    uint8_t : 4;

    uint8_t unused3[12];

    uint8_t redVelocity;
    uint8_t blueVelocity;
    uint8_t yellowVelocity;
    uint8_t greenVelocity;

    uint8_t yellowCymbalVelocity;
    uint8_t blueCymbalVelocity;
    uint8_t greenCymbalVelocity;

    uint8_t unused4[14];
} __attribute__((packed)) PS4RockBandDrums_Data_t;
typedef struct
{

    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t strumBar;
    uint8_t whammy;
    uint8_t tilt;

    // To make things easier we use bitfields here and them map to dpad later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t x : 1;  // square, white1
    uint8_t a : 1;  // cross, black1
    uint8_t b : 1;  // circle, black2
    uint8_t y : 1;  // triangle, black3

    uint8_t leftShoulder : 1;   // white2, l1
    uint8_t rightShoulder : 1;  // white3, r1
    uint8_t : 1;                // l2
    uint8_t : 1;                // r2

    uint8_t : 1;                 // share
    uint8_t start : 1;           // pause, options
    uint8_t leftThumbClick : 1;  // l3, ghtv
    uint8_t back : 1;            // r3, hero power, map it to back here as then this is like the ps3 and xb1 report

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // touchpad click

    uint8_t unused2[56];
} __attribute__((packed)) PS4GHLGuitar_Data_t;
typedef struct {
    uint8_t type;
    uint8_t u1;
    uint8_t size_challenge;
    uint8_t size_response;
    uint8_t u4[4];  // crc32?
} __attribute__((packed)) AuthPageSizeReport;

typedef struct {
    uint8_t type;      // 0
    uint8_t seq;       // 1
    uint8_t page;      // 2
    uint8_t sbz;       // 3
    uint8_t data[56];  // 4-59
    uint32_t crc32;    // 60-63
} __attribute__((packed)) AuthReport;

typedef struct {
    uint8_t type;        // 0
    uint8_t seq;         // 1
    uint8_t status;      // 2  0x10 = not ready, 0x00 = ready
    uint8_t padding[9];  // 3-11
    uint32_t crc32;      // 12-15
} __attribute__((packed)) AuthStatusReport;

enum BackendAuthState {
    OK,
    UNKNOWN_ERR,
    COMM_ERR,
    BUSY,
    NO_TRANSACTION,
};
enum PS4ReportIds {
    FEATURE_REPORT = 0x0303,
    SET_CHALLENGE = 0x03f0,
    GET_RESPONSE,
    GET_AUTH_STATUS,
    GET_AUTH_PAGE_SIZE,
};
/** Max payload size. */
#define PAYLOAD_MAX 0x38
/** Total length of a challenge. This is also the signature size. */
#define CHALLENGE_SIZE 0x100
/** Total length of a response. */
#define RESPONSE_SIZE 0x410

#define PS4_FEATURE_MAGIC 0x2721
#define PS5_FEATURE_MAGIC 0x2821

enum PS4DeviceType : uint8_t
{
    PS4DeviceStandard = 0x00,
    PS4DeviceGuitar = 0x01,
    PS4DeviceDrums = 0x02,
    // PS4 only
    PS4DeviceDjTurntable = 0x03,
    PS4DeviceDanceMat = 0x04,
    PS4DeviceNavigation = 0x05,
    PS4DeviceSteeringWheel = 0x06,
    PS4DeviceStick = 0x07,
    PS4DeviceFlightStick = 0x08,
};

// Feature report 0x03, which the PS4 and PS5 read to find out what a controller supports. Both
// consoles use the same layout, the PS5 adds the battery and player indicator bits to capability 2
struct PS4FeatureReport
{
    uint8_t report_id;
    uint16_t magic;
    uint8_t protocol_version;

    // capability
    uint8_t analog_pressure : 1;
    uint8_t motion_sensors : 1;
    uint8_t light_bar : 1;
    uint8_t vibration : 1;
    uint8_t extended_data : 1;
    uint8_t speaker : 1;
    uint8_t touch_pad : 1;
    uint8_t audio_jack : 1;

    uint8_t device_type;

    // dots per mm, as the digits before and after the decimal point, 0 without a touch pad
    uint8_t touch_pad_density_integer;
    uint8_t touch_pad_density_fraction;

    // motion sensor scale, all 0 without motion sensors
    uint16_t timestamp_tick_us;
    uint16_t gyro_multiplier;
    uint16_t gyro_divisor;
    uint16_t accel_multiplier;
    uint16_t accel_divisor;

    // 0x0d is what a DualShock uses, and should be used if you don't have sticks
    uint8_t left_stick_dead_zone;
    uint8_t right_stick_dead_zone;

    // capability 2
    uint8_t wireless : 1;
    uint8_t : 1;
    uint8_t battery : 1; // PS5 only
    uint8_t : 3;
    uint8_t vendor_interface : 1;
    uint8_t player_indicator : 1; // PS5 only

    uint8_t reserved[3];

    // depends on device_type
    union
    {
        struct
        {
            uint8_t selector_switch : 1;
            uint8_t tilt : 1;
            uint8_t whammy_bar : 1;
            uint8_t : 5;
            // not counting "no effect"
            uint8_t effect_count;
        } __attribute__((packed)) guitar;
        struct
        {
            // no tom 2 without this
            uint8_t four_pads : 1;
            uint8_t hi_hat : 1;
            uint8_t ride : 1;
            uint8_t crash : 1;
            uint8_t extra_foot_pedal : 1;
            uint8_t : 3;
        } __attribute__((packed)) drums;
        uint8_t extended[9];
    };

    uint8_t reserved2[15];
} __attribute__((packed));
static_assert(sizeof(PS4FeatureReport) == 48, "PS4 feature report is 48 bytes");
static_assert(offsetof(PS4FeatureReport, device_type) == 5, "Device type is feature report byte 5");
static_assert(offsetof(PS4FeatureReport, left_stick_dead_zone) == 18, "Dead zones are feature report bytes 18 and 19");
static_assert(offsetof(PS4FeatureReport, guitar) == 24, "Extended capability is feature report byte 24");
