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
    // Call from process() while tud_suspended(): turns the LEDs off and keeps polling
    // the mappings so a button press can remote-wake the host.
    void process_suspended(bool full_poll, bool send_events);
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
    // Only wake on a fresh press, so a button held as the host suspends doesn't wake it
    // straight back up, and one press doesn't keep re-signalling resume.
    bool m_wake_armed = false;
    uint32_t m_wake_generation = 0;
    static uint8_t m_last_epin;
    static uint8_t m_last_epout;
    static uint8_t m_last_strid;
};