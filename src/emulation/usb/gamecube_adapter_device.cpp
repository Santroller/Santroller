#include "emulation/usb/gamecube_adapter_device.h"
#include "emulation/usb/usb_devices.h"
#include "managers/profile_manager.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

void GameCubeAdapterDevice::initialize()
{
    m_epin = next_epin();
    m_epout = next_epout();
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);
    // A rumble report fills the endpoint, so isn't a short packet that would end a longer transfer
    m_epout_xfer_len = GAMECUBE_ADAPTER_EP_OUT_SIZE;
    m_started = false;
    memset(m_port_rumble, 0, sizeof(m_port_rumble));
    gamecube_adapter_build_report(nullptr, 0, m_report);
}

// The same resting report the joybus GameCube controller starts from
static GameCubeGamepad_Data_t resting_report(SubType subtype)
{
    GameCubeGamepad_Data_t report = {};
    report.alwaysOne = 1;
    report.leftStickX = GC_STICK_CENTER;
    report.leftStickY = GC_STICK_CENTER;
    report.cStickX = GC_STICK_CENTER;
    report.cStickY = GC_STICK_CENTER;
    if (subtype == Wheel)
    {
        report.leftStickX = GC_WHEEL_POWER_AND_PEDALS;
        report.cStickX = 0;
        report.cStickY = 0;
    }
    return report;
}

void GameCubeAdapterDevice::process(bool full_poll, bool send_events)
{
    // Inputs still update while there's no host, which never polls, so only skip frames with one
    if (tud_ready() && (!ready() || skip_report_this_frame()))
        return;

    GameCubeGamepad_Data_t ports[GAMECUBE_ADAPTER_PORTS];
    uint8_t count = std::min<size_t>(profiles.size(), GAMECUBE_ADAPTER_PORTS);
    for (uint8_t i = 0; i < count; i++)
    {
        const auto &profile = profiles[i];
        ports[i] = resting_report(profile->subtype);
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_gamecube(reinterpret_cast<uint8_t *>(&ports[i]));
        }
        for (const auto &led : profile->leds)
            led->update(full_poll, send_events);
    }
    gamecube_adapter_build_report(ports, count, m_report);
    // Once started, the real adapter answers every poll, changed or not, and hosts like Dolphin read it
    // that way. Before that it stays quiet, which matters: report 0x21 is declared as 37 bytes after its
    // ID, so a HID driver (Linux's usbhid) waits for 38, and a 37 byte packet fills the endpoint so isn't
    // short enough to end that. The next report would then overflow it (-EOVERFLOW, "input irq status
    // -75"). The real adapter does exactly this once started: the proposed Linux hid-gamecube-adapter
    // driver (2020, François-Xavier Carton) shrinks usbhid's URB to 37 bytes to get around it. So the
    // descriptor stays as it is, matching the hardware consoles expect.
    if (m_started && ready())
        send_report(sizeof(m_report), 0, m_report);
}

bool GameCubeAdapterDevice::winusb() const
{
    return std::any_of(profiles.begin(), profiles.end(), [](const auto &profile) { return profile->gamecube_adapter_winusb; });
}

// A real adapter gets the HID driver on Windows, which SDL reads it through. Dolphin only reads it
// through libusb though, which needs WinUSB, so a profile can ask for that instead
size_t GameCubeAdapterDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    if (!winusb())
        return 0;
    OS_COMPATIBLE_SECTION section = {
        FirstInterfaceNumber : interface_id,
        Reserved : 0x01,
        CompatibleID : "WINUSB",
        SubCompatibleID : {0},
        Reserved2 : {0}
    };
    assert(sizeof(section) <= remaining);
    if (sizeof(section) > remaining)
        return 0;
    memcpy(dest, &section, sizeof(section));
    return sizeof(section);
}

size_t GameCubeAdapterDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    const uint8_t descriptor[] = {
        // Interface
        9, TUSB_DESC_INTERFACE, interface_id, 0, 2, TUSB_CLASS_HID, 0, 0, 0,
        // HID
        9, HID_DESC_TYPE_HID, U16_TO_U8S_LE(0x0110), 0, 1, HID_DESC_TYPE_REPORT, U16_TO_U8S_LE(sizeof(desc_hid_report_gamecube_adapter)),
        // The real adapter's endpoint sizes, one report each
        7, TUSB_DESC_ENDPOINT, m_epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(GAMECUBE_ADAPTER_EP_IN_SIZE), 1,
        7, TUSB_DESC_ENDPOINT, m_epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(GAMECUBE_ADAPTER_EP_OUT_SIZE), 1};
    assert(sizeof(descriptor) <= remaining);
    if (sizeof(descriptor) > remaining)
        return 0;
    memcpy(dest, descriptor, sizeof(descriptor));
    return sizeof(descriptor);
}

size_t GameCubeAdapterDevice::device_name(uint8_t, char *)
{
    return 0;
}

void GameCubeAdapterDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = NINTENDO_VID;
    desc->idProduct = NINTENDO_GAMECUBE_ADAPTER_PID;
    // Windows only asks for the OS descriptors once per VID / PID / bcdDevice (it caches the answer under
    // usbflags), so the WinUSB version needs its own bcdDevice or turning it on would change nothing
    desc->bcdDevice = winusb() ? 0x0101 : 0x0100;
}

const uint8_t *GameCubeAdapterDevice::report_descriptor()
{
    return desc_hid_report_gamecube_adapter;
}

uint16_t GameCubeAdapterDevice::report_desc_len()
{
    return sizeof(desc_hid_report_gamecube_adapter);
}

uint16_t GameCubeAdapterDevice::get_report(uint8_t report_id, hid_report_type_t report_type,
                                           uint8_t *buffer, uint16_t reqlen)
{
    if (report_id != GAMECUBE_ADAPTER_INPUT_REPORT_ID || report_type != HID_REPORT_TYPE_INPUT || !buffer)
        return 0;
    uint16_t size = std::min<uint16_t>(reqlen, sizeof(m_report));
    memcpy(buffer, m_report, size);
    return size;
}

void GameCubeAdapterDevice::set_report(uint8_t, hid_report_type_t, uint8_t const *buffer, uint16_t bufsize)
{
    if (bufsize >= 1 && buffer[0] == GAMECUBE_ADAPTER_INIT)
    {
        m_started = true;
    }
    // An on / off byte per port, each going to its own profile, see profile_rumble
    else if (bufsize >= GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN && buffer[0] == GAMECUBE_ADAPTER_RUMBLE_REPORT_ID)
    {
        bool changed = false;
        bool any = false;
        for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
        {
            uint8_t rumble = buffer[1 + i] ? 0xFF : 0;
            changed |= rumble != m_port_rumble[i];
            any |= rumble;
            m_port_rumble[i] = rumble;
        }
        if (changed)
        {
            // The instance's own rumble is whether any port is, for anything not asking per profile
            rumble_left = rumble_right = any ? 0xFF : 0;
            update_feedback();
        }
    }
}

void GameCubeAdapterDevice::profile_rumble(size_t index, uint8_t &left, uint8_t &right) const
{
    left = right = index < GAMECUBE_ADAPTER_PORTS ? m_port_rumble[index] : 0;
}
