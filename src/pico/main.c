#include <device/usbd_pvt.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <pico/types.h>
#include <pico/unique_id.h>
#include <stdint.h>
#include "pico/multicore.h"
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
#include "usb_host/usb_host.h"
#include "xinput_device.h"
#include "pio_usb.h"
static usb_device_t *usb_device = NULL;
uint8_t controller[DEVICE_EPSIZE_IN];

CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf2[255];
void core1_main() {
    // To run USB SOF interrupt in core1, create alarm pool in core1.
    static pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
    config.pin_dp = USB_DP_PIN;
    config.alarm_pool = (void *)alarm_pool_create(2, 1);
    usb_device = pio_usb_host_init(&config);

    //// Call pio_usb_host_add_port to use multi port
    // const uint8_t pin_dp2 = 8;
    // pio_usb_host_add_port(pin_dp2);

    while (true) {
        pio_usb_host_task();
    }
}

tusb_control_request_t lastreq;
bool was_stall = false;
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    bool valid = false;
    usb_setup_packet_t req_host = {
        USB_SYNC, USB_PID_DATA0
    };
    req_host.crc16[0] = USB_CRC16_PLACE;
    req_host.crc16[1] = USB_CRC16_PLACE;
    memcpy(&req_host.request_type, request, sizeof(tusb_control_request_t));
    update_packet_crc16(&req_host);
    if (request->bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        if (stage == CONTROL_STAGE_SETUP) {
            requestType_t r = {bmRequestType : request->bmRequestType};
            uint16_t len = controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, buf, &valid);
            if (len > request->wLength || !valid) len = request->wLength;
            if (!valid) {
                if (control_in_protocol(usb_device, &req_host, sizeof(req_host), buf, len) < 0) {
                    return false;
                }
            }
            tud_control_xfer(rhport, request, buf, len);
        }
    } else {
        if (stage == CONTROL_STAGE_SETUP) {
            if (was_stall) {
                was_stall = false;
                return false;
            }
            tud_control_xfer(rhport, request, buf, request->wLength);
        }
        if (stage == CONTROL_STAGE_DATA || (stage == CONTROL_STAGE_SETUP && !request->wLength)) {
            requestType_t r = {bmRequestType : request->bmRequestType};
            controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, buf, &valid);
            if (!valid) {
                int data_id = 0;
                int max_len = usb_device->max_packet_size;
                int len = request->wLength;
                uint8_t* buf3 = buf2;
                uint8_t* buf4 = buf;
                uint16_t crc; 
                while (len) {
                    *buf3++ = USB_SYNC;
                    *buf3++ = data_id == 0 ? USB_PID_DATA0 : USB_PID_DATA1;
                    data_id = !data_id;
                    int real_len = MIN(len, max_len);
                    memcpy(buf3, buf4, real_len);
                    crc = calc_usb_crc16(buf3, real_len);
                    buf3 += real_len;
                    buf4 += real_len;
                    *buf3++ = crc & 0xff;
                    *buf3++ = (crc >> 8) & 0xff;
                    len -= real_len;
                }
                if (control_out_protocol(usb_device, &req_host, sizeof(req_host), request->wLength ? buf2 : NULL, buf3-buf2) < 0) {
                    return false;
                }
            }
        }
    }
    return true;
}

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(TDTYPE_Device << 8, 0, buf);
    TUSB_Descriptor_Device_t *td = (TUSB_Descriptor_Device_t *)buf;
    if (usb_device->enumerated) {
        td->VendorID = usb_device->vid;
        td->ProductID = usb_device->pid;
    }
    return buf;
}
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    descriptorRequest(THID_DTYPE_Report << 8, 0, buf);
    return buf;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    descriptorRequest(TDTYPE_Configuration << 8, 0, buf);
    return buf;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    // Arduinos handle their own serial descriptor, so theres no point in sharing an implementation there.
    if (index == 3) {
        return (uint16_t *)&serialString;
    }
    if (descriptorRequest(TDTYPE_String << 8 | index, 0, buf)) {
        return (uint16_t *)buf;
    }
    return NULL;
}

int main() {
    // set_sys_clock_khz(240000, true);
    set_sys_clock_khz(120000, true);
    generateSerialString(serialString.UnicodeString);
    board_init();
    multicore_reset_core1();
    // all USB task run in core1
    multicore_launch_core1(core1_main);
    init();
    tusb_init();
    bool ready = false;
    while (1) {
        if (usb_device->enumerated != ready) {
            ready = usb_device->enumerated;
            if (ready) {
                tud_disconnect();
                sleep_ms(100);
                tud_connect();
                sleep_ms(100);
                continue;
            }
        }
        tud_task();  // tinyusb device task
        uint8_t len = tick(controller);
        if (consoleType == XBOX360) {
            if (tud_xinput_n_ready(0)) {
                tud_xinput_n_report(0, 0, controller, len);
            }
        } else if (consoleType == MIDI) {
            tud_midi_n_packet_write(0, controller);
        } else {
            if (tud_hid_n_ready(0)) {
                tud_hid_n_report(0, 0, controller, len);
            }
        }
    }
}

usbd_class_driver_t driver[] = {{
#if CFG_TUSB_DEBUG >= 2
    .name = "XInput",
#endif
    .init = xinputd_init,
    .reset = xinputd_reset,
    .open = xinputd_open,
    .control_xfer_cb = tud_vendor_control_xfer_cb,
    .xfer_cb = xinputd_xfer_cb,
    .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return driver;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    requestType_t r = {bmRequestType_bit : {direction : USB_DIR_DEVICE_TO_HOST, recipient : USB_REQ_RCPT_INTERFACE, type : USB_REQ_TYPE_CLASS}};
    bool valid;
    return controlRequest(r, THID_REQ_GetReport, report_type << 8 | report_id, instance, reqlen, &buffer, &valid);
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
    requestType_t r = {bmRequestType_bit : {direction : USB_DIR_HOST_TO_DEVICE, recipient : USB_REQ_RCPT_INTERFACE, type : USB_REQ_TYPE_CLASS}};
    bool valid;
    controlRequest(r, THID_REQ_SetReport, report_type << 8 | report_id, instance, bufsize, &buffer, &valid);
}