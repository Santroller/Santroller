#include "hid.h"

#include <stdio.h>
#include <string.h>

#include "Usb.h"
#include "bt.h"
#include "commands.h"
#include "config.h"
#include "controllers.h"
#include "io.h"
#include "keyboard_mouse.h"
#include "pico_slave.h"
#include "pin_funcs.h"
#include "ps3_wii_switch.h"
#include "shared_main.h"
#include "stdint.h"
#include "usbhid.h"
#include "util.h"
#include "wii.h"

const PROGMEM char board[] = ARDWIINO_BOARD;
const PROGMEM char f_cpu_descriptor_str[] = STR(F_CPU);
uint8_t xbox_players[] = {
    0,  // 0x00	 All off
    0,  // 0x01	 All blinking
    1,  // 0x02	 1 flashes, then on
    2,  // 0x03	 2 flashes, then on
    3,  // 0x04	 3 flashes, then on
    4,  // 0x05	 4 flashes, then on
    1,  // 0x06	 1 on
    2,  // 0x07	 2 on
    3,  // 0x08	 3 on
    4,  // 0x09	 4 on
    0,  // 0x0A	 Rotating (e.g. 1-2-4-3)
    0,  // 0x0B	 Blinking*
    0,  // 0x0C	 Slow blinking*
    0,  // 0x0D	 Alternating (e.g. 1+4-2+3), then back to previous*
};

static const int ps4_colors[4][3] = {
    {0x00, 0x00, 0x40}, /* Blue */
    {0x40, 0x00, 0x00}, /* Red */
    {0x00, 0x40, 0x00}, /* Green */
    {0x20, 0x00, 0x20}  /* Pink */
};
uint8_t stage_kit_millis[] = {
    0,
    75,   // Fastest
    100,  // Fast
    125,  // Medium
    150,  // Slow
};
uint8_t current_player = 0xFF;
uint8_t strobe_delay = 0;
uint8_t last_star_power = 0;
bool star_power_active = false;
bool received_valid_command = false;
bool strobing = false;
long last_strobe = 0;
PCStageKitOutput_Data_t stage_kit_report = {0};
#ifdef INPUT_USB_HOST

uint8_t xone_sequences[8] = {1, 1, 1, 1, 1, 1, 1, 1};
// Support up to 8 connected ps3 controllers (no idea why someone would do that but hey)
ps4_output_report ps4_output_reports[8] = {
    {
        report_id : PS4_LED_RUMBLE_ID,
        valid_flag0 : 0xFF,
        valid_flag1 : 0x00,
        reserved1 : 0x00,
        motor_right : 0x00,
        motor_left : 0x00,
        lightbar_red : 0x00,
        lightbar_green : 0x00,
        lightbar_blue : 0x00,
        lightbar_blink_on : 0,
        lightbar_blink_off : 0,
        reserved : {0}

    }};
ps3_output_report ps3_output_reports[8] = {
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x01,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    },
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x01,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    },
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x01,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    },
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x01,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    },
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x01,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    },
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x01,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    },
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x01,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    },
    {
        report_id : PS3_LED_ID,
        rumble : {
            padding : 0x00,
            right_duration : 0xFF,
            right_motor_on : 0x00,
            left_duration : 0xFF,
            left_motor_force : 0x00,
        },
        padding : {0x00, 0x00, 0x00, 0x00},
        leds_bitmap : 0x00,
        led : {
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
            {time_enabled : 0xFF, duty_length : 0x27, enabled : 0x10, duty_off : 0x00, duty_on : 0x32},
        },
        _reserved : {time_enabled : 0x00, duty_length : 0x00, enabled : 0x00, duty_off : 0x00, duty_on : 0x00},
    }};
#endif

void handle_auth_led(void) {
    authentication_successful();
    HANDLE_AUTH_LED;
}
void handle_player_leds(uint8_t player) {
    if (player == current_player) return;
    if (player == 0) {
        player = current_player;
    }
    if (player == 0xFF) {
        player = 1;
    }
    current_player = player;
    HANDLE_PLAYER_LED;
#ifdef INPUT_USB_HOST
    USB_Device_Type_t type;
    for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
        type = get_usb_host_device_type(i);
        switch (type.console_type) {
            case PS3: {
                // Only actual ds3s support this
                if (type.sub_type == GAMEPAD) {
                    ps3_output_report *report = &ps3_output_reports[i];
                    report->leds_bitmap |= _BV(player);
                    transfer_with_usb_controller(type.dev_addr, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x0201, 0x00, sizeof(ps3_output_report), (uint8_t *)report);
                    // send_report_to_controller(type.dev_addr, (uint8_t *)report, sizeof(report));
                }
                return;
            }
            case PS4: {
                // Only actual ds4s support the lightbar
                if (type.sub_type == GAMEPAD) {
                    ps4_output_report *report = &ps4_output_reports[i];
                    report->lightbar_red = ps4_colors[player - 1][0];
                    report->lightbar_green = ps4_colors[player - 1][1];
                    report->lightbar_blue = ps4_colors[player - 1][2];
                    transfer_with_usb_controller(type.dev_addr, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x0205, 0x00, sizeof(ps4_output_report), (uint8_t *)report);
                }
                return;
            }
            case XBOX360: {
                XInputLEDReport_t report = {
                    rid : XBOX_LED_ID,
                    rsize : sizeof(XInputLEDReport_t),
                    led : (uint8_t)(player + LED_ONE - 1)
                };
                send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&report, sizeof(report));
                return;
            }
            case XBOX360_W: {
                uint8_t report[] = {0x00, 0x00, 0x08, (uint8_t)(0x40 | (player + LED_ONE - 1)), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                send_report_to_controller(type.dev_addr, type.instance, report, sizeof(report));
                return;
            }
        }
    }
