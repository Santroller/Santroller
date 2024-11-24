#include <stdint.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
extern bool hasFlags;
extern const uint8_t dpad_bindings[11];
extern const uint8_t dpad_bindings_reverse[8];
void ps4_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)data;
    DPAD_REV();
    switch (sub_type) {
        case GAMEPAD: {
            PS5Gamepad_Data_t *report = (PS5Gamepad_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            if (report->leftTrigger) {
                usb_host_data->leftTrigger = report->leftTrigger << 8;
            }
            if (report->rightTrigger) {
                usb_host_data->rightTrigger = report->rightTrigger << 8;
            }
            if (report->leftStickX != 0x80) {
                usb_host_data->leftStickX = (report->leftStickX - 0x80) << 8;
            }
            if (report->leftStickY != 0x80) {
                usb_host_data->leftStickY = (-report->leftStickY - 0x80) << 8;
            }
            if (report->rightStickX != 0x80) {
                usb_host_data->rightStickX = (report->rightStickX - 0x80) << 8;
            }
            if (report->rightStickY != 0x80) {
                usb_host_data->rightStickY = (-report->rightStickY - 0x80) << 8;
            }
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            ;
            break;
        }
        case ROCK_BAND_GUITAR: {
            PS5RockBandGuitar_Data_t *report = (PS5RockBandGuitar_Data_t *)data;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            if (report->tilt) {
                usb_host_data->tilt = (report->tilt - 0x80) << 8;
            }
            if (report->whammy) {
                usb_host_data->whammy = (report->whammy - 0x80) << 8;
            }
            if (report->leftStickX) {
                usb_host_data->leftStickX = (report->leftStickX - 0x80) << 8;
            }
            if (report->leftStickY) {
                usb_host_data->leftStickY = (-report->leftStickY - 0x80) << 8;
            }
            if (report->pickup) {
                usb_host_data->pickup = report->pickup;
            };
            usb_host_data->soloGreen |= report->soloGreen;
            usb_host_data->soloRed |= report->soloRed;
            usb_host_data->soloYellow |= report->soloYellow;
            usb_host_data->soloBlue |= report->soloBlue;
            usb_host_data->soloOrange |= report->soloOrange;
            solo_to_slider(usb_host_data);
            break;
        }
    }
}

uint8_t universal_report_to_ps4(uint8_t dpad, uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    PS4Dpad_Data_t *dpad_report = (PS4Dpad_Data_t *)data;
    dpad_report->dpad = dpad;
    dpad_report->guide = usb_host_data->guide;
    dpad_report->back = usb_host_data->back;
    dpad_report->start = usb_host_data->start;
    switch (sub_type) {
        case GAMEPAD: {
            PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)data;
            report->a = usb_host_data->a;
            report->b = usb_host_data->b;
            report->x = usb_host_data->x;
            report->y = usb_host_data->y;
            report->leftShoulder = usb_host_data->leftShoulder;
            report->rightShoulder = usb_host_data->rightShoulder;
            report->leftTrigger = usb_host_data->leftTrigger >> 8;
            report->rightTrigger = usb_host_data->rightTrigger >> 8;
            report->leftStickX = (usb_host_data->leftStickX >> 8) + 0x80;
            report->leftStickY = (-usb_host_data->leftStickY >> 8) + 0x80;
            report->rightStickX = (usb_host_data->rightStickX >> 8) + 0x80;
            report->rightStickY = (-usb_host_data->rightStickY >> 8) + 0x80;
            report->leftThumbClick = usb_host_data->leftThumbClick;
            report->rightThumbClick = usb_host_data->rightThumbClick;
            return sizeof(PS4Gamepad_Data_t);
        }
        case ROCK_BAND_GUITAR: {
            PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->tilt = (usb_host_data->tilt >> 8) + 0x80;
            report->whammy = (usb_host_data->whammy >> 8) + 0x80;
            report->leftStickX = (usb_host_data->leftStickX >> 8) + 0x80;
            report->leftStickY = (-usb_host_data->leftStickY >> 8) + 0x80;
            report->pickup = usb_host_data->pickup;
            report->soloGreen = usb_host_data->soloGreen;
            report->soloRed = usb_host_data->soloRed;
            report->soloYellow = usb_host_data->soloYellow;
            report->soloBlue = usb_host_data->soloBlue;
            report->soloOrange = usb_host_data->soloOrange;
            // TODO: dpad left -> back
            return sizeof(PS4RockBandGuitar_Data_t);
        }
        case LIVE_GUITAR: {
            PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)data;
            report->black1 = usb_host_data->black1;
            report->black2 = usb_host_data->black2;
            report->black3 = usb_host_data->black3;
            report->white1 = usb_host_data->white1;
            report->white2 = usb_host_data->white2;
            report->white3 = usb_host_data->white3;
            report->ghtv = usb_host_data->ghtv;
            if (usb_host_data->dpadUp) {
                report->leftStickY = 0x00;
            }
            if (usb_host_data->dpadDown) {
                report->leftStickY = 0xFF;
            }
            report->tilt = 0x0200 + ((-(usb_host_data->tilt >> 7)) - 40);
            report->whammy = usb_host_data->whammy;
            return sizeof(PS4GHLGuitar_Data_t);
        }
    }
    return 0;
}