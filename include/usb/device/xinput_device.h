#pragma once

#include "common/tusb_common.h"
#include "device/usbd.h"
#include "protocols/xinput.hpp"
#include "device.hpp"
#include "enums.pb.h"
#include "config.hpp"
#include "usb/usb_descriptors.h"

#ifndef CFG_TUD_XINPUT_EPSIZE
#define CFG_TUD_XINPUT_EPSIZE 64
#endif
class XInputSecurityDevice : public UsbDevice
{
public:
    XInputSecurityDevice();
    void initialize();
    void process();
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
};
class XInputGamepadDevice : public UsbDevice
{
public:
    XInputGamepadDevice();
    void initialize();
    void process();
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
    uint8_t m_epin;
    uint8_t m_epout;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_RX_BUFSIZE];

    static uint8_t xinputInterfaces[4];
    static uint8_t lastIntf;

private:
    XInputGamepad_Data_t initialReport;
};