#endif
}
// PS3 controllers and other controllers use different LED bitmasks
#if DEVICE_TYPE == GAMEPAD
void handle_player_leds_ps3(uint8_t player_mask) {
    if (player_mask == 2) {
        handle_player_leds(1);
    }
    if (player_mask == 4) {
        handle_player_leds(2);
    }
    if (player_mask == 8) {
        handle_player_leds(3);
    }
    if (player_mask == 16) {
        handle_player_leds(4);
    }
    if (player_mask == 18) {
        handle_player_leds(5);
    }
    if (player_mask == 20) {
        handle_player_leds(6);
    }
    if (player_mask == 24) {
        handle_player_leds(7);
    }
}
#else
void handle_player_leds_ps3(uint8_t player_mask) {
    if (player_mask == 1) {
        handle_player_leds(1);
    }
    if (player_mask == 2) {
        handle_player_leds(2);
    }
    if (player_mask == 4) {
        handle_player_leds(3);
    }
    if (player_mask == 8) {
        handle_player_leds(4);
    }
    if (player_mask == 9) {
        handle_player_leds(5);
    }
    if (player_mask == 10) {
        handle_player_leds(6);
    }
    if (player_mask == 12) {
        handle_player_leds(7);
    }
}
#endif
void handle_lightbar_leds(uint8_t red, uint8_t green, uint8_t blue) {
    HANDLE_LIGHTBAR_LED;
    // We know the default lightbar colours that sony uses so we can extract a player number from that.
    for (int i = 0; i < 4; i++) {
        if (red == ps4_colors[i][0] && green == ps4_colors[i][1] && blue == ps4_colors[i][2]) {
            handle_player_leds(i + 1);
        }
    }
}

void handle_rumble(uint8_t rumble_left, uint8_t rumble_right) {
    // printf("Rumble: %d %d\r\n", rumble_left, rumble_right);
    HANDLE_RUMBLE;
#ifdef HANDLE_LED_RUMBLE_OFF
    if (rumble_left == 0x00 && rumble_right == 0xFF) {
        last_strobe = 0;
        HANDLE_LED_RUMBLE_OFF;
    }
#endif

#if defined(INPUT_USB_HOST) && DEVICE_TYPE == GAMEPAD
    USB_Device_Type_t type;
    for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
        type = get_usb_host_device_type(i);
        if (type.sub_type != GAMEPAD && type.sub_type != XINPUT_WHEEL) continue;
        switch (type.console_type) {
            case PS3: {
                ps3_output_report *report = &ps3_output_reports[i];
                report->rumble.left_motor_force = rumble_left;
                report->rumble.right_motor_on = rumble_right != 0;
                transfer_with_usb_controller(i, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x0201, 0x00, sizeof(ps3_output_report), (uint8_t *)report);
                return;
            }
            case PS4: {
                ps4_output_report *report = &ps4_output_reports[i];
                report->motor_left = rumble_left;
                report->motor_right = rumble_right != 0;
                send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)report, sizeof(ps4_output_report));
                return;
            }
            case OG_XBOX: {
                uint8_t rumble_packet[] = {0x00, 0x06, 0x00, rumble_left, 0x00, rumble_right};
                send_report_to_controller(type.dev_addr, type.instance, rumble_packet, sizeof(rumble_packet));
                return;
            }
            case XBOX360: {
                XInputRumbleReport_t report = {
                    rid : XBOX_RUMBLE_ID,
                    rsize : sizeof(XInputRumbleReport_t),
                    leftRumble : rumble_left,
                    rightRumble : rumble_right
                };
                send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&report, sizeof(report));
                return;
            }
            case XBOX360_W: {
                uint8_t rumble_packet[] = {0x00, 0x01, 0x0f, 0xc0, 0x00, rumble_left, rumble_right, 0x00, 0x00, 0x00, 0x00, 0x00};
                send_report_to_controller(type.dev_addr, type.instance, rumble_packet, sizeof(rumble_packet));
                return;
            }
            case XBOXONE: {
                GipRumble_t report;
                GipRumble_t *packet = &report;
                GIP_HEADER(packet, GIP_CMD_RUMBLE, true, xone_sequences[i]++);
                report.leftMotor = rumble_left;
                report.rightMotor = rumble_right;
                if (xone_sequences[i] == 0) {
                    xone_sequences[i] = 1;
                }
                send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&report, sizeof(report));
            }
        }
    }
