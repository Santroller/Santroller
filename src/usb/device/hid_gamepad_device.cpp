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
  if (!ready())
    return;
  PCGamepadDpad_Data_t *report = (PCGamepadDpad_Data_t *)epin_buf;
  memcpy(epin_buf, initialReport, sizeof(epin_buf));
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
  if (invert_y_axis_hid && subtype == Gamepad) {
    report->leftStickY = -report->leftStickY;
    report->rightStickY = -report->rightStickY;
  }

  // convert bitmask dpad to actual hid dpad
  report->dpad = GamepadButtonMapping::dpad_bindings[report->dpad];
  if (subtype == GuitarHeroGuitar)
  {
    // convert bitmask slider to actual hid slider
    XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)report;
    reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
  }
  // if (memcmp(lastReport, epin_buf, sizeof(XInputGamepad_Data_t)) != 0)
  // {

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
    case REPORT_CREATE_EFFECT:
      _FFB_effect_index = buffer[1];
      printf(("create effect %d\n"), buffer[1]);
      break;
    }
  }
  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    switch (buffer[0])
    {
    case REPORT_SET_STATUS:
      printf(("eff. set stat 0x%02x 0x%02x\r\n"), buffer[1], buffer[2]);
      break;
    case REPORT_EFFECT_BLOCK_IDX:
      printf(("eff. blk. idx %d\r\n"), buffer[1]);
      break;
    case REPORT_DISABLE_ACTUATORS:
      printf(("disable actuators\r\n"));
      periodic_magnitude = 0;
      constant_force = 0;
      vibration_on = 0;
      break;
    case REPORT_PID_POOL:
      printf(("pid pool\r\n"));
      break;
    case REPORT_SET_EFFECT:
      _FFB_effect_index = buffer[1];
      _FFB_effect_duration = buffer[3] | (buffer[4] << 8);
      printf(("set effect %d. duration: %u\r\n"), buffer[1], _FFB_effect_duration);
      break;
    case REPORT_SET_PERIODIC:
      periodic_magnitude = buffer[2];
      printf(("Set periodic - mag: %d, period: %u\r\n"), buffer[2], buffer[5] | (buffer[6] << 8));
      break;
    case REPORT_SET_CONSTANT_FORCE:
      if (buffer[1] == 1)
      {
        constant_force = buffer[2];
        printf(("Constant force %d\r\n"), buffer[2]);
      }
      break;
    case REPORT_EFFECT_OPERATION:
      if (bufsize != 4)
      {
        printf(("Hey!\r\n"));
        return;
      }
      /* Byte 0 : report ID
       * Byte 1 : bit 7=rom flag, bits 6-0=effect block index
       * Byte 2 : Effect operation
       * Byte 3 : Loop count */

      printf(("EFFECT OP: rom=%s, idx=0x%02x : "), buffer[1] & 0x80 ? "Yes" : "No", buffer[1] & 0x7F);

      // With dolphin, an "infinite" duration is set. The effect is started, then never
      // stopped. Maybe I misunderstood something? In any case, the following works
      // and feels about right.
      if (_FFB_effect_duration == 0xffff)
      {
        if (buffer[3])
        {
          _loop_count = buffer[3] + 1; // +1 for a bit more strength
        }
        else
        {
          _loop_count = 0;
        }
      }
      else
      {
        // main.c uses a 16ms interval timer for vibration "loops"
        _loop_count = (_FFB_effect_duration / 16) * buffer[3];
        printf(("%d loops for %d ms\r\n"), buffer[3], _loop_count * 16);
      }

      switch (buffer[1] & 0x7F) // Effect block index
      {
      case 1:              // constant force
      case 3:              // square
      case 4:              // sine
        switch (buffer[2]) // effect operation
        {
        case EFFECT_OP_START:
          printf(("Start (lp=%d)\r\n"), _loop_count);
          vibration_on = 1;
          break;

        case EFFECT_OP_START_SOLO:
          printf(("Start solo (lp=%d)\r\n"), _loop_count);
          vibration_on = 1;
          break;

        case EFFECT_OP_STOP:
          printf(("Stop (lp=%d)\r\n"), _loop_count);
          vibration_on = 0;
          break;
        default:
          printf(("OP?? %02x (lp=%d)\r\n"), buffer[2], _loop_count);
          break;
        }
        break;

      // TODO : should probably drop these from the descriptor since they are
      default:
      case 2:  // ramp
      case 5:  // triangle
      case 6:  // sawtooth up
      case 7:  // sawtooth down
      case 8:  // spring
      case 9:  // damper
      case 10: // inertia
      case 11: // friction
      case 12: // custom force data
        printf(("Ununsed effect %d\n"), buffer[1] & 0x7F);
        break;
      }
      break;
    default:
      printf(("Set output report 0x%02x\r\n"), buffer[0]);
    }
  }
}

uint16_t HIDGamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  if (report_type == HID_REPORT_TYPE_INPUT)
  {
    if (report_id == REPORT_SET_STATUS)
    { // 2 : ES playing
      buffer[0] = report_id;
      buffer[1] = 0;
      buffer[2] = _FFB_effect_index;
      printf("ES playing\r\n");
      return 3;
    }
  }
  if (report_type != HID_REPORT_TYPE_FEATURE)
  {
    return 0;
  }
  if (report_id == PID_BLOCK_LOAD_REPORT)
  {
    buffer[0] = report_id;
    buffer[1] = 0x1; // Effect block index
    buffer[2] = 0x1; // (1: success, 2: oom, 3: load error)
    buffer[3] = 10;
    buffer[4] = 10;
    printf(("block load\r\n"));
    return 5;
  }
  else if (report_id == PID_SIMULTANEOUS_MAX)
  {
    buffer[0] = report_id;
    // ROM Effect Block count
    buffer[1] = 0x1;
    buffer[2] = 0x1;
    // PID pool move report?
    buffer[3] = 0xff;
    buffer[4] = 1;
    printf(("simultaneous max\r\n"));
    return 5;
  }
  else if (report_id == REPORT_CREATE_EFFECT)
  {
    buffer[0] = report_id;
    buffer[1] = 1;
    printf(("create effect\r\n"));
    return 2;
  }
  switch (report_id)
  {
  case ReportId::ReportIdPs3F2:
    newMode = ModePs3;
    return 0;
  case ReportId::ReportIdPs4Feature:
    if (supports_ps4 && reqlen == 0x30)
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