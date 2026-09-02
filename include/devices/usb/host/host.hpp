#pragma once
#include "tusb_config.h"
#include "tusb.h"
#include "device.pb.h"
#include "devices/base.hpp"
#include "devices/midi.hpp"
#include <vector>
#include <memory>
#include <array>

class UsbHostInterface : public MidiDevice
{
public:
    virtual ~UsbHostInterface() { printf("~UsbHostInterface()\r\n"); };

    UsbHostInterface(uint8_t d_addr, uint8_t interface, uint16_t id) : MidiDevice(nullptr, id, true), m_dev_addr(d_addr), m_interface(interface)
    {
        memset(m_name, 0, sizeof(m_name));
    }
    virtual bool set_config();
    virtual bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) = 0;
    uint8_t dev_addr()
    {
        return m_dev_addr;
    }
    void begin() {};
    void end(bool full) {};
    virtual void disconnect();
    virtual bool tick_digital(proto_Output& type) = 0;
    virtual uint16_t tick_analog(proto_Output& type) = 0;
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
    
    // Getters for protected members
    uint8_t dev_addr() const { return m_dev_addr; }
    uint8_t interface() const { return m_interface; }
    SubType subtype() const { return m_subtype; }
    void set_subtype(SubType type) { m_subtype = type; }

protected:
    uint8_t m_dev_addr;
    uint8_t m_interface;
    SubType m_subtype = SubType_Gamepad;
    bool m_sent_type = false;
    bool m_has_name = false;
    bool m_delayed_init = false;
    CFG_TUSB_MEM_ALIGN char m_name[128] = {0};
    uint32_t send_ctrl_xfer(tusb_control_request_t setup, void *buffer, bool *status);
    bool send_intr_xfer(uint8_t endpoint, const void *buffer, uint8_t len);
};

void usb_host_add_assignable_interface(std::shared_ptr<UsbHostInterface> device);
void usb_host_add_enumerating_interface(std::shared_ptr<UsbHostInterface> device);
void usb_host_remove_assignable_interface(UsbHostInterface *device);
void usb_host_remove_enumerating_interface(UsbHostInterface *device);
void usb_host_remove_interfaces_by_address(uint8_t dev_addr);
size_t usb_host_assignable_interface_count();
void usb_host_add_assignable_devices(bool rescan);
void usb_host_update_interfaces(bool full_poll, bool send_events);

class UsbHostDevice : public Device
{
public:
    ~UsbHostDevice() { printf("~UsbHostDevice(%p)\r\n", this); }

    UsbHostDevice(uint8_t d_addr, uint16_t id) : Device(id), m_dev_addr(d_addr)
    {
         printf("UsbHostDevice(%p)\r\n", this);
    }
    void disconnect();
    void begin() {};
    void end(bool full) {};
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
    std::array<std::shared_ptr<UsbHostInterface>,30> host_devices_by_itf;
    std::array<std::shared_ptr<UsbHostInterface>,16> host_devices_by_endpoint_in;
    std::array<std::shared_ptr<UsbHostInterface>,16> host_devices_by_endpoint_out;

protected:
    uint8_t m_dev_addr;
};