#include <stdint.h>
#include <string.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
#include "state_translation/wii.h"

extern uint8_t drumVelocity[8];
extern bool hasTapBar;
void wii_to_universal_report(const uint8_t *data, uint8_t len, uint16_t controllerType, bool hiRes, USB_Host_Data_t *usb_host_data) {
    switch (controllerType) {
        case WII_NUNCHUK: {
            WiiNunchukDataFormat_t *report = (WiiNunchukDataFormat_t *)data;
            usb_host_data->accelX = (report->accelX10 | report->accelX92 << 2) - 511;
            usb_host_data->accelY = (report->accelY10 | report->accelY92 << 2) - 511;
            usb_host_data->accelZ = (report->accelZ10 | report->accelZ92 << 2) - 511;
            usb_host_data->nunchukC = !report->c;
            usb_host_data->nunchukZ = !report->z;
            break;
        }
        case WII_CLASSIC_CONTROLLER: {
            WiiClassicButtons_t *buttons;
            if (hiRes) {
                WiiClassicDataFormat3_t *report = (WiiClassicDataFormat3_t *)data;
                usb_host_data->leftStickX = (report->leftStickX << 8) - INT16_MAX;
                usb_host_data->leftStickY = (report->leftStickY << 8) - INT16_MAX;
                usb_host_data->rightStickX = (report->rightStickX << 8) - INT16_MAX;
                usb_host_data->rightStickY = (report->rightStickY << 8) - INT16_MAX;
                usb_host_data->leftTrigger = report->leftTrigger << 8;
                usb_host_data->rightTrigger = report->rightTrigger << 8;
                buttons = &report->buttons;
            } else {
                WiiClassicDataFormat1_t *report = (WiiClassicDataFormat1_t *)data;
                usb_host_data->leftStickX = (report->leftStickX << 11) - INT16_MAX;
                usb_host_data->leftStickY = (report->leftStickY << 11) - INT16_MAX;
                usb_host_data->rightStickX = ((report->rightStickX0 | report->rightStickX21 << 1 | report->rightStickX43 << 3) << 12) - INT16_MAX;
                usb_host_data->rightStickY = (report->rightStickY << 12) - INT16_MAX;
                usb_host_data->leftTrigger = (report->leftTrigger20 | report->leftTrigger43 << 3) << 12;
                usb_host_data->rightTrigger = report->rightTrigger << 12;
                usb_host_data->leftShoulder = report->buttons.leftShoulder ? UINT16_MAX : 0;
                usb_host_data->rightShoulder = report->buttons.rightShoulder ? UINT16_MAX : 0;
                buttons = &report->buttons;
            }
            usb_host_data->start = !buttons->start;
            usb_host_data->guide = !buttons->guide;
            usb_host_data->back = !buttons->back;
            usb_host_data->dpadDown = !buttons->dpadDown;
            usb_host_data->dpadRight = !buttons->dpadRight;
            usb_host_data->dpadUp = !buttons->dpadUp;
            usb_host_data->dpadLeft = !buttons->dpadLeft;
            usb_host_data->a = !buttons->a;
            usb_host_data->b = !buttons->b;
            usb_host_data->x = !buttons->x;
            usb_host_data->y = !buttons->y;
            break;
        }
        case WII_THQ_UDRAW_TABLET: {
            WiiUDrawDataFormat_t *report = (WiiUDrawDataFormat_t *)data;
            usb_host_data->mouse.x = ((report->x70 | report->x118 << 8) << 5) + INT16_MAX;
            usb_host_data->mouse.y = ((report->y70 | report->y118 << 8) << 5) + INT16_MAX;
            usb_host_data->leftTrigger = (report->pressure70 | report->pressure8 << 8) << 7;
            usb_host_data->a = !report->up;
            usb_host_data->b = !report->down;
            break;
        }
        case WII_UBISOFT_DRAWSOME_TABLET: {
            WiiDrawsomeDataFormat_t *report = (WiiDrawsomeDataFormat_t *)data;
            usb_host_data->leftStickX = report->x + INT16_MAX;
            usb_host_data->leftStickY = report->y + INT16_MAX;
            usb_host_data->leftTrigger = report->pressure << 8;
            usb_host_data->a = !report->up;
            usb_host_data->b = !report->down;
            break;
        }
        case WII_GUITAR_HERO_GUITAR_CONTROLLER: {
            WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)data;
            usb_host_data->leftStickX = (report->leftStickX << 10) - INT16_MAX;
            usb_host_data->leftStickY = (report->leftStickY << 10) - INT16_MAX;
            usb_host_data->whammy = report->whammy << 3;
            if (!hasTapBar) {
                // This will only ever be set on guitars that support the slider
                if (report->slider == 0x0F) {
                    hasTapBar = true;
                }
                usb_host_data->slider = 0x80;
            } else if (report->slider == 0x0f) {
                usb_host_data->slider = 0x80;
            } else if (report->slider < 0x05) {
                usb_host_data->slider = 0x15;
            } else if (report->slider < 0x0A) {
                usb_host_data->slider = 0x30;
            } else if (report->slider < 0x0C) {
                usb_host_data->slider = 0x4D;
            } else if (report->slider < 0x12) {
                usb_host_data->slider = 0x66;
            } else if (report->slider < 0x14) {
                usb_host_data->slider = 0x9A;
            } else if (report->slider < 0x17) {
                usb_host_data->slider = 0xAF;
            } else if (report->slider < 0x1A) {
                usb_host_data->slider = 0xC9;
            } else if (report->slider < 0x1F) {
                usb_host_data->slider = 0xE6;
            } else {
                usb_host_data->slider = 0xFF;
            }
            slider_to_solo(usb_host_data);

            usb_host_data->start = !report->start;
            usb_host_data->guide = !report->guide;
            usb_host_data->back = !report->back;
            usb_host_data->dpadDown = !report->dpadDown;
            usb_host_data->dpadRight = !report->dpadRight;
            usb_host_data->dpadUp = !report->dpadUp;
            usb_host_data->dpadLeft = !report->dpadLeft;
            usb_host_data->green = !report->green;
            usb_host_data->red = !report->red;
            usb_host_data->yellow = !report->yellow;
            usb_host_data->blue = !report->blue;
            usb_host_data->orange = !report->orange;
            break;
        }
        case WII_GUITAR_HERO_DRUM_CONTROLLER: {
            WiiDrumDataFormat3_t *report = (WiiDrumDataFormat3_t *)data;

            usb_host_data->leftStickX = (report->leftStickX << 10) - INT16_MAX;
            usb_host_data->leftStickY = (report->leftStickY << 10) - INT16_MAX;

            usb_host_data->start = !report->start;
            usb_host_data->guide = !report->guide;
            usb_host_data->back = !report->back;
            // The analog values respond faster than the digital ones, so its best to use them as well
            usb_host_data->green = !report->green || drumVelocity[DRUM_GREEN];
            usb_host_data->red = !report->red || drumVelocity[DRUM_RED];
            usb_host_data->yellow = !report->yellow || drumVelocity[DRUM_YELLOW];
            usb_host_data->blue = !report->blue || drumVelocity[DRUM_BLUE];
            usb_host_data->orange = !report->orange || drumVelocity[DRUM_ORANGE];
            //
            uint8_t velocity = (report->velocity0) | (report->velocity1 << 1) | (report->velocity2 << 2) | (report->velocity3 << 3) | (report->velocity64 << 4);
            switch (~report->note) {
                case MIDI_DRUM_KICK:
                    drumVelocity[DRUM_KICK] = velocity;
                    break;
                case MIDI_DRUM_GREEN:
                    drumVelocity[DRUM_GREEN] = velocity;
                    break;
                case MIDI_DRUM_RED:
                    drumVelocity[DRUM_RED] = velocity;
                    break;
                case MIDI_DRUM_YELLOW:
                    drumVelocity[DRUM_YELLOW] = velocity;
                    break;
                case MIDI_DRUM_BLUE:
                    drumVelocity[DRUM_BLUE] = velocity;
                    break;
                case MIDI_DRUM_ORANGE:
                    drumVelocity[DRUM_ORANGE] = velocity;
                    break;
                case MIDI_DRUM_HI_HAT:
                    drumVelocity[DRUM_HIHAT] = velocity;
                    break;
            }
            // We only get velocity on events above, so zero them when the digital input is off
            if (!report->green) {
                drumVelocity[DRUM_GREEN] = 0;
            }
            if (!report->red) {
                drumVelocity[DRUM_RED] = 0;
            }
            if (!report->yellow) {
                drumVelocity[DRUM_YELLOW] = 0;
            }
            if (!report->blue) {
                drumVelocity[DRUM_BLUE] = 0;
            }
            if (!report->orange) {
                drumVelocity[DRUM_ORANGE] = 0;
            }
            if (!report->kick1) {
                drumVelocity[DRUM_KICK] = 0;
            }
            break;
        }
        case WII_DJ_HERO_TURNTABLE: {
            WiiTurntableDataFormat3_t *report = (WiiTurntableDataFormat3_t *)data;
            usb_host_data->leftStickX = (report->leftStickX << 10) - INT16_MAX;
            usb_host_data->leftStickY = (report->leftStickY << 10) - INT16_MAX;
            ltt_t ltt;
            rtt_t rtt;
            ltt.ltt40 = report->leftTableVelocity40;
            ltt.ltt5 = report->leftTableVelocity5;
            rtt.rtt0 = report->rightTableVelocity0;
            rtt.rtt21 = report->rightTableVelocity21;
            rtt.rtt43 = report->rightTableVelocity43;
            rtt.rtt5 = report->rightTableVelocity5;
            usb_host_data->leftTableVelocity = ltt.ltt << 10;
            usb_host_data->rightTableVelocity = rtt.rtt << 10;
            usb_host_data->start = !report->start;
            usb_host_data->guide = !report->guide;
            usb_host_data->back = !report->back;
            usb_host_data->dpadDown = !report->dpadDown;
            usb_host_data->dpadRight = !report->dpadRight;
            usb_host_data->dpadUp = !report->dpadUp;
            usb_host_data->dpadLeft = !report->dpadLeft;
            usb_host_data->leftGreen = !report->leftGreen;
            usb_host_data->leftRed = !report->leftRed;
            usb_host_data->leftBlue = !report->leftBlue;
            usb_host_data->rightGreen = !report->rightGreen;
            usb_host_data->rightRed = !report->rightRed;
            usb_host_data->rightBlue = !report->rightBlue;
            break;
        }
        case WII_TAIKO_NO_TATSUJIN_CONTROLLER: {
            WiiTaTaConDataFormat_t *report = (WiiTaTaConDataFormat_t *)data;
            usb_host_data->green = !report->centerLeft;
            usb_host_data->red = !report->rimLeft;
            usb_host_data->yellow = !report->centerRight;
            usb_host_data->blue = !report->rimRight;
            break;
        }
    }
}

