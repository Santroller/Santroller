#include <stdint.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
inline void ps2_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    if (sub_type == PSX_GUITAR_HERO_CONTROLLER) {
        PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)data;
        usb_host_data->start |= report->start;
        usb_host_data->back |= report->back;
        usb_host_data->dpadDown |= report->dpadDown | (((report->dpad >> 6) == 2) && report->dpad != 128);
        usb_host_data->dpadRight |= (((report->dpad >> 6) == 3) && report->dpad != 255);
        usb_host_data->dpadUp |= report->dpadUp | ((report->dpad >> 6) == 0);
        usb_host_data->dpadLeft |= (((report->dpad >> 6) == 1) && report->dpad != 127);
        usb_host_data->green |= report->green;
        usb_host_data->red |= report->red;
        usb_host_data->yellow |= report->yellow;
        usb_host_data->blue |= report->blue;
        usb_host_data->orange |= report->orange;
        usb_host_data->whammy = (-(report->whammy - 127)) << 9;
        // WT -> GH5
        if (report->slider < 0x2F) {
            usb_host_data->slider = 0x15;
        } else if (report->slider < 0x3F) {
            usb_host_data->slider = 0x30;
        } else if (report->slider < 0x5F) {
            usb_host_data->slider = 0x4D;
        } else if (report->slider < 0x6F) {
            usb_host_data->slider = 0x66;
        } else if (report->slider < 0x8F) {
            usb_host_data->slider = 0x80;
        } else if (report->slider < 0x9F) {
            usb_host_data->slider = 0x9A;
        } else if (report->slider < 0xAF) {
            usb_host_data->slider = 0xAF;
        } else if (report->slider < 0xCF) {
            usb_host_data->slider = 0xC9;
        } else if (report->slider < 0xEF) {
            usb_host_data->slider = 0xE6;
        } else {
            usb_host_data->slider = 0xFF;
        }
        return;
    }
    PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)data;
    if (sub_type == PSX_DUALSHOCK_2_CONTROLLER) {
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
        if (report->pressureDpadUp) {
            usb_host_data->pressureDpadUp = report->pressureDpadUp;
        }
        if (report->pressureDpadRight) {
            usb_host_data->pressureDpadRight = report->pressureDpadRight;
        }
        if (report->pressureDpadDown) {
            usb_host_data->pressureDpadDown = report->pressureDpadDown;
        }
        if (report->pressureDpadLeft) {
            usb_host_data->pressureDpadLeft = report->pressureDpadLeft;
        }
        if (report->pressureL1) {
            usb_host_data->pressureL1 = report->pressureL1;
        }
        if (report->pressureR1) {
            usb_host_data->pressureR1 = report->pressureR1;
        }
        if (report->pressureTriangle) {
            usb_host_data->pressureTriangle = report->pressureTriangle;
        }
        if (report->pressureCircle) {
            usb_host_data->pressureCircle = report->pressureCircle;
        }
        if (report->pressureCross) {
            usb_host_data->pressureCross = report->pressureCross;
        }
        if (report->pressureSquare) {
            usb_host_data->pressureSquare = report->pressureSquare;
        };
        return;
    }
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
    if (sub_type == PSX_NEGCON) {
        usb_host_data->leftStickX = 0;
        usb_host_data->leftStickY = 0;
        usb_host_data->rightStickX = 0;
        usb_host_data->rightStickY = 0;
        usb_host_data->leftTrigger = 0;
        usb_host_data->rightTrigger = 0;
        // Map the twist axis to X axis of left analog
        usb_host_data->leftStickX = report->rightStickX;
        // Map analog button data to their reasonable counterparts
        usb_host_data->pressureCross = report->rightStickY;
        usb_host_data->pressureSquare = report->leftStickX;
        usb_host_data->pressureL1 = report->leftStickY;
        // Make up "missing" digital data
        usb_host_data->x |= usb_host_data->pressureSquare >= 128;
        usb_host_data->a |= usb_host_data->pressureCross >= 128;
        usb_host_data->leftShoulder |= usb_host_data->pressureL1 >= 240;
    }
    if (sub_type == PSX_JOGCON) {
        usb_host_data->leftStickX = 0;
        usb_host_data->leftStickY = 0;
        usb_host_data->rightStickX = 0;
        usb_host_data->rightStickY = 0;
        usb_host_data->leftTrigger = 0;
        usb_host_data->rightTrigger = 0;
        /* Map the wheel X axis of left analog, half a rotation
         * per direction: byte 5 has the wheel position, it is
         * 0 at startup, then we have 0xFF down to 0x80 for
         * left/CCW, and 0x01 up to 0x80 for right/CW
         *
         * byte 6 is the number of full CW rotations
         * byte 7 is 0 if wheel is still, 1 if it is rotating CW
         *        and 2 if rotation CCW
         * byte 8 seems to stay at 0
         *
         * We'll want to cap the movement halfway in each
         * direction, for ease of use/implementation.
         */
        if (report->rightStickY < 0x80) {
            // CW up to half
            usb_host_data->leftStickX = report->rightStickX < 0x80 ? report->rightStickX : (0x80 - 1);
        } else {
            // CCW down to half
            usb_host_data->leftStickX = report->rightStickX > 0x80 ? report->rightStickX : (0x80 + 1);
        }

        usb_host_data->leftStickX += 0x80;
    }
}

inline uint8_t universal_report_to_ps2(uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    switch (sub_type) {
        case GUITAR_HERO_GUITAR: {
            PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)data;
            report->header = 0x5A;
            report->dpadLeft = true;
            report->dpadDown = usb_host_data->dpadDown;
            report->dpadUp = usb_host_data->dpadUp;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->kick1 = usb_host_data->kick1;
            report->tilt = usb_host_data->tilt > 20000;
            report->whammy = 0x7f - (usb_host_data->whammy >> 1);
            report->leftStickX = (usb_host_data->leftStickX >> 8) + 0x80;
            report->leftStickY = (-usb_host_data->leftStickY >> 8) + 0x80;
            report->slider = usb_host_data->slider;
            return sizeof(PS2GuitarHeroGuitar_Data_t);
        }
        default: {
            PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)data;
            report->header = 0x5A;
            report->dpadLeft = usb_host_data->dpadLeft;
            report->dpadRight = usb_host_data->dpadRight;
            report->dpadUp = usb_host_data->dpadUp;
            report->dpadDown = usb_host_data->dpadDown;
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
            report->pressureDpadUp = usb_host_data->pressureDpadUp;
            report->pressureDpadRight = usb_host_data->pressureDpadRight;
            report->pressureDpadDown = usb_host_data->pressureDpadDown;
            report->pressureDpadLeft = usb_host_data->pressureDpadLeft;
            report->pressureL1 = usb_host_data->pressureL1;
            report->pressureR1 = usb_host_data->pressureR1;
            report->pressureTriangle = usb_host_data->pressureTriangle;
            report->pressureCircle = usb_host_data->pressureCircle;
            report->pressureCross = usb_host_data->pressureCross;
            report->pressureSquare = usb_host_data->pressureSquare;
            return sizeof(PS2Gamepad_Data_t);
        }
    }
}