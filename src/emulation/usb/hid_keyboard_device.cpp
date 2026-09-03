#include "tusb_option.h"
#include <memory>
#include "managers/profile_manager.hpp"
#include "emulation/usb/hid_device.h"
#include "commands.pb.h"
#include "enums.pb.h"
#include "config/config.hpp"
#include "main.hpp"
#include "device/usbd.h"
#include "hid_reports.h"
#include "device/usbd_pvt.h"
#include "pico/bootrom.h"
#include "utils.h"
#include <stdint.h>
#include <class/hid/hid_device.h>

// TODO: we dont really need to support nkro, we could just have our kro be like 10kro or something
uint8_t const desc_hid_keyboard_report[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

uint8_t const desc_hid_mouse_report[] = {TUD_HID_REPORT_DESC_MOUSE()};
HIDKeyboardDevice::HIDKeyboardDevice()
{
}
void HIDKeyboardDevice::initialize()
{
  m_epin = next_epin();
  m_epout = next_epout();
  m_strid = next_strid();
  ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
  ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);
  memset(&m_initial_report, 0, sizeof(m_initial_report));
}
void HIDKeyboardDevice::process(bool full_poll, bool send_events)
{
  if (tud_suspended())
  {
    for (const auto &profile : profiles)
    {
      for (const auto &led : profile->leds)
      {
        led->off();
      }
    }
    return;
  }
  if (!tud_ready())
  {
    for (const auto &profile : profiles)
    {
      for (const auto &led : profile->leds)
      {
        led->update(full_poll, send_events);
      }
    }
    return;
  }
  hid_keyboard_report_t *report = (hid_keyboard_report_t *)epin_buf;
  memcpy(epin_buf, m_initial_report, sizeof(epin_buf));
  for (const auto &profile : profiles)
  {
    auto &state = profile->keyboard_state;
    state.pressed_keys = 0;
    for (const auto &mapping : profile->mappings)
    {
      mapping->update(full_poll, send_events);
      mapping->update_hid(epin_buf);
    }
    for (const auto &led : profile->leds)
    {
      led->update(full_poll, send_events);
    }
    size_t current = 0;
    for (size_t i = 0; i < sizeof(state.last_seen_keys); i++)
    {
      if (state.last_seen_keys[i] && state.pressed_keys & (1 << state.last_seen_keys[i]))
      {
        report->keycode[current++] = state.last_seen_keys[i];
        state.pressed_keys &= ~(1 << state.last_seen_keys[i]);
      }
    }
    for (size_t i = 0; i < 255; i++)
    {
      if (current >= sizeof(report->keycode))
      {
        break;
      }
      if (state.pressed_keys & (1 << i))
      {
        report->keycode[current++] = i;
      }
    }
    memcpy(state.last_seen_keys, report->keycode, sizeof(report->keycode));
  }
  if (!ready())
  {
    return;
  }
  send_report(sizeof(hid_keyboard_report_t), 0, epin_buf);
}

size_t HIDKeyboardDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
  return 0;
}

size_t HIDKeyboardDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
  uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, m_strid, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_keyboard_report), m_epout, m_epin, sizeof(hid_keyboard_report_t), 1)};
  assert(sizeof(desc) <= remaining);
  memcpy(dest, desc, sizeof(desc));
  return sizeof(desc);
}

size_t HIDKeyboardDevice::device_name(uint8_t idx, char *desc)
{
  if (idx == m_strid)
  {
    memcpy(desc, profiles[0]->name, sizeof(profiles[0]->name));
    return sizeof(profiles[0]->name);
  }
  return 0;
}

void HIDKeyboardDevice::device_descriptor(tusb_desc_device_t *desc)
{
}
const uint8_t *HIDKeyboardDevice::report_descriptor()
{
  UsbDetectionState::instance().mark_hid_descriptor_read();
  return desc_hid_keyboard_report;
}

uint16_t HIDKeyboardDevice::report_desc_len()
{
  return sizeof(desc_hid_keyboard_report);
}

void HIDKeyboardDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
}

uint16_t HIDKeyboardDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}