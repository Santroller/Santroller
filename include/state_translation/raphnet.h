#include <stdint.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
extern bool hasFlags;
extern const uint8_t dpad_bindings[11];
extern const uint8_t dpad_bindings_reverse[8];
inline void raphnet_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    switch (sub_type) {
        case GAMEPAD: {
            RaphnetGamepad_Data_t *report = (RaphnetGamepad_Data_t *)data;
            usb_host_data->a |= report->a;
            usb_host_data->b |= report->b;
            usb_host_data->x |= report->x;
            usb_host_data->y |= report->y;
            usb_host_data->leftShoulder |= report->leftShoulder;
            usb_host_data->rightShoulder |= report->rightShoulder;
            if (report->leftTrigger) {
                usb_host_data->leftTrigger = report->leftTrigger;
            }
            if (report->rightTrigger) {
                usb_host_data->rightTrigger = report->rightTrigger;
            }
            if (report->leftStickX != 0x80) {
                usb_host_data->leftStickX = report->leftStickX - 16000;
            }
            if (report->leftStickY != 0x80) {
                usb_host_data->leftStickY = -report->leftStickY - 16000;
            }
            if (report->rightStickX != 0x80) {
                usb_host_data->rightStickX = report->rightStickX - 16000;
            }
            if (report->rightStickY != 0x80) {
                usb_host_data->rightStickY = -report->rightStickY - 16000;
            };
            usb_host_data->dpadLeft |= report->left;
            usb_host_data->dpadRight |= report->right;
            usb_host_data->dpadUp |= report->up;
            usb_host_data->dpadDown |= report->down;
            break;
        }
        case GUITAR_HERO_GUITAR: {
            RaphnetGuitar_Data_t *report = (RaphnetGuitar_Data_t *)data;
            usb_host_data->dpadUp |= report->up;
            usb_host_data->dpadDown |= report->down;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            usb_host_data->kick1 |= report->kick1;
            if (report->tilt) {
                usb_host_data->tilt = report->tilt >> 8;
            }
            if (report->whammy) {
                usb_host_data->whammy = report->whammy >> 8;
            }
            if (report->leftStickX) {
                usb_host_data->leftStickX = report->leftStickX - 16000;
            }
            if (report->leftStickY) {
                usb_host_data->leftStickY = -report->leftStickY - 16000;
            }
            if (report->slider) {
                usb_host_data->slider = report->slider >> 8;
            };
            break;
        }
        case GUITAR_HERO_DRUMS: {
            RaphnetDrum_Data_t *report = (RaphnetDrum_Data_t *)data;
            // TODO: wii drums are weird but i assume we can atleast share the logic for both types of wii drums
            usb_host_data->leftStickX = report->leftStickX - 16000;
            usb_host_data->leftStickY = report->leftStickY - 16000;
            usb_host_data->start = report->plus;
            usb_host_data->back = report->minus;
            usb_host_data->green |= report->green;
            usb_host_data->red |= report->red;
            usb_host_data->yellow |= report->yellow;
            usb_host_data->blue |= report->blue;
            usb_host_data->orange |= report->orange;
            break;
        }
    }
}