#pragma once
// Fake devices/usb/host/host.hpp: the parts of UsbHostInterface / UsbHostDevice that
// xone_host.cpp uses, without the MIDI / device base classes behind them. Interrupt OUT
// transfers are recorded by fakes.cpp.
#include <stdint.h>
#include <string.h>
#include <array>
#include <memory>
#include <optional>
#include <vector>
#include "devices/hardware_id.hpp"
#include "tusb_config.h"
#include "tusb.h"
#include "device.pb.h"
#include "input.pb.h"

class UsbHostInterface
{
public:
    UsbHostInterface(uint8_t d_addr, uint8_t interface, uint16_t id) : m_dev_addr(d_addr), m_interface(interface), m_id(id) {}
    virtual ~UsbHostInterface() {}
    virtual bool set_config() { return true; }
    virtual bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) = 0;
    virtual void disconnect() {}
    virtual bool tick_digital(proto_Output &type) = 0;
    virtual uint16_t tick_analog(proto_Output &type) = 0;
    virtual void update(bool full_poll, bool send_events) { (void)full_poll, (void)send_events; }
    virtual void set_rumble(uint8_t left, uint8_t right) { (void)left, (void)right; }
    virtual void set_player_led(uint8_t player) { (void)player; }
    virtual bool has_rumble() const { return false; }
    virtual bool has_player_led() const { return false; }
    virtual std::optional<uint64_t> hardware_id() const { return std::nullopt; }
    virtual bool is_xbox_one_pad() const { return false; }
    virtual bool key_pressed(uint8_t keycode) { (void)keycode; return false; }
    // Recorded in gip_fake::hotplug_events
    void send_hotplug_event(bool connected);

    uint8_t dev_addr() const { return m_dev_addr; }
    uint8_t interface() const { return m_interface; }
    SubType subtype() const { return m_subtype; }
    void set_subtype(SubType type) { m_subtype = type; }

protected:
    uint8_t m_dev_addr;
    uint8_t m_interface;
    uint16_t m_id;
    SubType m_subtype = SubType_Gamepad;
    bool m_delayed_init = false;
    bool m_has_name = false;
    char m_name[128] = {0};
    bool send_intr_xfer(uint8_t endpoint, const void *buffer, uint8_t len);
};

class UsbHostDevice
{
public:
    UsbHostDevice(uint8_t d_addr, uint16_t id) : m_id(id), m_dev_addr(d_addr) {}
    uint8_t dev_addr() { return m_dev_addr; }
    void disconnect()
    {
        for (auto &itf : host_devices_by_itf)
        {
            if (itf)
            {
                itf->disconnect();
            }
        }
    }
    uint16_t m_id;
    std::array<std::shared_ptr<UsbHostInterface>, 30> host_devices_by_itf;
    std::array<std::shared_ptr<UsbHostInterface>, 16> host_devices_by_endpoint_in;
    std::array<std::shared_ptr<UsbHostInterface>, 16> host_devices_by_endpoint_out;

protected:
    uint8_t m_dev_addr;
};

void usb_host_add_assignable_interface(std::shared_ptr<UsbHostInterface> device);
void usb_host_add_enumerating_interface(std::shared_ptr<UsbHostInterface> device);
void usb_host_remove_enumerating_interface(UsbHostInterface *device);
void usb_host_remove_assignable_interface(UsbHostInterface *device);

// Same as the firmware's
inline bool any_key_pressed(UsbHostInterface &device)
{
    for (uint16_t key = 0x04; key <= 0xE7; key++)
    {
        if (device.key_pressed(key))
        {
            return true;
        }
    }
    return false;
}
