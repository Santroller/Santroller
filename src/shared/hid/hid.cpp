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
#include "pins.h"
#include "ps3_wii_switch.h"
#include "rf.h"
#include "shared_main.h"
#include "stdint.h"
#include "usbhid.h"
#include "util.h"

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
    150,  // Slow
    125,  // Medium
    100,  // Fast
    75,   // Fastest
};
uint8_t strobe_delay = 0;
uint8_t last_star_power = 0;
bool star_power_active = false;
bool strobing = false;
long last_strobe = 0;
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
        report_id : PS3_LED_RUMBLE_ID,
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
        report_id : PS3_LED_RUMBLE_ID,
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
        report_id : PS3_LED_RUMBLE_ID,
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
        report_id : PS3_LED_RUMBLE_ID,
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
        report_id : PS3_LED_RUMBLE_ID,
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
        report_id : PS3_LED_RUMBLE_ID,
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
        report_id : PS3_LED_RUMBLE_ID,
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
        report_id : PS3_LED_RUMBLE_ID,
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
    }};
#endif
const uint8_t PS3_REPORT_BUFFER[PS3_REPORT_BUFFER_SIZE] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0x27, 0x10, 0x00, 0x32,
    0xff, 0x27, 0x10, 0x00, 0x32,
    0xff, 0x27, 0x10, 0x00, 0x32,
    0xff, 0x27, 0x10, 0x00, 0x32,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#ifdef RF_RX
void handle_auth_led(void) {
    RfAuthLedPacket_t packet = {
        AckAuthLed};
    radio.writeAckPayload(1, &packet, sizeof(packet));
}
void handle_player_leds(uint8_t player) {
    RfPlayerLed_t packet = {
        AckPlayerLed,
        player};
    radio.writeAckPayload(1, &packet, sizeof(packet));
}
void handle_lightbar_leds(uint8_t r, uint8_t g, uint8_t b) {
    RfPlayerLed_t packet = {
        AckPlayerLed,
        r};
    radio.writeAckPayload(1, &packet, sizeof(packet));
}
void handle_rumble(uint8_t rumble_left, uint8_t rumble_right) {
    RfRumbleLed_t packet = {
        AckRumble,
        rumble_left,
        rumble_right};
    radio.writeAckPayload(1, &packet, sizeof(packet));
}
void handle_keyboard_leds(uint8_t leds) {
    RfRumbleLed_t packet = {
        AckKeyboardLed,
        leds};
    radio.writeAckPayload(1, &packet, sizeof(packet));
}
#else
void handle_auth_led(void) {
    HANDLE_AUTH_LED;
}
void handle_player_leds(uint8_t player) {
    HANDLE_PLAYER_LED;
#ifdef INPUT_USB_HOST
    USB_Device_Type_t type;
    for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
        type = get_usb_host_device_type(i);
        switch (type.console_type) {
            case PS3: {
                ps3_output_report *report = &ps3_output_reports[i];
                report->leds_bitmap |= _BV(player);
                transfer_with_usb_controller(i, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x0201, 0x00, sizeof(ps3_output_report), (uint8_t *)report);
                return;
            }
            case PS4: {
                // Only actual ds4s support the lightbar
                if (type.sub_type == GAMEPAD) {
                    ps4_output_report *report = &ps4_output_reports[i];
                    report->lightbar_red = ps4_colors[player - 1][0];
                    report->lightbar_green = ps4_colors[player - 1][1];
                    report->lightbar_blue = ps4_colors[player - 1][2];
                    transfer_with_usb_controller(i, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x0205, 0x00, sizeof(ps4_output_report), (uint8_t *)report);
                }
                return;
            }
            case XBOX360: {
                XInputLEDReport_t report = {
                    rid : XBOX_LED_ID,
                    rsize : sizeof(XInputLEDReport_t),
                    led : (xinput_led_t)(player + ONE - 1)
                };
                send_report_to_controller(type.dev_addr, (uint8_t *)&report, sizeof(report));
                return;
            }
        }
    }
#endif
}
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
    HANDLE_RUMBLE;
