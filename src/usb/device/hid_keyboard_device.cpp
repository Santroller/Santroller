#include "tusb_option.h"
#include "usb/device/hid_device.h"
#include "commands.pb.h"
#include "enums.pb.h"
#include "config.hpp"
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
  usb_instances_by_epin[m_epin & (~0x80)] = usb_instances[interface_id];
  usb_instances_by_epout[m_epout] = usb_instances[interface_id];
  memset(&initialReport, 0, sizeof(initialReport));
}
void HIDKeyboardDevice::process()
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
        led->update(false, false);
      }
    }
    return;
  }
  hid_keyboard_report_t *report = (hid_keyboard_report_t *)epin_buf;
  memcpy(epin_buf, initialReport, sizeof(epin_buf));
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
  seenHidDescriptorRead = true;
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