#include "state_translation/ogxbox.h"

#include <stdint.h>

#include "Usb.h"
#include "defines.h"
#include "ids.h"
#include "reports/controller_reports.h"
#include "state_translation/x360.h"
void ogxbox_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
    usb_host_data->dpadLeft |= report->dpadLeft;
    usb_host_data->dpadRight |= report->dpadRight;
    usb_host_data->dpadUp |= report->dpadUp;
    usb_host_data->dpadDown |= report->dpadDown;
    usb_host_data->start |= report->start;
    usb_host_data->back |= report->back;
    usb_host_data->a |= report->a > 0x20;
    usb_host_data->b |= report->b > 0x20;
    usb_host_data->x |= report->x > 0x20;
    usb_host_data->y |= report->y > 0x20;
    usb_host_data->leftShoulder |= report->leftShoulder > 0x20;
    usb_host_data->rightShoulder |= report->rightShoulder > 0x20;
    usb_host_data->back |= report->back;
    usb_host_data->start |= report->start;
    usb_host_data->leftThumbClick |= report->leftThumbClick;
    usb_host_data->rightThumbClick |= report->rightThumbClick;
    usb_host_data->dpadLeft = report->dpadLeft;
    usb_host_data->dpadRight = report->dpadRight;
    usb_host_data->dpadUp = report->dpadUp;
    usb_host_data->dpadDown = report->dpadDown;
    if (report->leftTrigger) {
        usb_host_data->leftTrigger = report->leftTrigger << 8;
    }
    if (report->rightTrigger) {
        usb_host_data->rightTrigger = report->rightTrigger << 8;
    }
    if (report->leftStickX) {
        usb_host_data->leftStickX = report->leftStickX;
    }
    if (report->leftStickY) {
        usb_host_data->leftStickY = report->leftStickY;
    }
    if (report->rightStickX) {
        usb_host_data->rightStickX = report->rightStickX;
    }
    if (report->rightStickY) {
        usb_host_data->rightStickY = report->rightStickY;
    }
    if (report->leftShoulder) {
        usb_host_data->pressureL1 = report->leftShoulder;
    }
    if (report->rightShoulder) {
        usb_host_data->rightShoulder = report->rightShoulder;
    }
    if (report->y) {
        usb_host_data->pressureTriangle = report->y;
    }
    if (report->b) {
        usb_host_data->pressureCircle = report->b;
    }
    if (report->a) {
        usb_host_data->pressureCross = report->a;
    }
    if (report->x) {
        usb_host_data->pressureSquare = report->x;
    }
}

uint8_t universal_report_to_ogxbox(uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    // Since none of the instruments on ogxbox exist, we just mirror the 360 reports
    XInputGamepad_Data_t dpad_report;
    universal_report_to_x360((uint8_t *)&dpad_report, sub_type, usb_host_data);
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
    report->a = dpad_report.a * 0xFF;
    report->b = dpad_report.b * 0xFF;
    report->x = dpad_report.x * 0xFF;
    report->y = dpad_report.y * 0xFF;
    report->leftShoulder = dpad_report.leftShoulder * 0xFF;
    report->rightShoulder = dpad_report.rightShoulder * 0xFF;
    report->leftTrigger = dpad_report.leftTrigger >> 8;
    report->rightTrigger = dpad_report.rightTrigger >> 8;
    report->leftStickX = dpad_report.leftStickX;
    report->leftStickY = dpad_report.leftStickY;
    report->rightStickX = dpad_report.rightStickX;
    report->rightStickY = dpad_report.rightStickY;
    report->leftThumbClick = dpad_report.leftThumbClick;
    report->rightThumbClick = dpad_report.rightThumbClick;
    return sizeof(OGXboxGamepad_Data_t);
}

void fill_device_descriptor_ogxbox(USB_DEVICE_DESCRIPTOR *dev) {
    dev->idVendor = MICROSOFT_VID;
    dev->idProduct = DUKE_PID;
}