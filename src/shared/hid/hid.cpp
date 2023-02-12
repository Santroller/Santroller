#include "hid.h"

#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "controllers.h"
#include "keyboard_mouse.h"
#include "ps3_wii_switch.h"
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
uint8_t strobe_delay = 0;
bool strobing = false;
long last_strobe = 0;
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

void hid_set_report(const uint8_t *data, uint8_t len, uint8_t reportType, uint8_t report_id) {
    #if CONSOLE_TYPE == KEYBOARD_MOUSE
        handle_keyboard_leds(data[0]);  
    #endif
    uint8_t id = data[0];
    // Handle Xbox 360 LEDs and rumble
    if (report_id == INTERRUPT_ID) {
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
        // Handle XBOX One Auth
    } else if (consoleType == XBOXONE) {
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

uint8_t hid_get_report(uint8_t *data, uint8_t reqlen, uint8_t reportType, uint8_t report_id) {
    return 0;
}