#endif
#if defined(INPUT_USB_HOST) && DEVICE_TYPE == STAGE_KIT
    USB_Device_Type_t type;
    // Only xinput has stage kit
    for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
        type = get_usb_host_device_type(i);
        if (type.sub_type != STAGE_KIT) continue;
        XInputRumbleReport_t report = {
            rid : XBOX_RUMBLE_ID,
            rsize : sizeof(XInputRumbleReport_t),
            leftRumble : rumble_left,
            rightRumble : rumble_right
        };
        send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&report, sizeof(report));
        return;
    }
#endif
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
    if (rumble_right == RUMBLE_SANTROLLER_EUPHORIA_LED) {
        lastEuphoriaLed = rumble_left != 0;
    }
#endif
#if defined(INPUT_USB_HOST) && DEVICE_TYPE == DJ_HERO_TURNTABLE
    USB_Device_Type_t type;
    // Only ps3 and xinput have dj turntables
    for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
        type = get_usb_host_device_type(i);
        if (type.sub_type != DJ_HERO_TURNTABLE) continue;
        switch (type.console_type) {
            case XBOX360: {
                XInputRumbleReport_t report = {
                    rid : XBOX_RUMBLE_ID,
                    rsize : sizeof(XInputRumbleReport_t),
                    leftRumble : rumble_left,
                    rightRumble : rumble_right
                };
                send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&report, sizeof(report));
                return;
            }
            case PS3: {
                ps3_turntable_output_report_t report = {
                    outputType : DJ_LED_ID,
                    unknown1 : 0x01,
                    enable : rumble_left != 0,
                    padding : {0}
                };
                transfer_with_usb_controller(i, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x0200, 0x00, sizeof(report), (uint8_t *)&report);
                return;
            }
        }
    }
#endif
}
void handle_rumble(PCStageKitOutputWithoutReportId_Data_t *report) {
    HANDLE_RUMBLE_EXPANDED;
}
void handle_keyboard_leds(uint8_t leds) {
    HANDLE_KEYBOARD_LED;
}

#if BLUETOOTH_RX
void handle_bt_rumble(uint8_t rumble_left, uint8_t rumble_right) {
    switch (rumble_right) {
        case RUMBLE_STAGEKIT_FOG_ON:
            stage_kit_report.report.stageKitFog = true;
            break;
        case RUMBLE_STAGEKIT_FOG_OFF:
            stage_kit_report.report.stageKitFog = false;
            break;
        case RUMBLE_STAGEKIT_SLOW_STROBE:
            stage_kit_report.report.stageKitStrobe = 1;
            break;
        case RUMBLE_STAGEKIT_MEDIUM_STROBE:
            stage_kit_report.report.stageKitStrobe = 2;
            break;
        case RUMBLE_STAGEKIT_FAST_STROBE:
            stage_kit_report.report.stageKitStrobe = 3;
            break;
        case RUMBLE_STAGEKIT_FASTEST_STROBE:
            stage_kit_report.report.stageKitStrobe = 4;
            break;
        case RUMBLE_STAGEKIT_NO_STROBE:
            stage_kit_report.report.stageKitStrobe = 0;
            break;
        case RUMBLE_STAGEKIT_ALLOFF:
            stage_kit_report.report.stageKitFog = false;
            stage_kit_report.report.stageKitStrobe = 0;
            stage_kit_report.report.stageKitBlue = 0;
            stage_kit_report.report.stageKitGreen = 0;
            stage_kit_report.report.stageKitYellow = 0;
            stage_kit_report.report.stageKitRed = 0;
            break;
        case RUMBLE_STAGEKIT_BLUE:
            stage_kit_report.report.stageKitBlue = rumble_left;
            break;
        case RUMBLE_STAGEKIT_GREEN:
            stage_kit_report.report.stageKitGreen = rumble_left;
            break;
        case RUMBLE_STAGEKIT_YELLOW:
            stage_kit_report.report.stageKitYellow = rumble_left;
            break;
        case RUMBLE_STAGEKIT_RED:
            stage_kit_report.report.stageKitRed = rumble_left;
            break;
        case RUMBLE_SANTROLLER_NOTE_MISS:
            stage_kit_report.report.noteMiss = rumble_left;
            break;
        case RUMBLE_SANTROLLER_MULTIPLIER:
            stage_kit_report.report.multiplier = rumble_left;
            break;
        case RUMBLE_SANTROLLER_SOLO:
            stage_kit_report.report.soloActive = rumble_left;
            break;
        case RUMBLE_SANTROLLER_STAR_POWER_ACTIVE:
            stage_kit_report.report.starPowerActive = rumble_left;
            break;
        case RUMBLE_SANTROLLER_STAR_POWER_FILL:
            stage_kit_report.report.starPowerState = rumble_left;
            break;
        case RUMBLE_SANTROLLER_NOTE_HIT:
            stage_kit_report.report.noteHitRaw = rumble_left;
            break;
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
        case RUMBLE_SANTROLLER_EUPHORIA_LED:
            stage_kit_report.report.euphoriaBrightness = rumble_left;
            break;
#endif
    }
    stage_kit_report.reportId = PS3_RUMBLE_ID;
    stage_kit_report.report.reportTypeId = SANTROLLER_LED_EXPANDED_ID;
    bt_set_report((uint8_t *)&stage_kit_report, sizeof(stage_kit_report), 1, 1);
}
#endif

void hid_set_report(const uint8_t *data, uint8_t len, uint8_t reportType, uint8_t report_id) {
    // for (int i = 0; i < len; i++) {
    //     printf("%02x, ", data[i]);
    // }
    // printf("\r\n");
    // if (data[0] == 0x0b) {
    //     if (data[2] == 0x52) {
    //         portal_state = 1;
    //     }
    // }
#if DEVICE_TYPE_IS_KEYBOARD
    handle_keyboard_leds(data[0]);
#endif
    uint8_t id = data[0];
#if defined(INPUT_USB_HOST)
    for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
        USB_Device_Type_t type = get_usb_host_device_type(i);
        if (type.console_type != SANTROLLER) continue;
        // Convert xinput payloads to their hid counterparts and send
        if ((consoleType == XBOX360 || consoleType == WINDOWS) && report_id != BLUETOOTH_REPORT) {
            uint8_t data_hid[8] = {0};
            data_hid[0] = PS3_REPORT_ID;
            if (id == XBOX_LED_ID) {
                uint8_t led = data[2];
                uint8_t player = xbox_players[led];
                if (player) {
                    data_hid[1] = PS3_LED_ID;
                    data_hid[3] = 1 << player;
                    send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&data_hid, sizeof(data_hid));
                }

            } else if (id == XBOX_RUMBLE_ID) {
                data_hid[1] = SANTROLLER_LED_ID;
                data_hid[2] = data[3];
                data_hid[3] = data[4];
                send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&data_hid, sizeof(data_hid));
            }
        } else if ((consoleType == XBOXONE) && report_id != BLUETOOTH_REPORT) {
            if (xbox_one_state == Ready) {
                uint8_t data_hid[8] = {0};
                data_hid[0] = PS3_REPORT_ID;
                // Live guitar is a bit special, so handle it here
#if DEVICE_TYPE == LIVE_GUITAR
                if (id == GHL_HID_OUTPUT) {
                    uint8_t sub_id = data[1];
                    if (sub_id == PS3_RUMBLE_ID) {
                        uint8_t player = (data[3] & 0x0F);
                        data_hid[1] = PS3_RUMBLE_ID;
                        data_hid[3] = 1 << player;
                        send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&data_hid, sizeof(data_hid));
                    }
                }
#endif
                if (id == GIP_CMD_RUMBLE) {
                    GipRumble_t *rumble = (GipRumble_t *)data;
                    data_hid[1] = SANTROLLER_LED_ID;
                    data_hid[2] = rumble->leftMotor;
                    data_hid[3] = rumble->rightMotor;
                    send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)&data_hid, sizeof(data_hid));
                }
            }
        } else {
            // Already a hid payload, send it as is
            send_report_to_controller(type.dev_addr, type.instance, data, len);
        }
    }
