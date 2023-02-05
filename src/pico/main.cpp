#include <Arduino.h>
#include <SPI.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <string.h>
#include <tusb.h>

#include "commands.h"
#include "common/tusb_types.h"
#include "config.h"
#include "controller_reports.h"
#include "controllers.h"
#include "device/dcd.h"
#include "device/usbd_pvt.h"
#include "hardware/structs/usb.h"
#include "hardware/watchdog.h"
#include "host/usbh_classdriver.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pins.h"
#include "pio_usb.h"
#include "serial.h"
#include "shared_main.h"
#include "xinput_device.h"
#include "xinput_host.h"

CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf2[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN STRING_DESCRIPTOR_PICO serialstring = {
    .bLength = (sizeof(uint8_t) + sizeof(uint8_t) + SERIAL_LEN),
    .bDescriptorType = USB_DESCRIPTOR_STRING,
    .UnicodeString = {}};

uint16_t host_vid = 0;
uint16_t host_pid = 0;
uint8_t host_dev_addr = 0;
bool passthrough_ready = false;

void setup() {
    uart_set_baudrate(uart0, 115200);
    generateSerialString(&serialstring);
    tusb_init();
    init_main();
}
bool reset_on_next = false;
USB_Report_Data_t report;
unsigned int last = 0;
#if CONSOLE_TYPE != MIDI
#define TICK_CHECK tud_xinput_n_ready(0)
#else
#define TICK_CHECK false
#endif
long xbox_timer = 0;
long ps5_timer = 0;
long wii_timer = 0;
bool read_config = false;
bool received_after_read_config = false;
void loop() {
    if (reset_on_next) {
        tud_disconnect();
        sleep_ms(100);
        tud_connect();
        reset_on_next = false;
        return;
    }
    if (consoleType == UNIVERSAL) {
        // PS5 just stops communicating after sending a set idle
        if (set_idle && ps5_timer == 0) {
            ps5_timer = millis();
        }
        if (ps5_timer != 0 && millis() - ps5_timer > 100) {
            consoleType = PS3;
            reset_usb();
        }
        // Windows and XBOX One both send out a WCID request
        if (xbox_timer == 0 && windows_or_xbox_one) {
            xbox_timer = millis();
        }
        // But xbox follows that up with a SET_INTERFACE call, so if we don't see one of those then we can assume windows
        if (xbox_timer != 0 && millis() - xbox_timer > 1000) {
            consoleType = XBOX360;
            reset_usb();
        }
        // Wii gives up after reading the config descriptor
        if (read_config && !received_after_read_config && millis() - wii_timer > 100) {
            consoleType = WII_RB;
            reset_usb();
            received_after_read_config = true;
        }
    }
    if (fromConsoleLen) {
        tuh_xinput_send_report(1, 0, fromConsole,  fromConsoleLen);
        fromConsoleLen = 0;
    }
    if (tuh_xinput_ready(1, 0)) {
        tuh_xinput_receive_report(1,0);
    }
    tud_task();
    tuh_task();
    uint8_t size = 0;
    if (TICK_CHECK || millis() - last > 5) {
        last = millis();
        size = tick(&report);
    }

    if (size) {
#if CONSOLE_TYPE == MIDI
        // tud_midi_n_packet_write(0, report);
#else
        if (tud_xinput_n_ready(0)) {
            tud_xinput_n_report(0, 0, &report, size);
        }
#endif
    }
}
void tud_mount_cb(void) {
    xbox_one_state = Announce;
    fromControllerLen = 0;
    fromConsoleLen = 0;
    if (consoleType == XBOXONE && tuh_xinput_mounted(1, 0)) {
        fromConsole[0] = 0x05;
        fromConsole[1] = 0x00;
        tuh_xinput_send_report(1, 0, fromConsole,  2);
    }
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance) {
    if (consoleType == XBOX360) {
        if (passthrough_ready) {
            return;
        }
        host_dev_addr = dev_addr;
        tuh_vid_pid_get(dev_addr, &host_vid, &host_pid);
        passthrough_ready = true;
    }
    reset_usb();
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {
    host_vid = 0;
    host_pid = 0;
    host_dev_addr = 0;
    passthrough_ready = false;
    reset_usb();
}

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(USB_DESCRIPTOR_DEVICE << 8, 0, buf);
    if (passthrough_ready && consoleType == XBOX360) {
        USB_DEVICE_DESCRIPTOR *td = (USB_DEVICE_DESCRIPTOR *)buf;
        td->idVendor = host_vid;
        td->idProduct = host_pid;
    }
    return buf;
}
uint8_t const *tud_hid_custom_descriptor_report_cb(uint8_t instance) {
    descriptorRequest(HID_DESCRIPTOR_REPORT << 8, 0, buf);
    return buf;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    descriptorRequest(USB_DESCRIPTOR_CONFIGURATION << 8, 0, buf);
    wii_timer = millis();
    read_config = true;
    return buf;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    // Arduinos handle their own serial descriptor, so theres no point in sharing an implementation there.
    if (index == 3) {
        return (uint16_t *)&serialstring;
    }
    if (descriptorRequest(USB_DESCRIPTOR_STRING << 8 | index, 0, buf)) {
        return (uint16_t *)buf;
    }
    return NULL;
}
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    if (xbox_one_state != Auth) {
        return;
    }
    fromControllerLen = len;
    memcpy(fromController, report, len);
}
void reset_usb(void) {
    reset_on_next = true;
}
tusb_control_request_t lastreq;
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    if (stage == CONTROL_STAGE_SETUP) {
        if (set_idle && ps5_timer != 0) {
            set_idle = false;
            ps5_timer = 0;
        }
        if (read_config) {
            received_after_read_config = true;
        }
    }
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD && request->bRequest == TUSB_REQ_GET_DESCRIPTOR) {
        //------------- STD Request -------------//
        if (stage == CONTROL_STAGE_SETUP) {
            uint8_t const desc_type = tu_u16_high(request->wValue);
            // uint8_t const desc_index = tu_u16_low (request->wValue);

            if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_HID) {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_HID << 8, 0, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            } else if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_REPORT) {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_REPORT << 8, 0, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            } else {
                return false;  // stall unsupported request
            }
        }
    }
    if (controlRequestValid(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength)) {
        if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
            if (stage == CONTROL_STAGE_SETUP) {
                uint16_t len = controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf);
                tud_control_xfer(rhport, request, buf, len);
            }
        } else if (stage == CONTROL_STAGE_SETUP) {
            tud_control_xfer(rhport, request, buf, request->wLength);
        }
        if (stage == CONTROL_STAGE_DATA || (stage == CONTROL_STAGE_SETUP && !request->wLength)) {
            controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf);
        }
    } else if (passthrough_ready) {
        tuh_xfer_t xfer = {};
        xfer.daddr = host_dev_addr;
        xfer.ep_addr = 0;
        xfer.setup = request;
        xfer.buffer = buf;
        xfer.complete_cb = NULL;
        xfer.user_data = 0;
        if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
            if (stage == CONTROL_STAGE_SETUP) {
                tuh_control_xfer(&xfer);
                if (xfer.result != XFER_RESULT_SUCCESS) {
                    return false;
                }
                tud_control_xfer(rhport, request, buf, request->wLength);
            }
        } else {
            if (stage == CONTROL_STAGE_SETUP) {
                tud_control_xfer(rhport, request, buf, request->wLength);
            }
            if (stage == CONTROL_STAGE_DATA || (stage == CONTROL_STAGE_SETUP && !request->wLength)) {
                tuh_control_xfer(&xfer);
                if (xfer.result != XFER_RESULT_SUCCESS) {
                    return false;
                }
            }
        }
    }

    return true;
}

usbd_class_driver_t driver[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Device_HID",
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

usbh_class_driver_t driver_host[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Host_HID",
#endif
        .init = xinputh_init,
        .open = xinputh_open,
        .set_config = xinputh_set_config,
        .xfer_cb = xinputh_xfer_cb,
        .close = xinputh_close}};

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return driver_host;
}
void reboot(void) {
    watchdog_enable(1, false);
    for (;;) {
    }
}
void bootloader(void) {
    reset_usb_boot(0, 0);
}