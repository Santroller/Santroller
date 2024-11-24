#include <stdint.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "state_translation/slider.h"
void gh5_to_universal_report(const uint8_t *data, uint8_t len, USB_Host_Data_t *usb_host_data) {
    usb_host_data->green = data[0] & 1 << 4;
    usb_host_data->red = data[0] & 1 << 5;
    usb_host_data->yellow = data[0] & 1 << 6;
    usb_host_data->blue = data[0] & 1 << 7;
    usb_host_data->orange = data[0] & 1 << 0;
    usb_host_data->slider = data[1] ^ 0x80;
    slider_to_solo(usb_host_data);
}