#endif
#if BLUETOOTH_RX
    // BT uses HID, so convert XInput based reports to HID before sending them over BT
    if (consoleType == XBOX360 || consoleType == WINDOWS) {
        uint8_t data_hid[8] = {0};
        data_hid[0] = PS3_REPORT_ID;
        if (id == XBOX_LED_ID) {
            uint8_t led = data[2];
            uint8_t player = xbox_players[led];
            if (player) {
                handle_player_leds(player);
                data_hid[1] = PS3_RUMBLE_ID;
                data_hid[3] = 1 << player;
                bt_set_report(data_hid, 8, reportType, report_id);
            }

        } else if (id == XBOX_RUMBLE_ID) {
#if DEVICE_TYPE == GAMEPAD
            data_hid[1] = SANTROLLER_LED_ID;
            data_hid[2] = data[3];
            data_hid[3] = data[4];
            bt_set_report(data_hid, 8, reportType, report_id);
#else
            handle_bt_rumble(data[3], data[4]);
#endif
        }
    } else {
#if DEVICE_TYPE == GAMEPAD
        bt_set_report(data, len, reportType, report_id);
#else
        // Convert rumble based reports to combined reports
        if (id == SANTROLLER_LED_ID) {
            handle_bt_rumble(data[1], data[2]);
        } else if (id == PS3_REPORT_ID && data[1] == SANTROLLER_LED_ID) {
            handle_bt_rumble(data[2], data[3]);
            // Pass combined reports directly over bt
        } else if (id == SANTROLLER_LED_EXPANDED_ID || (id == PS3_REPORT_ID && data[1] == SANTROLLER_LED_EXPANDED_ID)) {
            bt_set_report(data, len, reportType, report_id);
        }
#endif
    }

#endif
#ifdef INPUT_USB_HOST
    // Handle Xbox 360 LEDs and rumble
    // Handle XBOX One Auth
    if ((consoleType == XBOXONE) && report_id != BLUETOOTH_REPORT) {
        if (xbox_one_state == WaitingDesc1) {
            xbox_one_state = IdentDesc1;
        } else if (xbox_one_state == WaitingDesc) {
            xbox_one_state = IdentDesc;
        } else if (xbox_one_state == WaitingDescEnd) {
            xbox_one_state = IdentDescEnd;
        } else if (xbox_one_state == Auth) {
            delay(1);
            if (data[0] == GIP_AUTHENTICATION && len == 6 && data[3] == 2 && data[4] == 1 && data[5] == 0) {
                handle_auth_led();
                printf("Ready!\r\n");
                xbox_one_state = Ready;
                data_from_console_size = len;
                memcpy(data_from_console, data, len);
            } else if (data[0] != GIP_POWER_MODE_DEVICE_CONFIG && data[0] != 2 && data[0] != 4) {
                data_from_console_size = len;
                memcpy(data_from_console, data, len);
            }
        } else if (xbox_one_state == Ready) {
            // Live guitar is a bit special, so handle it here
#if DEVICE_TYPE == LIVE_GUITAR
            if (id == GHL_HID_OUTPUT) {
                uint8_t sub_id = data[1];
                if (sub_id == PS3_RUMBLE_ID) {
                    uint8_t player = (data[3] & 0x0F);
                    handle_player_leds(player + 1);
                }
            }
#endif
            if (id == GIP_CMD_RUMBLE) {
                GipRumble_t *rumble = (GipRumble_t *)data;
                handle_rumble(rumble->leftMotor, rumble->rightMotor);
            }
        }
    } else
#endif
        // Bt reports are ALWAYS hid even if USB is in xinput mode
        if ((consoleType == XBOX360 || consoleType == WINDOWS) && report_id != BLUETOOTH_REPORT) {
            if (id == XBOX_LED_ID) {
                uint8_t led = data[2];
                uint8_t player = xbox_players[led];
                if (player) {
                    handle_player_leds(player);
                }

            } else if (id == XBOX_RUMBLE_ID) {
                uint8_t rumble_left = data[3];
                uint8_t rumble_right = data[4];
                // Turntable commands will conflict as the actual game sends rumble out on both motors at the same time
                // To solve this problem, we stay in a turntable compatibility mode until we receive a valid santroller command
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
                if ((rumble_right >= RUMBLE_SANTROLLER_STAR_POWER_FILL && rumble_right <= RUMBLE_SANTROLLER_NOTE_MISS) || rumble_right == RUMBLE_SANTROLLER_NOTE_HIT) {
                    if (rumble_left != rumble_right) {
                        received_valid_command = true;
                    }
                }
                // Turntable led emulation mode
                if (!received_valid_command && rumble_left == rumble_right) {
                    rumble_right = RUMBLE_SANTROLLER_EUPHORIA_LED;
                }
#endif
                handle_rumble(rumble_left, rumble_right);
            }
        } else {
            if (id == PS4_LED_RUMBLE_ID) {
                ps4_output_report *report = (ps4_output_report *)data;
                handle_lightbar_leds(report->lightbar_red, report->lightbar_green, report->lightbar_blue);
                handle_rumble(report->motor_left, report->motor_right);

#if defined(INPUT_USB_HOST) && DEVICE_TYPE == GAMEPAD
                USB_Device_Type_t type;
                for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
                    type = get_usb_host_device_type(i);
                    if (type.sub_type != GAMEPAD || type.console_type != PS4) continue;
                    send_report_to_controller(type.dev_addr, type.instance, (uint8_t *)report, sizeof(ps4_output_report));
                    return;
                }
#endif
            }
