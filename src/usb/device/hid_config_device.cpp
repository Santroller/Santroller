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
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "math.h"

uint8_t const desc_hid_report_config[] =
    {
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdLoaded)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdCommand)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(1, HID_REPORT_ID(ReportIdKeepalive)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdBootloader)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdGetActiveProfiles))};

HIDConfigDevice::HIDConfigDevice()
{
}
void HIDConfigDevice::initialize()
{
  m_epin = next_epin();
}
void HIDConfigDevice::process()
{
  if (clearedIn && clearedOut)
  {
    newMode = ModeSwitch;
  }
  if (tool_closed())
  {
    profile_selected = false;
    return;
  }
  if (just_loaded)
  {
    for (const auto &device : active_devices)
    {
      device->update(true, true);
    }
    just_loaded = false;
  }
  if (profile_selected)
  {
    auto selected = all_profiles.find(selected_profile);
    if (selected == all_profiles.end())
    {
      return;
    }
    if (detect_done)
    {

      switch (m_detect_type)
      {
      case DetectDigital:
        for (uint8_t i = 0; i < NUM_BANK0_GPIOS; i++)
        {
          if (m_valid_pins & (1 << i) && gpio_get(i) != last_digital_vals[i])
          {
            printf("detected digital: %d %d %d\r\n", i, gpio_get(i), last_digital_vals[i]);
            detect_done = 0;
            proto_Event evt;
            evt.which_event = proto_Event_pin_tag;
            evt.event.pin.pin = i;
            send_event(evt);
            break;
          }
        }
        break;
      case DetectAnalog:
        for (uint8_t i = 0; i < NUM_ADC_CHANNELS; i++)
        {
          if (m_valid_pins & (1 << i))
          {
            adc_select_input(i);
            if (abs(last_adc_vals[i] - adc_read()) > 100)
            {
              // found!
              printf("detected adc: %d %d %d\r\n", last_adc_vals[i], adc_read(), i + ADC_BASE_PIN);
              detect_done = 0;
              proto_Event evt;
              evt.which_event = proto_Event_pin_tag;
              evt.event.pin.pin = i + ADC_BASE_PIN;
              send_event(evt);
              break;
            }
          }
        }
        break;
      }

      if (profile_changed || (detect_done && millis() > detect_done))
      {
        detect_done = 0;
      }
      if (!detect_done)
      {
        switch (m_detect_type)
        {
        case DetectDigital:
          for (uint8_t i = 0; i < NUM_BANK0_GPIOS; i++)
          {
            if (m_valid_pins & (1 << i))
            {
              gpio_init(i);
              gpio_set_dir(i, false);
              gpio_set_pulls(i, false, false);
            }
          }
          break;
        case DetectAnalog:
          for (uint8_t i = 0; i < NUM_ADC_CHANNELS; i++)
          {
            if (m_valid_pins & (1 << i))
            {
              adc_gpio_init(i + ADC_BASE_PIN);
              gpio_set_pulls(i + ADC_BASE_PIN, false, false);
            }
          }
          break;
        }
        for (auto &mapping : selected->second->mappings)
        {
          mapping->reload();
        }
        for (auto &led : selected->second->leds)
        {
          led->reload();
        }
      }
    }
    else
    {
      if (profile_changed)
      {
        for (const auto &device : active_devices)
        {
          device->update(profile_changed, true);
        }
      }
      for (const auto &mapping : selected->second->mappings)
      {
        mapping->update(profile_changed, true);
      }
      for (const auto &mapping : selected->second->triggers)
      {
        mapping->validate(false, true, true);
      }
      for (const auto &led : selected->second->leds)
      {
        led->update(true, true);
      }
    }
  }
  if (list.event_count == 0 || !tud_ready() || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
  {
    return;
  }

  epin_buf[0] = ReportId::ReportIdConfig;
  pb_ostream_t outputStream = pb_ostream_from_buffer(epin_buf + 1, 63);
  if (pb_encode(&outputStream, proto_EventList_fields, &list))
  {
    usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, outputStream.bytes_written + 1);
  }
  list.event_count = 0;

  profile_changed = false;
}

size_t HIDConfigDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
  return 0;
}

size_t HIDConfigDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
  uint8_t desc[] = {TUD_HID_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_config), m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
  assert(sizeof(desc) <= remaining);
  memcpy(dest, desc, sizeof(desc));
  return sizeof(desc);
}

void HIDConfigDevice::device_descriptor(tusb_desc_device_t *desc)
{
}
const uint8_t *HIDConfigDevice::report_descriptor()
{
  return desc_hid_report_config;
}

uint16_t HIDConfigDevice::report_desc_len()
{
  return sizeof(desc_hid_report_config);
}

