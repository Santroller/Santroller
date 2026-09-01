#include "tusb_option.h"
#include <memory>
#include "managers/profile_manager.hpp"
#include "emulation/usb/hid_device.h"
#include "managers/config_manager.hpp"
#include "usb/auth_broker.h"
#include "commands.pb.h"
#include "enums.pb.h"
#include "config/config.hpp"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/ps3_device.h"
#include "emulation/usb/ps4_device.h"
#include "device/usbd.h"
#include "hid_reports.h"
#include "device/usbd_pvt.h"
#include "pico/bootrom.h"
#include "utils.h"

// Dance pads really need simultaneous directions, so they emulate buttons instead of hats
static uint8_t const desc_hid_report_buttons[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_BUTTONS)};

// for compatibility though, report the dpad as a hat for non-dancepad devices
static uint8_t const desc_hid_report_hat[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad), TUD_HID_REPORT_DESC_GAME_CONTROLLER_HAT_SWITCH)};

HIDGamepadDevice::HIDGamepadDevice()
{
}
void HIDGamepadDevice::initialize()
{
  m_epin = next_epin();
  m_epout = next_epout();
  m_strid = next_strid();
  ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
  ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);
  memset(&initialReport, 0, sizeof(initialReport));
  switch (subtype)
  {
  case RockBandDrums:
  {
    XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)&initialReport;
    report->redVelocity = -1;
    report->blueVelocity = -1;
    report->greenVelocity = 0;
    report->yellowVelocity = 0;
    break;
  }
  case GuitarHeroGuitar:
  {
    XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)&initialReport;
    report->whammy = INT16_MIN;
    break;
  }
  case LiveGuitar:
  {
    XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)&initialReport;
    report->whammy = INT16_MIN;
    break;
  }
  case GuitarHeroDrums:
  {
    XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)&initialReport;
    report->leftThumbClick = true;
    break;
  }
  default:
    break;
  }
  XInputGamepad_Data_t *gamepad = (XInputGamepad_Data_t *)initialReport;
  gamepad->rsize = sizeof(XInputGamepad_Data_t);
}
void HIDGamepadDevice::process()
{
    if (tud_suspended()) {
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
  // Deal with devices that don't have easy detection methods
  auto& detection = UsbDetectionState::instance();
  if (!mode_recently_changed() && detection.should_infer_console() && tud_connected())
  {
    // Switch 2 does read the hid descriptor
    if (detection.seen_hid_descriptor_read())
    {
      ConfigManager::instance().request_mode(ModeSwitch);
    }
    else if (tud_ready() && (subtype == RockBandGuitar || subtype == RockBandDrums))
    {
      // PS2 / Wii / WiiU does not read hid descriptor
      // The wii however will configure the usb device before it stops communicating
      ConfigManager::instance().request_mode(ModeWiiRb);
    }
    else
    {
      // But the PS2 does not. We also end up here on the wii/wiiu if a device does not have an explicit wii mode.
      ConfigManager::instance().request_mode(ModePs3);
    }
  }
  // if usb stack isnt ready, then we want to update inputs
  // we want to limit updates to usb speed though if it is ready
  if (tud_ready() && !ready()) {
    return;
  }
  PCGamepadDpad_Data_t *report = (PCGamepadDpad_Data_t *)epin_buf;
  memcpy(epin_buf, initialReport, sizeof(epin_buf));
  report->rid = ReportIdGamepad;
  report->rsize = sizeof(PCGamepadDpad_Data_t);

  for (const auto &profile : profiles)
  {
    profile->drum_state.blueCymbal = 0;
    profile->drum_state.bluePad = 0;
    profile->drum_state.yellowCymbal = 0;
    profile->drum_state.yellowPad = 0;
    profile->drum_state.greenCymbal = 0;
    profile->drum_state.greenPad = 0;
    profile->drum_state.redPad = 0;
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
  if (invert_y_axis_hid && subtype == Gamepad)
  {
    report->leftStickY = -report->leftStickY;
    report->rightStickY = -report->rightStickY;
  }
  // dance pads need to report the dpad as buttons, so skip the conversion to hat
  if (subtype != Dancepad)
  {
    // convert bitmask dpad to actual hid dpad
    report->dpad = GamepadButtonMapping::dpad_bindings[report->dpad];
  }
  if (subtype == GuitarHeroGuitar)
  {
    // convert bitmask slider to actual hid slider
    XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)report;
    reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
  }
  // if (memcmp(lastReport, epin_buf, sizeof(XInputGamepad_Data_t)) != 0)
  // {
  if (!ready()) {
    return;
  }
  send_report(sizeof(XInputGamepad_Data_t), 0, epin_buf);
  //   memcpy(lastReport, epin_buf, sizeof(XInputGamepad_Data_t));
  // }
}

size_t HIDGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
  return 0;
}

size_t HIDGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
  if (subtype == Dancepad)
  {
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, m_strid, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_buttons), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
  }
  else
  {
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, m_strid, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_hat), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
  }
}

size_t HIDGamepadDevice::device_name(uint8_t idx, char *desc)
{
  if (idx == m_strid)
  {
    memcpy(desc, profiles[0]->name, sizeof(profiles[0]->name));
    return sizeof(profiles[0]->name);
  }
  return 0;
}

void HIDGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
}
const uint8_t *HIDGamepadDevice::report_descriptor()
{
  UsbDetectionState::instance().mark_hid_descriptor_read();
  if (subtype == Dancepad)
  {
    return desc_hid_report_buttons;
  }
  else
  {
    return desc_hid_report_hat;
  }
}

uint16_t HIDGamepadDevice::report_desc_len()
{
  if (subtype == Dancepad)
  {
    return sizeof(desc_hid_report_buttons);
  }
  else
  {
    return sizeof(desc_hid_report_hat);
  }
}

void HIDGamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  // printf("set report %d %d %d\r\n", report_id, report_type, bufsize);
  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // if the host is asking for capabilities, send them
    if (report_id == ReportIdSantrollerCapabilities)
    {
      epin_buf[0] = ReportIdSantrollerCapabilities;
      epin_buf[1] = subtype;
      epin_buf[2] = capabilities;
      // Make sure this packet isn't dropped
      while (!ready())
      {
        tud_task();
      }
      send_report(3, 0, epin_buf);
    }
  }
  if (report_type == HID_REPORT_TYPE_FEATURE)
  {
    switch (report_id)
    {
    case ReportId::ReportIdPs3F4:
      ConfigManager::instance().request_mode(ModePs3);
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
    ConfigManager::instance().request_mode(ModePs3);
    return 0;
  case ReportId::ReportIdPs4Feature:
    if (supports_ps4 && reqlen == 0x30)
    {
      if (auth_broker.has_handler(ModePs5))
      {
        ConfigManager::instance().request_mode(ModePs5);
      }
      else
      {
        ConfigManager::instance().request_mode(ModePs4);
      }
    }
    else
    {
      ConfigManager::instance().request_mode(ModePs3);
    }
    return 0;
  }
  return 0;
}