#if DEVICE_TYPE_IS_INSTRUMENT
            else if (id == PS3_REPORT_ID && data[1] == SANTROLLER_LED_ID) {
                handle_rumble(data[2], data[3]);
            } else if (id == PS3_REPORT_ID && data[1] == SANTROLLER_LED_EXPANDED_ID) {
                PCStageKitOutput_Data_t *output = (PCStageKitOutput_Data_t *)data;
                handle_rumble(&output->report);
            } else if (id == PS3_REPORT_ID && data[1] == PS3_RUMBLE_ID) {
                uint8_t player = data[3];
                handle_player_leds_ps3(player);
            } else if (id == PS3_RUMBLE_ID) {
                uint8_t player = data[2];
                handle_player_leds_ps3(player);
            } else if (id == SANTROLLER_LED_ID) {
                handle_rumble(data[1], data[2]);
            } else if (id == SANTROLLER_LED_EXPANDED_ID) {
                PCStageKitOutputWithoutReportId_Data_t *output = (PCStageKitOutputWithoutReportId_Data_t *)data;
                handle_rumble(output);
            }
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
            else if (id == DJ_LED_ID) {
                uint8_t euphoria_on = data[2] * 0xFF;
                // Use the santroller euphoria rumble command so that rumble is correctly mapped
                handle_rumble(euphoria_on, RUMBLE_SANTROLLER_EUPHORIA_LED);
            }
#endif
#else
        else if (id == PS3_LED_ID) {
            handle_player_leds_ps3(data[9]);
            handle_rumble(data[0x04], data[0x02] ? 0xff : 0);
        } else if (id == PS3_RUMBLE_ID) {
            handle_rumble(data[0x05], data[0x03] ? 0xff : 0);
        }
