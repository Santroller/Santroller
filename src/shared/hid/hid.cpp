#include "hid.h"

#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "config.h"
#include "controllers.h"
#include "keyboard_mouse.h"
#include "pins.h"
#include "ps3_wii_switch.h"
#include "rf.h"
#include "stdint.h"
#include "util.h"

const PROGMEM char board[] = ARDWIINO_BOARD;
const PROGMEM char f_cpu_descriptor_str[] = STR(F_CPU);
const PROGMEM uint8_t version[] = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION};
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
uint8_t stage_kit_millis[] = {
    150,  // Slow
    125,  // Medium
    100,  // Fast
    75,   // Fastest
};
uint8_t strobe_delay = 0;
bool strobing = false;
long last_strobe = 0;
#ifdef RF_RX
void handle_auth_led(void) {
    RfAuthLedPacket_t packet = {
        AckAuthLed};
    nrfRadio.addAckData(&packet, sizeof(packet));
}
void handle_player_leds(uint8_t player) {
    RfPlayerLed_t packet = {
        AckPlayerLed,
        player};
    nrfRadio.addAckData(&packet, sizeof(packet));
}
void handle_rumble(uint8_t rumble_left, uint8_t rumble_right) {
    RfRumbleLed_t packet = {
        AckRumble,
        rumble_left,
        rumble_right};
    nrfRadio.addAckData(&packet, sizeof(packet));
}
void handle_keyboard_leds(uint8_t leds) {
    RfRumbleLed_t packet = {
        AckKeyboardLed,
        leds};
    nrfRadio.addAckData(&packet, sizeof(packet));
}
#else
void handle_auth_led(void) {
    HANDLE_AUTH_LED;
}
void handle_player_leds(uint8_t player) {
    HANDLE_PLAYER_LED;
}
void handle_rumble(uint8_t rumble_left, uint8_t rumble_right) {
    HANDLE_RUMBLE;
}
void handle_keyboard_leds(uint8_t leds) {
    HANDLE_KEYBOARD_LED;
}
#endif
void hid_set_report(const uint8_t *data, uint8_t len, uint8_t reportType, uint8_t report_id) {
#if CONSOLE_TYPE == KEYBOARD_MOUSE
    handle_keyboard_leds(data[0]);
#endif
    uint8_t id = data[0];
    // Handle Xbox 360 LEDs and rumble
    if (report_id == INTERRUPT_ID) {
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
                    printf("Ready!\n");
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
                    if (sub_id == PS3_LED_ID) {
                        uint8_t player = (data[3] >> 2);
                        handle_player_leds(player);
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
        } else {
            while (len) {
                uint8_t size = data[1];
                len -= size;
                if (id == XBOX_LED_ID) {
                    uint8_t led = data[2];
                    uint8_t player = xbox_players[led];
                    handle_player_leds(player);
                } else if (id == XBOX_RUMBLE_ID) {
                    uint8_t rumble_left = data[3];
                    uint8_t rumble_right = data[4];
                    handle_rumble(rumble_left, rumble_right);
                }
            }
            data += len;
        }
    } else {
        uint8_t *data = (uint8_t *)data;
        if (id == PS3_LED_ID) {
            uint8_t player = (data[2] >> 2);
            handle_player_leds(player);
#if DEVICE_TYPE == DJ_HERO_TURNTABLE
        } else if (id == DJ_LED_ID) {
            uint8_t euphoria_on = data[2] * 0xFF;
            handle_rumble(euphoria_on, euphoria_on);
#endif
        } else if (id == SANTROLLER_PS3_RUMBLE_ID) {
            uint8_t rumble_left = data[3];
            uint8_t rumble_right = data[4];
            handle_rumble(rumble_left, rumble_right);
        } else if (id == COMMAND_SET_DETECT) {
            uint8_t enabled = data[2];
            uint8_t r2_value = data[3];
            overrideR2 = enabled > 0;
            overriddenR2 = r2_value;
        }
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
        case COMMAND_READ_BOARD:
            memcpy_P(response_buffer, board, sizeof(board));
            return sizeof(board);
        case COMMAND_READ_VERSION: {
            memcpy_P(response_buffer, version, sizeof(version));
            return sizeof(version);
        }
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
    }
    *success = false;
    return 0;
}

uint8_t hid_get_report(uint8_t *data, uint8_t reqlen, uint8_t reportType, uint8_t report_id) {
    return 0;
}