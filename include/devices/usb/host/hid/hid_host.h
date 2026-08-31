#pragma once
#include "devices/usb/host/host.hpp"
#include "protocols/xinput.hpp"
#include "class/hid/hid.h"
#include "hidparser.h"
#include "protocols/dance_pad.hpp"

#define UP 1 << 0
#define DOWN 1 << 1
#define LEFT 1 << 2
#define RIGHT 1 << 3
class HidHost : public UsbHostInterface
{
public:
    ~HidHost() {}
    HidHost(uint8_t dev_addr, uint8_t interface, uint16_t id) : UsbHostInterface(dev_addr, interface, id) {}
    static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
    static const uint8_t dpad_bindings_reverse[8];
    uint32_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen, bool* status);
    uint32_t set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t bufsize, bool* status);
    virtual bool send_intr_report(const void *buffer, uint8_t len) {
        return false;
    }
    virtual bool get_intr_report(void* buffer, uint8_t max_len) {
        return false;
    }
};

// Specific HID host implementations are now in separate headers
#include "devices/usb/host/hid/switch_host.h"
#include "devices/usb/host/hid/raphnet_host.h"
#include "devices/usb/host/hid/generic_hid_host.h"
#include "devices/usb/host/hid/rhythm_game_host.h"
#include "devices/usb/host/hid/misc_hid_host.h"
#include "devices/usb/host/hid/keyboard_mouse_host.h"
