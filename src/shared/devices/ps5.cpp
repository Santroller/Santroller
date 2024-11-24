#include <stdint.h>

#include "Usb.h"
#include "config.h"
#include "defines.h"
#include "ids.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
void ps5_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    PS4Dpad_Data_t *report = (PS4Dpad_Data_t *)data;
    DPAD_REV();
    switch (sub_type) {
        case GAMEPAD: {
            PS4Gamepad_Data_t *report = (PS4Gamepad_Data_t *)data;
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
            PS4RockBandGuitar_Data_t *report = (PS4RockBandGuitar_Data_t *)data;
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
        case LIVE_GUITAR: {
            PS4GHLGuitar_Data_t *report = (PS4GHLGuitar_Data_t *)data;
            usb_host_data->black1 |= report->black1;
            usb_host_data->black2 |= report->black2;
            usb_host_data->black3 |= report->black3;
            usb_host_data->white1 |= report->white1;
            usb_host_data->white2 |= report->white2;
            usb_host_data->white3 |= report->white3;
            usb_host_data->ghtv |= report->ghtv;
            if (report->dpadUp) {
                usb_host_data->leftStickY = 0x00;
            }
            if (report->dpadDown) {
                usb_host_data->leftStickY = 0xFF;
            }
            if (report->tilt) {
                usb_host_data->tilt = (-((report->tilt - 0x0200) + 40)) << 7;
            }
            if (report->whammy) {
                usb_host_data->whammy = report->whammy;
            };
            break;
        }
    }
}