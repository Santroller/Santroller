#pragma once
#include <stdint.h>

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

    uint32_t padding : 24;
    uint8_t mystery[22];
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

    uint32_t padding : 24;
    uint8_t mystery[22];
    uint8_t touchpadData[8];
    uint8_t mystery_2[21];
} __attribute__((packed)) PS4Gamepad_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t unused1;
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