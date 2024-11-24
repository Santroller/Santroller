#include "config.h"
#include "defines.h"
#include "shared_main.h"
#include "endpoints.h"
#include "device_lookup.h"
void get_usb_device_type_for(uint16_t vid, uint16_t pid, uint16_t version, USB_Device_Type_t *type) {
    switch (vid) {
        case STREAM_DECK_VID: {
            type->console_type = STREAM_DECK;
            type->sub_type = UNKNOWN;
            switch (pid) {
                case STREAM_DECK_OG_PID:
                    type->sub_type = STREAM_DECK_OG;
                    break;
                case STREAM_DECK_MINI_PID:
                    type->sub_type = STREAM_DECK_MINI;
                    break;
                case STREAM_DECK_XL_PID:
                    type->sub_type = STREAM_DECK_XL;
                    break;
                case STREAM_DECK_V2_PID:
                    type->sub_type = STREAM_DECK_V2;
                    break;
                case STREAM_DECK_MK2_PID:
                    type->sub_type = STREAM_DECK_MK2;
                    break;
                case STREAM_DECK_PLUS_PID:
                    type->sub_type = STREAM_DECK_PLUS;
                    break;
                case STREAM_DECK_PEDAL_PID:
                    type->sub_type = STREAM_DECK_PEDAL;
                    break;
                case STREAM_DECK_XLV2_PID:
                    type->sub_type = STREAM_DECK_XLV2;
                    break;
                case STREAM_DECK_MINIV2_PID:
                    type->sub_type = STREAM_DECK_MINIV2;
                    break;
                case STREAM_DECK_NEO_PID:
                    type->sub_type = STREAM_DECK_NEO;
                    break;
            }
            break;
        }
        case NINTENDO_VID: {
            if (pid == SWITCH_PRO_PID) {
                type->console_type = SWITCH;
                type->sub_type = GAMEPAD;
            }
            break;
        }
        case ARDWIINO_VID: {
            if (pid == ARDWIINO_PID) {
                type->console_type = SANTROLLER;
                type->sub_type = (version >> 8) & 0xFF;
            }
            break;
        }
        case RAPHNET_VID: {
            type->console_type = RAPHNET;
            type->sub_type = GAMEPAD;
            break;
        }
        case MAGICBOOTS_PS4_VID: {
            if (pid == MAGICBOOTS_PS4_PID) {
                type->console_type = PS4;
                type->sub_type = GAMEPAD;
            }
            break;
        }
        case ARDUINO_VID: {
            if (pid == STEPMANIA_X_PID) {
                type->console_type = STEPMANIAX;
                type->sub_type = DANCE_PAD;
            }
            break;
        }
        case LTEK_LUFA_VID: {
            if (pid == LTEK_LUFA_PID) {
                type->console_type = LTEK_ID;
                type->sub_type = DANCE_PAD;
            }
            break;
        }
        case LTEK_VID: {
            if (pid == LTEK_PID) {
                type->console_type = LTEK;
                type->sub_type = DANCE_PAD;
            }
            break;
        }
        case SONY_VID:
            switch (pid) {
                case SONY_DS3_PID:
                    type->console_type = PS3;
                    type->sub_type = GAMEPAD;
                    break;
                case PS4_DS_PID_1:
                case PS4_DS_PID_2:
                case PS4_DS_PID_3:
                    type->console_type = PS4;
                    type->sub_type = GAMEPAD;
                    break;
                case PS5_DS_PID:
                    type->console_type = PS5;
                    type->sub_type = GAMEPAD;
                    break;
            }
            break;
        case REDOCTANE_VID:
            switch (pid) {
                case PS3_GH_GUITAR_PID:
                    type->console_type = PS3;
                    type->sub_type = GUITAR_HERO_GUITAR;
                    break;
                case PS3_GH_DRUM_PID:
                    type->console_type = PS3;
                    type->sub_type = GUITAR_HERO_DRUMS;
                    break;
                case PS3_RB_GUITAR_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case PS3_RB_DRUM_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_DRUMS;
                    break;
                case PS3_DJ_TURNTABLE_PID:
                    type->console_type = PS3;
                    type->sub_type = DJ_HERO_TURNTABLE;
                    break;
                case PS3WIIU_GHLIVE_DONGLE_PID:
                    type->console_type = PS3;
                    type->sub_type = LIVE_GUITAR;
                    break;
                case PS3_MPA_KEYBOARD_PID:
                case PS3_KEYBOARD_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_KEYS;
                    break;
                case PS3_MUSTANG_PID:
                case PS3_MPA_MUSTANG_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_MUSTANG;
                    break;
                case PS3_SQUIRE_PID:
                case PS3_MPA_SQUIRE_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_SQUIRE;
            }
            break;

        case HARMONIX_VID:
            // Polled the same as PS3, so treat them as PS3 instruments
            switch (pid) {
                case WII_RB_GUITAR_PID:
                case WII_RB_GUITAR_2_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;

                case WII_RB_DRUM_PID:
                case WII_RB_DRUM_2_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_DRUMS;
                    break;
                case WII_KEYBOARD_PID:
                case WII_MPA_KEYBOARD_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_KEYS;
                    break;
                case WII_MUSTANG_PID:
                case WII_MPA_MUSTANG_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_MUSTANG;
                case WII_SQUIRE_PID:
                case WII_MPA_SQUIRE_PID:
                    type->console_type = PS3;
                    type->sub_type = ROCK_BAND_PRO_GUITAR_SQUIRE;
                    break;
            }

            break;
        case PDP_VID:
            switch (pid) {
                case XBOX_ONE_JAG_PID:
                case XBOX_ONE_RIFFMASTER_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case PS4_JAG_PID:
                case PS4_RIFFMASTER_PID:
                    type->console_type = PS4;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case PS5_RIFFMASTER_PID:
                    type->console_type = PS5;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
            }

            break;

        case MAD_CATZ_VID:
            switch (pid) {
                case XBOX_ONE_RB_GUITAR_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
                case XBOX_ONE_RB_DRUM_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = ROCK_BAND_DRUMS;
                    break;
                case PS4_STRAT_PID:
                    type->console_type = PS4;
                    type->sub_type = ROCK_BAND_GUITAR;
                    break;
            }

            break;

        case XBOX_REDOCTANE_VID:
            switch (pid) {
                case XBOX_ONE_GHLIVE_DONGLE_PID:
                    type->console_type = XBOXONE;
                    type->sub_type = LIVE_GUITAR;
                    break;
                case XBOX_360_GHLIVE_DONGLE_PID:
                    type->console_type = XBOX360;
                    type->sub_type = XINPUT_GUITAR_HERO_LIVE;
                    break;
                case XBOX_360_WT_KIOSK_PID:
                    type->console_type = XBOX360;
                    type->sub_type = XINPUT_GUITAR_WT;
                    break;
            }
            break;
    }
}