uint8_t universal_report_to_wii(uint8_t *data, uint8_t sub_type, uint8_t format, const USB_Host_Data_t *usb_host_data) {
    switch (sub_type) {
        case GUITAR_HERO_GUITAR: {
            WiiGuitarDataFormat3_t *report = (WiiGuitarDataFormat3_t *)data;
            memset(data, 0, sizeof(WiiGuitarDataFormat3_t));
            report->dpadUp = usb_host_data->dpadUp;
            report->dpadDown = usb_host_data->dpadDown;
            report->leftStickX = usb_host_data->leftStickX >> 2;
            report->leftStickY = usb_host_data->leftStickY >> 2;
            report->slider = 0x0F;
            if ((usb_host_data->slider != PS3_STICK_CENTER)) {
                uint8_t slider_tmp = usb_host_data->slider;
                if (slider_tmp <= 0x15) {
                    report->slider = 0x04;
                } else if (slider_tmp <= 0x30) {
                    report->slider = 0x07;
                } else if (slider_tmp <= 0x4D) {
                    report->slider = 0x0a;
                } else if (slider_tmp <= 0x66) {
                    report->slider = 0x0c;
                } else if (slider_tmp <= 0x9A) {
                    report->slider = 0x12;
                } else if (slider_tmp <= 0xAF) {
                    report->slider = 0x14;
                } else if (slider_tmp <= 0xC9) {
                    report->slider = 0x17;
                } else if (slider_tmp <= 0xE6) {
                    report->slider = 0x1A;
                } else {
                    report->slider = 0x1F;
                }
            }
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            report->whammy = usb_host_data->whammy >> 3;
            data[4] = ~data[4];
            data[5] = ~data[5];
            return sizeof(WiiGuitarDataFormat3_t);
        }
        case GUITAR_HERO_DRUMS: {
            WiiDrumDataFormat3_t *report = (WiiDrumDataFormat3_t *)data;
            memset(data, 0, sizeof(WiiDrumDataFormat3_t));
            report->leftStickX = usb_host_data->leftStickX >> 2;
            report->leftStickY = usb_host_data->leftStickY >> 2;
            report->green = usb_host_data->green;
            report->red = usb_host_data->red;
            report->yellow = usb_host_data->yellow;
            report->blue = usb_host_data->blue;
            report->orange = usb_host_data->orange;
            // TODO: velocity
            data[4] = ~data[4];
            data[5] = ~data[5];
            return sizeof(WiiDrumDataFormat3_t);
        }
        case DJ_HERO_TURNTABLE: {
            WiiTurntableDataFormat3_t *report = (WiiTurntableDataFormat3_t *)data;
            memset(data, 0, sizeof(WiiTurntableDataFormat3_t));
            report->leftStickX = (usb_host_data->leftStickX + INT16_MAX) >> 10;
            report->leftStickY = (usb_host_data->leftStickY + INT16_MAX) >> 10;
            ltt_t ltt;
            rtt_t rtt;
            ltt.ltt = usb_host_data->leftTableVelocity >> 10;
            rtt.rtt = usb_host_data->rightTableVelocity >> 10;
            report->leftTableVelocity40 = ltt.ltt40;
            report->leftTableVelocity5 = ltt.ltt5;
            report->rightTableVelocity0 = rtt.rtt0;
            report->rightTableVelocity21 = rtt.rtt21;
            report->rightTableVelocity43 = rtt.rtt43;
            report->rightTableVelocity5 = rtt.rtt5;
            report->start = usb_host_data->start;
            report->guide = usb_host_data->guide;
            report->back = usb_host_data->back;
            report->dpadDown = usb_host_data->dpadDown;
            report->dpadRight = usb_host_data->dpadRight;
            report->dpadUp = usb_host_data->dpadUp;
            report->dpadLeft = usb_host_data->dpadLeft;
            report->leftGreen = usb_host_data->leftGreen;
            report->leftRed = usb_host_data->leftRed;
            report->leftBlue = usb_host_data->leftBlue;
            report->rightGreen = usb_host_data->rightGreen;
            report->rightRed = usb_host_data->rightRed;
            report->rightBlue = usb_host_data->rightBlue;
            return sizeof(WiiTurntableDataFormat3_t);
        }
        default: {
            // TODO: we can just go straight to the correct report format
            WiiClassicDataFormat3_t temp_report;
            WiiClassicDataFormat3_t *report = &temp_report;
            memset(report, 0, sizeof(temp_report));
            memset(data, 0, sizeof(WiiClassicDataFormat3_t));
            // Center sticks
            report->leftStickX = PS3_STICK_CENTER;
            report->leftStickY = PS3_STICK_CENTER;
            report->rightStickX = PS3_STICK_CENTER;
            report->rightStickY = PS3_STICK_CENTER;

            // report->guide = usb_host_data->guide;
            // report->a = usb_host_data->a;
            // report->b = usb_host_data->b;
            // report->x = usb_host_data->x;
            // report->y = usb_host_data->y;
            // report->leftShoulder = usb_host_data->leftShoulder;
            // report->rightShoulder = usb_host_data->rightShoulder;
            report->leftTrigger = usb_host_data->leftTrigger >> 8;
            report->rightTrigger = usb_host_data->rightTrigger >> 8;
            report->leftStickX = (usb_host_data->leftStickX >> 8) + 0x80;
            report->leftStickY = (-usb_host_data->leftStickY >> 8) + 0x80;
            report->rightStickX = (usb_host_data->rightStickX >> 8) + 0x80;
            report->rightStickY = (-usb_host_data->rightStickY >> 8) + 0x80;
            // button bits are inverted
            // report->buttonsLow = ~report->buttonsLow;
            // report->buttonsHigh = ~report->buttonsHigh;
            if (format == 3) {
                memcpy(data, report, sizeof(WiiClassicDataFormat3_t));
                return sizeof(WiiClassicDataFormat3_t);
            } else if (format == 2) {
                WiiClassicDataFormat2_t *real_report = (WiiClassicDataFormat2_t *)data;
                // real_report->buttonsLow = report->buttonsLow;
                // real_report->buttonsHigh = report->buttonsHigh;
                real_report->leftStickX92 = report->leftStickX;
                real_report->leftStickY92 = report->leftStickY;
                real_report->rightStickX92 = report->rightStickX;
                real_report->rightStickY92 = report->rightStickY;
                real_report->leftTrigger = report->leftTrigger;
                real_report->rightTrigger = report->rightTrigger;
                return sizeof(WiiClassicDataFormat2_t);
            } else if (format == 1) {
                // Similar to the turntable, classic format 1 is awful so we need to translate to an intermediate format first
                WiiIntermediateClassicDataFormat_t intermediate_report;
                intermediate_report.rightStickX = report->rightStickX >> 3;
                intermediate_report.leftTrigger = report->leftTrigger >> 3;
                WiiClassicDataFormat1_t *real_report = (WiiClassicDataFormat1_t *)data;
                // real_report->buttonsLow = report->buttonsLow;
                // real_report->buttonsHigh = report->buttonsHigh;
                real_report->leftStickX = report->leftStickX >> 2;
                real_report->leftStickY = report->leftStickY >> 2;
                real_report->rightTrigger = report->rightTrigger >> 3;
                real_report->rightStickX0 = intermediate_report.rightStickX0;
                real_report->rightStickX21 = intermediate_report.rightStickX21;
                real_report->rightStickX43 = intermediate_report.rightStickX43;
                real_report->leftTrigger20 = intermediate_report.leftTrigger20;
                real_report->leftTrigger43 = intermediate_report.leftTrigger43;
                return sizeof(WiiClassicDataFormat1_t);
            }
            // TODO: allow swapping classic face buttons too
            break;
        }
    }
    return 0;
}