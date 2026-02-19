#include "tusb_option.h"
#include "usb/device/hid_device.h"
#include "commands.pb.h"
#include "enums.pb.h"
#include "config.hpp"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "usb/device/ps3_device.h"
#include "usb/device/ps4_device.h"
#include "device/usbd.h"
#include "hid_reports.h"
#include "device/usbd_pvt.h"
#include "pico/bootrom.h"
#include "utils.h"

uint8_t const desc_hid_report[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad))};

HIDGamepadDevice::HIDGamepadDevice()
{
}
void HIDGamepadDevice::initialize()
{
  m_epin = next_epin();
  m_epout = next_epout();
  usb_instances_by_epnum[m_epin] = usb_instances[interface_id];
  usb_instances_by_epnum[m_epout] = usb_instances[interface_id];
}
void HIDGamepadDevice::process()
{
  if (!ready())
    return;
  PCGamepadDpad_Data_t *report = (PCGamepadDpad_Data_t *)epin_buf;
  memset(epin_buf, 0, sizeof(epin_buf));
  report->rid = ReportIdGamepad;
  report->rsize = sizeof(PCGamepadDpad_Data_t);

  for (const auto &profile : profiles)
  {
    for (const auto &mapping : profile->mappings)
    {
      mapping->update(false, false);
      mapping->update_hid(epin_buf);
    }
    for (const auto &led : profile->leds)
    {
      led->update(false, false);
    }
  }

  // convert bitmask dpad to actual hid dpad
  report->dpad = GamepadButtonMapping::dpad_bindings[report->dpad];
  if (current_type == GuitarHeroGuitar)
  {
    // convert bitmask slider to actual hid slider
    XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)report;
    reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
  }
  send_report(sizeof(XInputGamepad_Data_t), 0, epin_buf);
}

size_t HIDGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
  return 0;
}

size_t HIDGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
  uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
  assert(sizeof(desc) <= remaining);
  memcpy(dest, desc, sizeof(desc));
  return sizeof(desc);
}

void HIDGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
}
const uint8_t *HIDGamepadDevice::report_descriptor()
{
  return desc_hid_report;
}

uint16_t HIDGamepadDevice::report_desc_len()
{
  return sizeof(desc_hid_report);
}

void HIDGamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  if (report_type == HID_REPORT_TYPE_FEATURE)
  {
    switch (report_id)
    {
    case ReportId::ReportIdPs3F4:
      newMode = ModePs3;
      break;
    }
  }
}

uint16_t HIDGamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  if (report_type != HID_REPORT_TYPE_FEATURE)
  {
    return 0;
  }

  switch (report_id)
  {
  case ReportId::ReportIdPs3F2:
    newMode = ModePs3;
    return 0;
  case ReportId::ReportIdPs4Feature:
    if (ps4_based() && reqlen == 0x30)
    {
      // TODO: if theres a ps5 dongle plugged in here we shoud just jump straight to PS5 mode here
      newMode = ModePs4;
    }
    else
    {
      newMode = ModePs3;
    }
    return 0;
  }
  return 0;
}