#endif
        }
}
long millis_since_command = 0;
uint8_t handle_serial_command(uint8_t request, uint16_t wValue, uint8_t *response_buffer, bool *success) {
    switch (request) {
        case COMMAND_READ_CONFIG: {
            if (wValue > CONFIGURATION_LEN) {
                return 0;
            }
            uint16_t size = CONFIGURATION_LEN - wValue;
            if (size > 64) size = 64;
            memcpy_P(response_buffer, config + wValue, size);
            return size;
        }
        case COMMAND_READ_SERIAL:
            read_serial(response_buffer, 20);
            return 20;
        case COMMAND_READ_BOARD:
            memcpy_P(response_buffer, board, sizeof(board));
            return sizeof(board);
        case COMMAND_READ_F_CPU:
            memcpy_P(response_buffer, f_cpu_descriptor_str, sizeof(f_cpu_descriptor_str));
            return sizeof(f_cpu_descriptor_str);

        case COMMAND_GET_EXTENSION_WII:
            if (!lastWiiWasSuccessful) {
                return 0;
            }
            memcpy(response_buffer, &wiiControllerType, sizeof(wiiControllerType));
            return sizeof(wiiControllerType);
        case COMMAND_GET_EXTENSION_PS2:
            if (!lastPS2WasSuccessful) {
                return 0;
            }
            memcpy(response_buffer, &ps2ControllerType, sizeof(ps2ControllerType));
            return sizeof(ps2ControllerType);
        case COMMAND_READ_WII:
            if (!lastWiiWasSuccessful) {
                return 0;
            }
            memcpy(response_buffer, &lastSuccessfulWiiPacket, wiiBytes);
            return wiiBytes;
        case COMMAND_READ_PS2:
            if (!lastPS2WasSuccessful) {
                return 0;
            }
            memcpy(response_buffer, &lastSuccessfulPS2Packet, sizeof(lastSuccessfulPS2Packet));
            return sizeof(lastSuccessfulPS2Packet);
        case COMMAND_READ_DJ_LEFT:
            if (!lastTurntableWasSuccessfulLeft) {
                return 0;
            }
            memcpy(response_buffer, &lastSuccessfulTurntablePacketLeft, sizeof(lastSuccessfulTurntablePacketLeft));
            return sizeof(lastSuccessfulTurntablePacketLeft);
        case COMMAND_READ_DJ_RIGHT:
            if (!lastTurntableWasSuccessfulRight) {
                return 0;
            }
            memcpy(response_buffer, &lastSuccessfulTurntablePacketRight, sizeof(lastSuccessfulTurntablePacketRight));
            return sizeof(lastSuccessfulTurntablePacketRight);
        case COMMAND_READ_GH5:
            if (!lastGH5WasSuccessful) {
                return 0;
            }
            memcpy(response_buffer, &lastSuccessfulGH5Packet, sizeof(lastSuccessfulGH5Packet));
            return sizeof(lastSuccessfulGH5Packet);
        case COMMAND_READ_CLONE:
            if (!lastCloneWasSuccessful) {
                return 0;
            }
            memcpy(response_buffer, &lastSuccessfulClonePacket, sizeof(lastSuccessfulClonePacket));
            return sizeof(lastSuccessfulClonePacket);
#ifdef INPUT_WT_NECK
        case COMMAND_READ_GHWT: {
            volatile uint8_t *data = (volatile uint8_t *)lastWt;
            for (int i = 0; i < sizeof(lastWt); i++) {
                response_buffer[i] = data[i];
            }
            return sizeof(lastWt);
        }
#endif
        case COMMAND_READ_ANALOG: {
            uint8_t pin = wValue & 0xff;
            uint8_t mask = (wValue >> 8);
            uint16_t response = adc_read(pin, mask);
            memcpy(response_buffer, &response, sizeof(response));
            return sizeof(response);
        }
        case COMMAND_READ_ACCEL: {
            memcpy(response_buffer, &filtered, sizeof(filtered));
            memcpy(response_buffer+sizeof(filtered), &accel_adc, sizeof(accel_adc));
            return sizeof(filtered) + sizeof(accel_adc);
        }
        case COMMAND_ACCEL_VALID: {
            response_buffer[0] = accel_found;
            return 1;
        }
        case COMMAND_SET_ACCEL_FILTER: {
            memcpy(&currentLowPassAlpha, response_buffer, sizeof(currentLowPassAlpha));
            return 0;
        }
        case COMMAND_READ_DIGITAL: {
            uint8_t port = wValue & 0xff;
            uint8_t mask = (wValue >> 8);
            uint8_t response = digital_read(port, mask);
            memcpy(response_buffer, &response, sizeof(response));
            return sizeof(response);
        }
        case COMMAND_WRITE_ANALOG: {
            uint8_t port = response_buffer[0];
            uint8_t value = response_buffer[1];
            analogWrite(port, value);
            return 0;
        }
        case COMMAND_WRITE_DIGITAL: {
            uint8_t port = response_buffer[0];
            uint8_t mask = response_buffer[1];
            uint8_t activeMask = response_buffer[2];
            digital_write(port, mask, activeMask);
            return 0;
        }
        case COMMAND_LED_BRIGHTNESS: {
            brightness = response_buffer[0];
            return 0;
        }
        case COMMAND_DISABLE_MULTIPLEXER: {
            disable_multiplexer = response_buffer[0];
        }
#ifdef INPUT_MIDI
        case COMMAND_READ_MIDI: {
            memcpy(response_buffer, &midiData, sizeof(Midi_Data_t));
            return sizeof(Midi_Data_t);
        }
#endif
#ifdef MAX1704X_TWI_PORT
        case COMMAND_READ_MAX170X_VALID:
            response_buffer[0] = max170x_init;
            return 1;
        case COMMAND_READ_MAX170X:
            memcpy(response_buffer, &lastBattery, sizeof(lastBattery));
            return sizeof(lastBattery);
#endif
#ifdef MPR121_TWI_PORT
        case COMMAND_READ_MPR121_VALID:
            response_buffer[0] = mpr121_init;
            return 1;
        case COMMAND_READ_MPR121:
            memcpy(response_buffer, &lastMpr121, sizeof(lastMpr121));
            return sizeof(lastMpr121);
#endif
#ifdef SLAVE_TWI_PORT
#ifdef INPUT_WT_SLAVE_NECK
        case COMMAND_READ_PERIPHERAL_GHWT:
            return slaveReadWtRaw(response_buffer);
#endif
        case COMMAND_READ_PERIPHERAL_VALID:
            response_buffer[0] = slave_initted;
            return 1;
        case COMMAND_READ_PERIPHERAL_DIGITAL: {
            uint8_t port = wValue & 0xff;
            uint8_t mask = (wValue >> 8);
            uint8_t response = slaveReadDigital(port, mask);
            memcpy(response_buffer, &response, sizeof(response));
            return sizeof(response);
        }
#endif
#if LED_COUNT_WS2812
        case COMMAND_SET_LEDS: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_WS2812) return 0;
            if (!response_buffer[1]) {
                ledState[led].select = 0;
                return 0;
            }
            memcpy(&ledState[led], response_buffer, sizeof(Led_WS2812_t));
            ledState[led].select = 1;
            return 0;
        }
#endif
#if LED_COUNT_WS2812W
        case COMMAND_SET_LEDS: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_WS2812W) return 0;
            if (!response_buffer[1]) {
                ledState[led].select = 0;
                return 0;
            }
            memcpy(&ledState[led], response_buffer, sizeof(Led_WS2812_t));
            ledState[led].select = 1;
            return 0;
        }
