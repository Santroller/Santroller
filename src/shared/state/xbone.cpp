#include <stdint.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
extern bool hasFlags;
extern const uint8_t dpad_bindings[11];
extern const uint8_t dpad_bindings_reverse[8];
void xbone_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)data;
    usb_host_data->dpadLeft |= report->dpadLeft;
    usb_host_data->dpadRight |= report->dpadRight;
    usb_host_data->dpadUp |= report->dpadUp;
    usb_host_data->dpadDown |= report->dpadDown;
    usb_host_data->start |= report->start;
    usb_host_data->back |= report->back;
    usb_host_data->guide |= report->guide;
    usb_host_data->a |= report->a;
    usb_host_data->b |= report->b;
    usb_host_data->x |= report->x;
    usb_host_data->y |= report->y;
    switch (sub_type) {
        case GAMEPAD: {
            XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)data;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            if (report->leftTrigger) {
                usb_host_data->leftTrigger = report->leftTrigger << 6;
            }
            if (report->rightTrigger) {
                usb_host_data->rightTrigger = report->rightTrigger << 6;
            }
            if (report->leftStickX != 0x80) {
                usb_host_data->leftStickX = report->leftStickX;
            }
            if (report->leftStickY != 0x80) {
                usb_host_data->leftStickY = -report->leftStickY;
            }
            if (report->rightStickX != 0x80) {
                usb_host_data->rightStickX = report->rightStickX;
            }
            if (report->rightStickY != 0x80) {
                usb_host_data->rightStickY = -report->rightStickY;
            }
            usb_host_data->leftThumbClick |= report->leftThumbClick;
            usb_host_data->rightThumbClick |= report->rightThumbClick;
            ;
            break;
        }
        case ROCK_BAND_GUITAR: {
            XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)data;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            usb_host_data->leftShoulder |= report->leftShoulder;
            if (report->tilt) {
                usb_host_data->tilt = report->tilt << 8;
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
            switch (report->pickup) {
                case 0x00:
                    report->pickup = 0x19;
                    break;
                case 0x10:
                    report->pickup = 0x4C;
                    break;
                case 0x20:
                    report->pickup = 0x96;
                    break;
                case 0x30:
                    report->pickup = 0xB2;
                    break;
                case 0x40:
                    report->pickup = 0xE5;
                    break;
            }
            break;
        }
        case ROCK_BAND_DRUMS: {
            XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)data;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->kick1 |= report->kick1;
            usb_host_data->kick2 |= report->kick2;
            if (report->greenVelocity) {
                usb_host_data->greenVelocity = report->greenVelocity << 4;
            }
            if (usb_host_data->greenCymbalVelocity) {
                usb_host_data->greenCymbalVelocity = report->greenCymbalVelocity << 4;
            }
            if (report->redVelocity) {
                usb_host_data->redVelocity = report->redVelocity << 4;
            }
            if (report->yellowVelocity) {
                usb_host_data->yellowVelocity = report->yellowVelocity << 4;
            }
            if (usb_host_data->yellowCymbalVelocity) {
                usb_host_data->yellowCymbalVelocity = report->yellowVelocity << 4;
            }
            if (report->blueVelocity) {
                usb_host_data->blueVelocity = report->blueVelocity << 4;
            }
            if (usb_host_data->blueCymbalVelocity) {
                usb_host_data->blueCymbalVelocity = report->blueVelocity << 4;
            };
            break;
        }
    }
}

uint8_t universal_report_to_xbone(uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    XboxOneGamepad_Data_t *dpad_report = (XboxOneGamepad_Data_t *)data;
    dpad_report->dpadLeft = usb_host_data->dpadLeft;
    dpad_report->dpadRight = usb_host_data->dpadRight;
    dpad_report->dpadUp = usb_host_data->dpadUp;
    dpad_report->dpadDown = usb_host_data->dpadDown;
    dpad_report->guide = usb_host_data->guide;
    dpad_report->back = usb_host_data->back;
    dpad_report->start = usb_host_data->start;
    dpad_report->a = usb_host_data->a;
    dpad_report->b = usb_host_data->b;
    dpad_report->x = usb_host_data->x;
    dpad_report->y = usb_host_data->y;
    switch (sub_type) {
        case GAMEPAD: {
            XboxOneGamepad_Data_t *report = (XboxOneGamepad_Data_t *)data;
            report->leftShoulder = usb_host_data->leftShoulder;
            report->rightShoulder = usb_host_data->rightShoulder;
            report->leftThumbClick = usb_host_data->leftThumbClick;
            report->rightThumbClick = usb_host_data->rightThumbClick;
            report->leftTrigger = usb_host_data->leftTrigger >> 6;
            report->rightTrigger = usb_host_data->rightTrigger >> 6;
            report->leftStickX = usb_host_data->leftStickX;
            report->leftStickY = -usb_host_data->leftStickY;
            report->rightStickX = usb_host_data->rightStickX;
            report->rightStickY = -usb_host_data->rightStickY;
            return sizeof(XboxOneGamepad_Data_t);
        }
        case ROCK_BAND_GUITAR: {
            XboxOneRockBandGuitar_Data_t *report = (XboxOneRockBandGuitar_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->tilt = usb_host_data->tilt;
            report->leftShoulder |= usb_host_data->leftShoulder;
            if (report->tilt < 0x70) {
                report->tilt = 0;
            }
            report->whammy = (usb_host_data->whammy << 8) - 0x7fff;
            report->leftStickX = usb_host_data->leftStickX;
            report->leftStickY = INT16_MAX - usb_host_data->leftStickY;
            report->pickup = usb_host_data->pickup;
            report->soloGreen = usb_host_data->soloGreen;
            report->soloRed = usb_host_data->soloRed;
            report->soloYellow = usb_host_data->soloYellow;
            report->soloBlue = usb_host_data->soloBlue;
            report->soloOrange = usb_host_data->soloOrange;
            // Convert PS / 360 format to xb1 format
            report->pickup = ((usb_host_data->pickup + 0x20) >> 2) & 0xf0;
            // TODO: dpad left -> back
            return sizeof(XboxOneRockBandGuitar_Data_t);
        }
        case ROCK_BAND_DRUMS: {
            XboxOneRockBandDrums_Data_t *report = (XboxOneRockBandDrums_Data_t *)data;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->kick1 = usb_host_data->kick1;
            report->kick2 = usb_host_data->kick2;
            report->greenVelocity = usb_host_data->greenVelocity >> 4;
            report->greenCymbalVelocity = usb_host_data->greenCymbalVelocity >> 4;
            report->redVelocity = usb_host_data->redVelocity >> 4;
            report->yellowVelocity = usb_host_data->yellowVelocity >> 4;
            report->yellowCymbalVelocity = usb_host_data->yellowVelocity >> 4;
            report->blueVelocity = usb_host_data->blueVelocity >> 4;
            report->blueCymbalVelocity = usb_host_data->blueVelocity >> 4;
            return sizeof(XboxOneRockBandDrums_Data_t);
        }
    }
    return 0;
}