#ifdef HANDLE_LED_RUMBLE_OFF
    if (rumble_left == 0x00 && rumble_right == 0xFF) {
        HANDLE_LED_RUMBLE_OFF;
    }
#endif

#if defined(INPUT_USB_HOST) && CONSOLE_TYPE == UNIVERSAL && DEVICE_TYPE == GAMEPAD
    USB_Device_Type_t type;
    for (uint8_t i = 0; i < get_usb_host_device_count(); i++) {
        type = get_usb_host_device_type(i);
        if (type.sub_type != GAMEPAD) continue;
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
                report->motor_right = rumble_right;
                transfer_with_usb_controller(i, (USB_SETUP_HOST_TO_DEVICE | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), HID_REQUEST_SET_REPORT, 0x0205, 0x00, sizeof(ps4_output_report), (uint8_t *)report);
                return;
            }
            case XBOX360: {
                XInputRumbleReport_t report = {
                    rid : XBOX_RUMBLE_ID,
                    rsize : sizeof(XInputRumbleReport_t),
                    leftRumble : rumble_left,
                    rightRumble : rumble_right
                };
                send_report_to_controller(type.dev_addr, (uint8_t *)&report, sizeof(report));
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
                send_report_to_controller(type.dev_addr, (uint8_t *)&report, sizeof(report));
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
        send_report_to_controller(type.dev_addr, (uint8_t *)&report, sizeof(report));
        return;
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
                send_report_to_controller(type.dev_addr, (uint8_t *)&report, sizeof(report));
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
void handle_keyboard_leds(uint8_t leds) {
    HANDLE_KEYBOARD_LED;
}
#endif
void hid_set_report(const uint8_t *data, uint8_t len, uint8_t reportType, uint8_t report_id) {
    if (consoleType == WINDOWS_XBOXONE && report_id == INTERRUPT_ID) {
        if (data[0] == XBOX_LED_ID) {
            consoleType = WINDOWS;
            reset_usb();
        } else if (data[0] == GIP_DEVICE_DESCRIPTOR) {
            consoleType = XBOXONE;
            reset_usb();
        }
    }
#if CONSOLE_TYPE == KEYBOARD_MOUSE
    handle_keyboard_leds(data[0]);
#endif
    uint8_t id = data[0];
    // Handle Xbox 360 LEDs and rumble
    // Handle XBOX One Auth
    if (consoleType == XBOXONE) {
        if (xbox_one_state == Waiting1) {
            xbox_one_state = Ident1;
        } else if (xbox_one_state == Waiting2) {
            xbox_one_state = Ident2;
        } else if (xbox_one_state == Waiting5) {
            xbox_one_state = Ident5;
        } else if (xbox_one_state == Auth) {
            if (data[0] == 6 && len == 6 && data[3] == 2 && data[4] == 1 && data[5] == 0) {
                handle_auth_led();
                // printf("Ready!\r\n");
                xbox_one_state = Ready;
                data_from_console_size = len;
                memcpy(data_from_console, data, len);
            } else {
                data_from_console_size = len;
                memcpy(data_from_console, data, len);
            }
        } else if (xbox_one_state == Ready) {
            // Live guitar is a bit special, so handle it here
#if DEVICE_TYPE == LIVE_GUITAR
            if (id == 0x22) {
                uint8_t sub_id = data[1];
                if (sub_id == PS3_LED_RUMBLE_ID) {
                    uint8_t player = (data[3] & 0x0F);
                    handle_player_leds(player + 1);
                } else if (sub_id == XBOX_ONE_GHL_POKE_ID) {
                    last_ghl_poke_time = millis();
                }
            }
#endif
            if (id == GIP_CMD_RUMBLE) {
                GipRumble_t *rumble = (GipRumble_t *)data;
                handle_rumble(rumble->leftMotor, rumble->rightMotor);
            }
        }
    } else if (consoleType == XBOX360 || consoleType == WINDOWS) {
        if (id == XBOX_LED_ID) {
            uint8_t led = data[2];
            uint8_t player = xbox_players[led];
            handle_player_leds(player);
        } else if (id == XBOX_RUMBLE_ID) {
            uint8_t rumble_left = data[3];
            uint8_t rumble_right = data[4];
            handle_rumble(rumble_left, rumble_right);
        }
    } else {
        if (id == PS4_LED_RUMBLE_ID) {
            ps4_output_report *report = (ps4_output_report *)data;
            handle_lightbar_leds(report->lightbar_red, report->lightbar_green, report->lightbar_blue);
            handle_rumble(report->motor_left, report->motor_right);
        }
#if DEVICE_TYPE_IS_INSTRUMENT
        else if (id == PS3_REPORT_ID && data[1] == SANTROLLER_LED_ID) {
            handle_rumble(data[2], data[3]);
        } else if (id == PS3_REPORT_ID && data[1] == PS3_LED_RUMBLE_ID) {
            uint8_t player = (data[2] & 0x0F);
            handle_player_leds(player + 1);
        } else if (id == PS3_LED_RUMBLE_ID) {
            uint8_t player = (data[2] & 0x0F);
            handle_player_leds(player + 1);
        } else if (id == SANTROLLER_LED_ID) {
            handle_rumble(data[1], data[2]);
        }
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
        else if (id == DJ_LED_ID) {
            uint8_t euphoria_on = data[2] * 0xFF;
            handle_rumble(euphoria_on, euphoria_on);
        }
#endif
#else
        else if (id == PS3_LED_RUMBLE_ID) {
            ps3_output_report *report = (ps3_output_report *)data;
            uint8_t player = report->leds_bitmap;
            if (player & _BV(1)) {
                handle_player_leds(1);
            }
            if (player & _BV(2)) {
                handle_player_leds(2);
            }
            if (player & _BV(3)) {
                handle_player_leds(3);
            }
            if (player & _BV(4)) {
                handle_player_leds(4);
            }
        }
#endif
    }
}
long millis_since_command = 0;
uint8_t handle_serial_command(uint8_t request, uint16_t wValue, uint8_t *response_buffer, bool *success) {
    switch (request) {
        case COMMAND_READ_CONFIG: {
            if (wValue > sizeof(config)) {
                return 0;
            }
            uint16_t size = sizeof(config) - wValue;
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

        case COMMAND_READ_RF:
            response_buffer[0] = rf_connected;
            response_buffer[1] = rf_initialised;
            return 2;

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

        case COMMAND_READ_GHWT:
            if (!lastGHWTWasSuccessful) {
                return 0;
            }
            memcpy(response_buffer, &lastTap, sizeof(lastTap));
            return sizeof(lastTap);
        case COMMAND_READ_ANALOG: {
            uint8_t pin = wValue & 0xff;
            uint8_t mask = (wValue >> 8);
            uint16_t response = adc_read(pin, mask);
            memcpy(response_buffer, &response, sizeof(response));
            return sizeof(response);
        }
        case COMMAND_READ_DIGITAL: {
            uint8_t port = wValue & 0xff;
            uint8_t mask = (wValue >> 8);
            uint8_t response = digital_read(port, mask);
            memcpy(response_buffer, &response, sizeof(response));
            return sizeof(response);
        }
        case COMMAND_SET_DETECT: {
            overrideR2 = wValue > 0;
            overriddenR2 = wValue;
            return 0;
        }
#if LED_COUNT
        case COMMAND_SET_LEDS: {
            uint8_t led = response_buffer[0];
            if (led >= LED_COUNT) return 0;
            if (response_buffer[1] == 0 && response_buffer[2] == 0 && response_buffer[3] == 0) {
                ledState[led].select = 0;
                return 0;
            }
            ledState[led].select = 1;
            ledState[led].r = response_buffer[1];
            ledState[led].g = response_buffer[2];
            ledState[led].b = response_buffer[3];
            return 0;
        }
#endif
#ifdef INPUT_USB_HOST
        case COMMAND_READ_USB_HOST: {
            return read_usb_host_devices(response_buffer);
        }
#endif
#ifdef BLUETOOTH_RX
        case COMMAND_START_BT_SCAN: {
            hog_start_scan();
            return 0;
        }
        case COMMAND_STOP_BT_SCAN: {
            hog_stop_scan();
            return 0;
        }
        case COMMAND_GET_BT_DEVICES: {
            return hog_get_scan_results(response_buffer);
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