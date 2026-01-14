#pragma once
#include "tusb_config.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "device/usbd_pvt.h"
#include "usb/usb_descriptors.h"
#include "instance.hpp"

class UsbDevice : public Instance
{
public:
    inline uint8_t interface()
    {
        return m_interface;
    };
    virtual size_t compatible_section_descriptor(uint8_t *desc, size_t remaining) = 0;
    virtual size_t config_descriptor(uint8_t *desc, size_t remaining) = 0;
    virtual void device_descriptor(tusb_desc_device_t *desc) = 0;
    uint8_t m_interface;
    static inline uint8_t next_epin()
    {
        printf("epin: %d\r\n", m_last_epin);
        return m_last_epin++;
    }
    static inline uint8_t next_epout()
    {
        return m_last_epout++;
    }
    static inline void reset_ep()
    {
        m_last_epin = 0x81;
        m_last_epout = 0x01;
    }

protected:
    bool m_eps_assigned = false;

private:
    static uint8_t m_last_epin;
    static uint8_t m_last_epout;
};