#endif
#if LED_COUNT_PERIPHERAL_WS2812
        case COMMAND_SET_LEDS_PERIPHERAL: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_PERIPHERAL_WS2812) return 0;
            if (!response_buffer[1]) {
                ledStatePeripheral[led].select = 0;
                return 0;
            }
            memcpy(&ledStatePeripheral[led], response_buffer, sizeof(Led_WS2812_t));
            ledStatePeripheral[led].select = 1;
            return 0;
        }
#endif
#if LED_COUNT_PERIPHERAL_WS2812W
        case COMMAND_SET_LEDS_PERIPHERAL: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_PERIPHERAL_WS2812W) return 0;
            if (!response_buffer[1]) {
                ledStatePeripheral[led].select = 0;
                return 0;
            }
            memcpy(&ledStatePeripheral[led], response_buffer, sizeof(Led_WS2812_t));
            ledStatePeripheral[led].select = 1;
            return 0;
        }
#endif
#if LED_COUNT
        case COMMAND_SET_LEDS: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT) return 0;
            if (response_buffer[1] == 0 && response_buffer[2] == 0 && response_buffer[3] == 0) {
                ledState[led].select = 0;
                return 0;
            }
            ledState[led].select = 1;
            ledState[led].brightness = response_buffer[1];
            ledState[led].r = response_buffer[2];
            ledState[led].g = response_buffer[3];
            ledState[led].b = response_buffer[4];
            return 0;
        }
#endif
#if LED_COUNT_PERIPHERAL
        case COMMAND_SET_LEDS_PERIPHERAL: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_PERIPHERAL) return 0;
            if (response_buffer[1] == 0 && response_buffer[2] == 0 && response_buffer[3] == 0) {
                ledStatePeripheral[led].select = 0;
                return 0;
            }
            ledStatePeripheral[led].select = 1;
            ledStatePeripheral[led].brightness = response_buffer[1];
            ledStatePeripheral[led].r = response_buffer[2];
            ledStatePeripheral[led].g = response_buffer[3];
            ledStatePeripheral[led].b = response_buffer[4];
            return 0;
        }
#endif
#if LED_COUNT_MPR121
        case COMMAND_SET_LEDS_MPR121: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_MPR121) return 0;
            if (response_buffer[1]) {
                bit_set(ledStateMpr121, led % 8);
                bit_set(ledStateMpr121Select, led % 8);
            } else {
                bit_clear(ledStateMpr121, led % 8);
                bit_clear(ledStateMpr121Select, led % 8);
            }
            return 0;
        }
#endif
#if LED_COUNT_STP
        case COMMAND_SET_LEDS: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_STP) return 0;
            if (response_buffer[1]) {
                bit_set(ledState[led >> 3], led % 8);
                bit_set(ledStateSelect[led >> 3], led % 8);
            } else {
                bit_clear(ledState[led >> 3], led % 8);
                bit_clear(ledStateSelect[led >> 3], led % 8);
            }
            return 0;
        }
#endif
#if LED_COUNT_PERIPHERAL_STP
        case COMMAND_SET_LEDS_PERIPHERAL: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT_PERIPHERAL_STP) return 0;
            if (response_buffer[1]) {
                bit_set(ledStatePeripheral[led >> 3], led % 8);
                bit_set(ledStatePeripheralSelect[led >> 3], led % 8);
            } else {
                bit_clear(ledStatePeripheral[led >> 3], led % 8);
                bit_clear(ledStatePeripheralSelect[led >> 3], led % 8);
            }
            return 0;
        }
#endif
#ifdef INPUT_USB_HOST
        case COMMAND_READ_USB_HOST: {
            return read_usb_host_devices(response_buffer);
        }
        case COMMAND_READ_USB_HOST_INPUTS: {
            memcpy(response_buffer, &last_usb_host_data, sizeof(last_usb_host_data));
            return sizeof(last_usb_host_data);
        }
#endif
#if BLUETOOTH_RX
        case COMMAND_START_BT_SCAN: {
            bt_start_scan();
            return 0;
        }
        case COMMAND_STOP_BT_SCAN: {
            bt_stop_scan();
            return 0;
        }
        case COMMAND_GET_BT_DEVICES: {
            return bt_get_scan_results(response_buffer);
        }
        case COMMAND_READ_BLUETOOTH_INPUTS: {
            memcpy(response_buffer, &bt_data, sizeof(bt_data));
            return sizeof(bt_data);
        }
#endif
#if BLUETOOTH
        case COMMAND_GET_BT_STATE: {
            response_buffer[0] = check_bluetooth_ready();
            return 1;
        }
        case COMMAND_GET_BT_ADDRESS: {
            return get_bt_address(response_buffer);
        }
#endif
    }
    *success = false;
    return 0;
}

uint8_t hid_get_report(uint8_t *data, uint8_t reqlen, uint8_t reportType, uint8_t report_id) {
    return 0;
}