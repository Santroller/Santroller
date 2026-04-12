#pragma once
#include "tusb_config.h"
#include "tusb.h"
#include "device.pb.h"
#include "devices/base.hpp"
#include "devices/midi.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

class UsbHostInterface : public MidiDevice
{
public:
    virtual ~UsbHostInterface() { printf("~UsbHostInterface()\r\n"); };
    UsbHostInterface(uint8_t d_addr, uint8_t interface, uint16_t id) : MidiDevice(id, true), m_dev_addr(d_addr), m_interface(interface)
    {
        printf("UsbHostInterface: %p\r\n", this);
    }
    virtual bool set_config() = 0;
    virtual bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) = 0;
    uint8_t dev_addr()
    {
        return m_dev_addr;
    }
    virtual bool tick_digital(UsbButtonType type) = 0;
    virtual uint16_t tick_analog(UsbAxisType type) = 0;
    virtual void update(bool full_poll, bool send_events);
    bool is_wii_extension(WiiExtType type)
    {
        return false;
    }
    bool is_usb_device(proto_SpecificUsbDevice type)
    {
        return false;
    }
    bool is_usb_type(SubType type)
    {
        return type == m_subtype;
    }
    bool is_bluetooth_device(proto_SpecificUsbDevice type)
    {
        return false;
    }
    bool is_bluetooth_type(SubType type)
    {
        return false;
    }
    bool is_ps2_device(PS2ControllerType type)
    {
        return false;
    }
    bool using_pin(uint8_t pin)
    {
        return false;
    }

protected:
    uint8_t m_dev_addr;
    uint8_t m_interface;
    SubType m_subtype = SubType_Gamepad;
    bool m_sent_type = false;
    char m_name[32] = "";
    bool m_fetched_name = false;
    uint32_t send_ctrl_xfer(tusb_control_request_t setup, void *buffer, bool *status);
    bool send_intr_xfer(uint8_t endpoint, const void *buffer, uint8_t len);
};

class UsbHostDevice : public Device
{
public:
    ~UsbHostDevice() { printf("~UsbHostDevice()\r\n"); }
    UsbHostDevice(uint8_t d_addr, uint16_t id) : Device(id), m_dev_addr(d_addr)
    {
    }
    uint8_t dev_addr()
    {
        return m_dev_addr;
    }
    void update(bool full_poll, bool send_events)
    {
    }
    bool is_wii_extension(WiiExtType type)
    {
        return false;
    }
    bool is_usb_device(proto_SpecificUsbDevice type)
    {
        return false;
    }
    bool is_usb_type(SubType type)
    {
        return false;
    }
    bool is_bluetooth_device(proto_SpecificUsbDevice type)
    {
        return false;
    }
    bool is_bluetooth_type(SubType type)
    {
        return false;
    }
    bool is_ps2_device(PS2ControllerType type)
    {
        return false;
    }
    bool using_pin(uint8_t pin)
    {
        return false;
    }
    std::unordered_map<uint8_t, std::shared_ptr<UsbHostInterface>> host_devices_by_itf;
    std::shared_ptr<UsbHostInterface> host_devices_by_endpoint_in[16];
    std::shared_ptr<UsbHostInterface> host_devices_by_endpoint_out[16];

protected:
    uint8_t m_dev_addr;
};