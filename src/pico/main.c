#include <device/usbd_pvt.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <pico/types.h>
#include <pico/unique_id.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tusb.h>

#include "bsp/board.h"
#include "defines.h"
#include "descriptors.h"
#include "lib_main.h"
#include "serial.h"
#include "stdbool.h"
#include "usb.h"
#include "xinput_device.h"
int validAnalog = 0;

bool isRF = false;
bool typeIsGuitar;
bool typeIsDrum;
uint8_t inputType;
uint8_t pollRate;

#define UART_ID uart1
#define BAUD_RATE 115200

// TODO: how big does this need to be?
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
// We are using pins 4 and 5, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 4
#define UART_RX_PIN 5
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    uint8_t *data;
    if (request->bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        if (stage == CONTROL_STAGE_SETUP) {
            requestType_t r = {bmRequestType : request->bmRequestType};
            if (controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, &data)) {
                tud_control_xfer(rhport, request, data, request->wLength);
            }
        }
    } else {
        if (stage == CONTROL_STAGE_SETUP) {
            tud_control_xfer(rhport, request, buf, request->wLength);
        } else if (stage == CONTROL_STAGE_ACK) {
            data = buf;
            requestType_t r = {bmRequestType : request->bmRequestType};
            controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, &data);
        }
    }
}

uint8_t const *tud_descriptor_device_cb(void) {
    const void *data;
    descriptorRequest(TDTYPE_Device << 8, 0, &data);
    return data;
}
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    const void *data;
    descriptorRequest(THID_DTYPE_Report << 8, 0, &data);
    return data;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    const void *data;
    descriptorRequest(TDTYPE_Configuration << 8, 0, &data);
    return data;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    const void *data;
    // Arduinos handle their own serial descriptor, so theres no point in sharing an implementation there.
    if (index == 3) {
        return (uint16_t *)&serialString;
    } else {
        descriptorRequest(TDTYPE_String << 8 | index, 0, &data);
    }
    return data;
}

int main() {
    generateSerialString(serialString.UnicodeString);
    init();
    board_init();
    tusb_init();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    while (1) {
        tud_task();  // tinyusb device task
    }
}

usbd_class_driver_t driver[] = {{.init = xinputd_init,
                                 .reset = xinputd_reset,
                                 .open = xinputd_open,
                                 .control_xfer_cb = tud_vendor_control_xfer_cb,
                                 .xfer_cb = xinputd_xfer_cb,
                                 .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return driver;
}

static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        //  When requested, return the ps3 report ids so that we have console
        //  compatibility
        if (deviceType <= PS3_ROCK_BAND_DRUMS) {
            id[3] = 0x00;
        } else if (deviceType <= PS3_GUITAR_HERO_DRUMS) {
            id[3] = 0x06;
        }
        buffer = id;
        return sizeof(id);
    }
    return 0;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {}