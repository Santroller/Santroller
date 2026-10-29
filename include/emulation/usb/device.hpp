#pragma once
#include "tusb_config.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "device/usbd_pvt.h"
#include "emulation/usb/usb_descriptors.h"
#include "instance.hpp"

class UsbDevice : public Instance
{
public:
    virtual ~UsbDevice() {}
    struct AllocationState
    {
        uint8_t epin;
        uint8_t epout;
        uint8_t strid;
    };
    static AllocationState allocation_state()
    {
        return {m_last_epin, m_last_epout, m_last_strid};
    }
    static void restore_allocation_state(AllocationState state)
    {
        m_last_epin = state.epin;
        m_last_epout = state.epout;
        m_last_strid = state.strid;
    }
    // The first of this device's interfaces, it owns interface_count() of them from there on
    uint8_t interface_id = 0;
    virtual uint8_t interface_count() const { return 1; }
    virtual size_t compatible_section_descriptor(uint8_t *desc, size_t remaining) = 0;
    virtual size_t config_descriptor(uint8_t *desc, size_t remaining) = 0;
    virtual size_t device_name(uint8_t idx, char *desc) = 0;
    virtual void device_descriptor(tusb_desc_device_t *desc) = 0;
    virtual bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) = 0;
    virtual bool control_transfer(uint8_t stage, tusb_control_request_t const *request) = 0;
    virtual uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len) = 0;
    bool suspends_with_host() const override { return true; }
    // Runs in place of process() while the host is suspended: turns the LEDs off and keeps the mappings
    // updating, so their state is current when the host resumes
    void process_suspended(bool full_poll, bool send_events) override;
    // Once per loop while the host is suspended, with whether one of the standby profile's wake buttons is held
    static void wake_host(bool pressed);
    // Once per loop otherwise, with whether a 360 is in standby (see host_in_standby) and whether one of
    // standby lighting's wake buttons is held. A fresh press sends guide, which turns it on.
    static void wake_console_in_standby(bool in_standby, bool pressed);
    // Works out whether the host is suspending the bus, once per loop before the instances run
    static bool update_host_suspended();
    static bool host_suspended() { return s_host_suspended; }
    // No frames from the host for a while, whether or not it ever configured us
    static bool bus_idle() { return s_bus_idle; }
    // Whether a host has configured us since the device stack started
    static bool mounted_since_init;
    // Disconnects the bluetooth controllers of profiles that ask for it, once per suspend
    void disconnect_bluetooth_controllers();
    static inline uint8_t next_epin()
    {
        return m_last_epin++;
    }
    static inline uint8_t next_epout()
    {
        return m_last_epout++;
    }
    static inline uint8_t next_strid()
    {
        return m_last_strid++;
    }
    static inline void reset_ep()
    {
        m_last_epin = 0x81;
        m_last_epout = 0x01;
        m_last_strid = STRID_COUNT;
    }

    // Bumped by tud_suspend_cb so every instance disarms wakeup at the start of a suspend
    static volatile uint32_t suspend_generation;

private:
    // The suspend this instance last saw start, to disconnect bluetooth controllers once per suspend
    uint32_t m_suspend_generation = 0;
    // Only wake on a fresh press, so a button held as the host suspends doesn't wake it
    // straight back up, and one press doesn't keep re-signalling resume.
    static bool s_wake_armed;
    static bool s_standby_wake_armed;
    static uint32_t s_wake_generation;
    static bool s_host_suspended;
    static bool s_bus_idle;
    static uint8_t m_last_epin;
    static uint8_t m_last_epout;
    static uint8_t m_last_strid;
};