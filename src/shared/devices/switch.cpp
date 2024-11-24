#include <stdint.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
void switch_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    SwitchProGamepad_Data_t *report = (SwitchProGamepad_Data_t *)data;
    DPAD_REV();
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
}