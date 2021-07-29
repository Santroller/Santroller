#pragma once
#include <stdint.h>
#include <stdbool.h>

// Type Defines
// Enumeration for joystick buttons.
typedef enum {
    SWITCH_Y       = 0x01,
    SWITCH_B       = 0x02,
    SWITCH_A       = 0x04,
    SWITCH_X       = 0x08,
    SWITCH_L       = 0x10,
    SWITCH_R       = 0x20,
    SWITCH_ZL      = 0x40,
    SWITCH_ZR      = 0x80,
    SWITCH_MINUS   = 0x100,
    SWITCH_PLUS    = 0x200,
    SWITCH_LCLICK  = 0x400,
    SWITCH_RCLICK  = 0x800,
    SWITCH_HOME    = 0x1000,
    SWITCH_CAPTURE = 0x2000,
} JoystickButtons_t;

// Battery levels
#define BATTERY_FULL        0x08
#define BATTERY_MEDIUM      0x06
#define BATTERY_LOW         0x04
#define BATTERY_CRITICAL    0x02
#define BATTERY_EMPTY       0x00
#define BATTERY_CHARGING    0x01 // Can be OR'ed

// DPAD values
#define HAT_TOP          0x00
#define HAT_TOP_RIGHT    0x01
#define HAT_RIGHT        0x02
#define HAT_BOTTOM_RIGHT 0x03
#define HAT_BOTTOM       0x04
#define HAT_BOTTOM_LEFT  0x05
#define HAT_LEFT         0x06
#define HAT_TOP_LEFT     0x07
#define HAT_CENTER       0x08

// Analog sticks
#define STICK_MIN      0
#define STICK_CENTER 128
#define STICK_MAX    255

/** LED mask for the library LED driver, to indicate that the UART is not synced. */
#define LEDMASK_NOT_SYNCED             LEDS_NO_LEDS
/** LED mask for the library LED driver, to indicate that UART is synced and we're receiving packets. */
#define LEDMASK_SYNCED                 LEDS_ALL_LEDS
/** LED mask for the library LED driver, to indicate that we're in "connection lost" mode (empty buffer). */
#define LEDMASK_PAUSE_EMPTY_BUFFER     LEDS_LED1
/** LED mask for the library LED driver, to indicate that we're in "connection lost" mode (home buffer). */
#define LEDMASK_PAUSE_HOME_BUFFER      LEDS_LED2

typedef enum {
    COMMAND_NOP         = 0,
    COMMAND_SYNC_1      = 0x33,
    COMMAND_SYNC_2      = 0xCC,
    COMMAND_SYNC_START  = 0xFF
} Command_t;

typedef enum {
    RESP_USB_ACK        = 0x90, // USB report sent to Switch: Not currently used.
    RESP_UPDATE_ACK     = 0x91, // Sent to host after a valid packet
    RESP_UPDATE_NACK    = 0x92, // Sent to host after an incorrect packet (CRC mismatch)
    RESP_SYNC_START     = 0xFF, // Sent to host after COMMAND_SYNC_START
    RESP_SYNC_1         = 0xCC, // Sent to host after COMMAND_SYNC_1
    RESP_SYNC_OK        = 0x33, // Sent to host after COMMAND_SYNC_2, synchronization finished
} Response_t;

// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_subcommands_notes.md
typedef enum {
    SUBCOMMAND_CONTROLLER_STATE_ONLY        = 0x00,
    SUBCOMMAND_BLUETOOTH_MANUAL_PAIRING     = 0x01,
    SUBCOMMAND_REQUEST_DEVICE_INFO          = 0x02,
    SUBCOMMAND_SET_INPUT_REPORT_MODE        = 0x03,
    SUBCOMMAND_TRIGGER_BUTTONS_ELAPSED_TIME = 0x04,
    SUBCOMMAND_GET_PAGE_LIST_STATE          = 0x05,
    SUBCOMMAND_SET_HCI_STATE                = 0x06,
    SUBCOMMAND_RESET_PAIRING_INFO           = 0x07,
    SUBCOMMAND_SET_SHIPMENT_LOW_POWER_STATE = 0x08,
    SUBCOMMAND_SPI_FLASH_READ               = 0x10,
    SUBCOMMAND_SPI_FLASH_WRITE              = 0x11,
    SUBCOMMAND_SPI_SECTOR_ERASE             = 0x12,
    SUBCOMMAND_RESET_NFC_IR_MCU             = 0x20,
    SUBCOMMAND_SET_NFC_IR_MCU_CONFIG        = 0x21,
    SUBCOMMAND_SET_NFC_IR_MCU_STATE         = 0x22,
    SUBCOMMAND_SET_PLAYER_LIGHTS            = 0x30,
    SUBCOMMAND_GET_PLAYER_LIGHTS            = 0x31,
    SUBCOMMAND_SET_HOME_LIGHTS              = 0x38,
    SUBCOMMAND_ENABLE_IMU                   = 0x40,
    SUBCOMMAND_SET_IMU_SENSITIVITY          = 0x41,
    SUBCOMMAND_WRITE_IMU_REGISTERS          = 0x42,
    SUBCOMMAND_READ_IMU_REGISTERS           = 0x43,
    SUBCOMMAND_ENABLE_VIBRATION             = 0x48,
    SUBCOMMAND_GET_REGULATED_VOLTAGE        = 0x50,
} Switch_Subcommand_t;

// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/spi_flash_notes.md
typedef enum {
    ADDRESS_SERIAL_NUMBER         = 0x6000,
    ADDRESS_CONTROLLER_COLOR      = 0x6050,
    ADDRESS_FACTORY_PARAMETERS_1  = 0x6080,
    ADDRESS_FACTORY_PARAMETERS_2  = 0x6098,
    ADDRESS_FACTORY_CALIBRATION_1 = 0x6020,
    ADDRESS_FACTORY_CALIBRATION_2 = 0x603D,
    ADDRESS_STICKS_CALIBRATION    = 0x8010,
    ADDRESS_IMU_CALIBRATION       = 0x8028,
} SPI_Address_t;

typedef struct {
    uint8_t input[8];
    uint8_t crc8_ccitt;
    uint8_t received_bytes;
} Serial_Input_Packet_t;

// Standard input report sent to Switch (doesn't contain IMU data)
// Note that compilers can align and order bits in every byte however they want (endianness)
// Taken from https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_notes.md#standard-input-report-format
// The order in every byte is inverted
typedef struct {
    uint8_t connection_info: 4;
    uint8_t battery_level: 4;
    bool button_y: 1;
    bool button_x: 1;
    bool button_b: 1;
    bool button_a: 1;
    bool button_right_sr: 1;
    bool button_right_sl: 1;
    bool button_r: 1;
    bool button_zr: 1;
    bool button_minus: 1;
    bool button_plus: 1;
    bool button_thumb_r: 1;
    bool button_thumb_l: 1;
    bool button_home: 1;
    bool button_capture: 1;
    uint8_t dummy: 1;
    bool charging_grip: 1;
    bool dpad_down: 1;
    bool dpad_up: 1;
    bool dpad_right: 1;
    bool dpad_left: 1;
    bool button_left_sr: 1;
    bool button_left_sl: 1;
    bool button_l: 1;
    bool button_zl: 1;
    uint8_t analog[6];
    uint8_t vibrator_input_report;
} USB_SwitchStandardReport_t;

// Full (extended) input report sent to Switch, with IMU data
typedef struct {
    USB_SwitchStandardReport_t standardReport;
    uint16_t imu[3 * 2 * 3]; // each axis is uint16_t, 3 axis per sensor, 2 sensors (accel and gyro), 3 reports
} USB_SwitchExtendedReport_t;