void HIDConfigDevice::handle_command(proto_Command command)
{
  switch (command.which_command)
  {
  case proto_Command_setProfile_tag:
  {
    printf("Set id: %d\r\n", command.command.setProfile.profileId);
    profile_selected = true;
    profile_changed = true;
    selected_profile = command.command.setProfile.profileId;
  }
  break;
  case proto_Command_detectPin_tag:
  {
    // only detect for 10 seconds

    printf("detect: %d\r\n", command.command.detectPin.detectType);
    detect_done = millis() + 10000;
    m_valid_pins = 0;
    m_detect_type = command.command.detectPin.detectType;
    switch (command.command.detectPin.detectType)
    {
    case DetectDigital:
      for (uint8_t i = 0; i < NUM_BANK0_GPIOS; i++)
      {
        if (i == 23) {
          continue;
        }
        bool found = false;
        for (auto &device : active_devices)
        {
          if (device->using_pin(i))
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          m_valid_pins |= 1 << i;
          gpio_init(i);
          gpio_set_dir(i, false);
          gpio_set_pulls(i, true, false);
          sleep_us(1);
          last_digital_vals[i] = gpio_get(i);
        }
      }
      break;
    case DetectAnalog:
      for (uint8_t i = 0; i < NUM_ADC_CHANNELS; i++)
      {
        bool found = false;
        for (auto &device : active_devices)
        {
          if (device->using_pin(i + ADC_BASE_PIN))
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          m_valid_pins |= 1 << i;
          adc_gpio_init(i + ADC_BASE_PIN);
          gpio_set_pulls(i + ADC_BASE_PIN, true, false);
          adc_select_input(i);
          sleep_us(10);
          last_adc_vals[i] = adc_read();
        }
      }
      break;
    }
  }
  break;
  }
}

void HIDConfigDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  if (report_type == HID_REPORT_TYPE_FEATURE)
  {
    // skip over report id
    buffer++;
    bufsize--;
    switch (report_id)
    {
    case ReportId::ReportIdConfig:
      tool_seen = true;
      lastKeepAlive = millis();
      write_config(buffer, bufsize, start);
      start += bufsize;
      break;
    case ReportId::ReportIdConfigInfo:
      lastKeepAlive = millis();
      tool_seen = true;
      start = 0;
      write_config_info(buffer, bufsize);
      break;
    case ReportId::ReportIdLoaded:
      lastKeepAlive = millis();
      tool_seen = true;
      just_loaded = true;
      break;
    case ReportId::ReportIdKeepalive:
      lastKeepAlive = millis();
      tool_seen = true;
      break;
    case ReportId::ReportIdBootloader:
      reset_usb_boot(0, 0);
      break;
    case ReportId::ReportIdCommand:
    {
      tool_seen = true;
      proto_Command cmd;
      pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
      if (!pb_decode_delimited(&inputStream, proto_Command_fields, &cmd))
      {
        printf("Didn't decode cmd?\r\n");
        break;
      }
      handle_command(cmd);
      break;
    }
    case ReportId::ReportIdPs3F4:
      newMode = ModePs3;
      break;
    }
  }
}

bool encode_int32_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
  for (auto &profile : active_profiles)
  {
    if (!pb_encode_tag_for_field(stream, field))
      return false;

    if (!pb_encode_varint(stream, profile))
      return false;
  }
  return true;
}

uint16_t HIDConfigDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
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
  case ReportId::ReportIdConfig:
  {
    buffer[0] = report_id;
    buffer++;
    uint32_t ret = copy_config(buffer, start);
    start += ret;
    return ret + 1;
  }
  case ReportId::ReportIdGetActiveProfiles:
  {
    buffer[0] = report_id;
    buffer++;
    auto stream = pb_ostream_from_buffer(buffer, reqlen);
    proto_GetActiveProfiles resp;
    resp.profiles.funcs.encode = encode_int32_array;
    if (!pb_encode(&stream, proto_GetActiveProfiles_fields, &resp))
      return 1;
    return stream.bytes_written + 1;
  }
  case ReportId::ReportIdConfigInfo:
    buffer[0] = report_id;
    buffer++;
    start = 0;
    return copy_config_info(buffer) + 1;
  case ReportId::ReportIdPs3F2:
    newMode = ModePs3;
    return 0;
  case ReportId::ReportIdPs4Feature:
    if (ps4_based())
    {
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

bool HIDConfigDevice::tool_closed()
{
  auto dev = HIDConfigDevice::instance;
  if (!dev || !dev->tool_seen)
  {
    return true;
  }
  return millis() - dev->lastKeepAlive > 500;
}

bool HIDConfigDevice::send_event(proto_Event event)
{
  auto dev = HIDConfigDevice::instance;
  if (tool_closed() || dev->list.event_count >= TU_ARRAY_SIZE(dev->list.event))
  {
    return false;
  }
  dev->list.event[dev->list.event_count++] = event;
  return true;
}

std::shared_ptr<HIDConfigDevice> HIDConfigDevice::instance = std::make_shared<HIDConfigDevice>();