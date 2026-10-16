#pragma once
#include <stdint.h>

// Whether a USB host is there, so USB profiles only match with one and a profile further down (eg
// bluetooth or PS2 emulation) can take over otherwise. The config interface always enumerates, so
// this works whether or not a profile is using USB right now.
//
// Once a host has talked to us it counts as there until VBUS goes away, so a host that is asleep
// (suspending the bus) or the device stack restarting doesn't drop the USB profile. Without VBUS
// sensing an unplug can't be seen, only that no host turned up after powering on.
class UsbHostPresence
{
public:
    static UsbHostPresence &instance()
    {
        static UsbHostPresence presence;
        return presence;
    }

    // Long enough for a host to enumerate us after powering on
    static constexpr uint32_t ABSENT_MS = 1500;
    // Rides out VBUS bouncing as the cable goes in or out
    static constexpr uint32_t NO_VBUS_MS = 50;

    // Assumed present at boot, so a USB profile doesn't lose to a bluetooth one for the moment
    // before the host enumerates us
    bool present() const { return m_present; }

    // host_active: a host has reset the bus and isn't suspending it. vbus is only used when
    // vbus_known. Returns true when the result changes.
    bool update(bool host_active, bool vbus_known, bool vbus, uint32_t now)
    {
        bool no_vbus = vbus_known && !vbus;
        if (no_vbus)
        {
            m_host_seen = false;
        }
        else if (host_active)
        {
            m_host_seen = true;
        }
        if (m_host_seen != m_raw)
        {
            m_raw = m_host_seen;
            m_raw_since = now;
        }
        if (m_raw == m_present)
        {
            return false;
        }
        if (!m_raw && now - m_raw_since < (no_vbus ? NO_VBUS_MS : ABSENT_MS))
        {
            return false;
        }
        m_present = m_raw;
        return true;
    }

    void reset(uint32_t now)
    {
        m_present = true;
        m_raw = true;
        m_raw_since = now;
        m_host_seen = false;
    }

private:
    UsbHostPresence() = default;

    bool m_present = true;
    bool m_raw = true;
    uint32_t m_raw_since = 0;
    bool m_host_seen = false;
};

