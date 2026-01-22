#include "tusb_option.h"
#include "usb/device/hid_device.h"
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

uint16_t HIDDevice::open(tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
  TU_VERIFY(TUSB_CLASS_HID == desc_itf->bInterfaceClass, 0);

  // len = interface + hid + n*endpoints
  uint16_t const drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t) +
                                      desc_itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
  TU_ASSERT(max_len >= drv_len, 0);

  uint8_t const *p_desc = (uint8_t const *)desc_itf;
  //------------- HID descriptor -------------//
  p_desc = tu_desc_next(p_desc);
  TU_ASSERT(HID_DESC_TYPE_HID == tu_desc_type(p_desc), 0);
  hid_descriptor = (tusb_hid_descriptor_hid_t const *)p_desc;

  //------------- Endpoint Descriptor -------------//
  p_desc = tu_desc_next(p_desc);

  TU_ASSERT(usbd_open_edpt_pair(TUD_OPT_RHPORT, p_desc, desc_itf->bNumEndpoints, TUSB_XFER_INTERRUPT, &m_epout, &m_epin), 0);

  if (desc_itf->bInterfaceSubClass == HID_SUBCLASS_BOOT)
  {
    itf_protocol = desc_itf->bInterfaceProtocol;
  }

  protocol_mode = HID_PROTOCOL_REPORT; // Per Specs: default is report mode

  // Prepare for output endpoint
  if (m_epout)
  {
    TU_ASSERT(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, CFG_TUD_HID_EP_BUFSIZE), drv_len);
  }

  return drv_len;
}
bool HIDDevice::ready()
{
  return tud_ready() && m_eps_assigned && !usbd_edpt_busy(TUD_OPT_RHPORT, m_epin);
}
bool HIDDevice::send_report(uint8_t len, uint8_t report_id, void const *report)
{
  if (report_id)
  {
    memcpy(epin_buf + 1, report, len);
    len += 1;
  }
  else
  {
    memcpy(epin_buf, report, len);
  }
  if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
  {
    return false;
  }
  return usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, len);
}
bool HIDDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{

  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
  {
    return true;
  }
  // Output report
  if (XFER_RESULT_SUCCESS == result)
  {
    set_report(0, HID_REPORT_TYPE_OUTPUT, epout_buf, (uint16_t)xferred_bytes);
  }

  // prepare for new transfer
  TU_ASSERT(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, CFG_TUD_HID_EP_BUFSIZE));

  return true;
}
bool HIDDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
  if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT && request->bRequest == TUSB_REQ_CLEAR_FEATURE)
  {
    clearedIn |= tu_edpt_dir(request->wIndex) == TUSB_DIR_IN;
    clearedOut |= tu_edpt_dir(request->wIndex) == TUSB_DIR_OUT;
    if (clearedIn && clearedOut)
    {
      newMode = ModeSwitch;
      return false;
    }
  }
  TU_VERIFY(request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE);
  if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
  {
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
    {
      if (request->bRequest == 6 && request->wValue == 0x4200)
      {
        newMode = ModeOgXbox;
        return false;
      }
    }
  }
  if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD)
  {
    //------------- STD Request -------------//
    if (stage == CONTROL_STAGE_SETUP)
    {
      uint8_t const desc_type = tu_u16_high(request->wValue);
      // uint8_t const desc_index = tu_u16_low (request->wValue);

      if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESC_TYPE_HID)
      {
        TU_VERIFY(hid_descriptor);
        TU_VERIFY(tud_control_xfer(TUD_OPT_RHPORT, request, (void *)(uintptr_t)hid_descriptor, hid_descriptor->bLength));
      }
      else if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESC_TYPE_REPORT)
      {
        TU_VERIFY(tud_control_xfer(TUD_OPT_RHPORT, request, (void *)(uintptr_t)report_descriptor(), report_desc_len()));
      }
      else
      {
        return false; // stall unsupported request
      }
    }
  }
  else if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS)
  {
    //------------- Class Specific Request -------------//
    switch (request->bRequest)
    {
    case HID_REQ_CONTROL_GET_REPORT:
      if (stage == CONTROL_STAGE_SETUP)
      {
        uint8_t const report_type = tu_u16_high(request->wValue);
        uint8_t const report_id = tu_u16_low(request->wValue);

        uint8_t *report_buf = ctrl_buf;
        uint16_t req_len = tu_min16(request->wLength, CFG_TUD_HID_EP_BUFSIZE);
        uint16_t xferlen = 0;

        xferlen += get_report(report_id, (hid_report_type_t)report_type, report_buf, req_len);

        tud_control_xfer(TUD_OPT_RHPORT, request, ctrl_buf, xferlen);
      }
      break;

    case HID_REQ_CONTROL_SET_REPORT:
      if (stage == CONTROL_STAGE_SETUP)
      {
        TU_VERIFY(request->wLength <= CFG_TUD_HID_EP_BUFSIZE);
        tud_control_xfer(TUD_OPT_RHPORT, request, ctrl_buf, request->wLength);
      }
      else if (stage == CONTROL_STAGE_ACK)
      {
        uint8_t const report_type = tu_u16_high(request->wValue);
        uint8_t const report_id = tu_u16_low(request->wValue);

        uint8_t const *report_buf = ctrl_buf;
        uint16_t report_len = tu_min16(request->wLength, CFG_TUD_HID_EP_BUFSIZE);

        set_report(report_id, (hid_report_type_t)report_type, report_buf, report_len);
      }
      break;

    case HID_REQ_CONTROL_SET_IDLE:
      if (stage == CONTROL_STAGE_SETUP)
      {
        idle_rate = tu_u16_high(request->wValue);
        tud_control_status(TUD_OPT_RHPORT, request);
      }
      break;

    case HID_REQ_CONTROL_GET_IDLE:
      if (stage == CONTROL_STAGE_SETUP)
      {
        tud_control_xfer(TUD_OPT_RHPORT, request, &idle_rate, 1);
      }
      break;

    case HID_REQ_CONTROL_GET_PROTOCOL:
      if (stage == CONTROL_STAGE_SETUP)
      {
        tud_control_xfer(TUD_OPT_RHPORT, request, &protocol_mode, 1);
      }
      break;

    case HID_REQ_CONTROL_SET_PROTOCOL:
      if (stage == CONTROL_STAGE_SETUP)
      {
        tud_control_status(TUD_OPT_RHPORT, request);
      }
      else if (stage == CONTROL_STAGE_ACK)
      {
        protocol_mode = (uint8_t)request->wValue;
      }
      break;

    default:
      return false;
    }
  }

  return true;
}

uint8_t const desc_hid_report[] =
    {TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad))};

uint8_t const desc_hid_report_non_gamepad[] =
    {
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdLoaded)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdSetActiveProfile)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdGetActiveProfiles))};

HIDGamepadDevice::HIDGamepadDevice()
{
}
void HIDGamepadDevice::initialize()
{
}
void HIDGamepadDevice::process(bool full_poll)
{
  if (!ready())
    return;
  PCGamepadDpad_Data_t *report = (PCGamepadDpad_Data_t *)epin_buf;
  memset(epin_buf, 0, sizeof(epin_buf));
  report->rid = ReportIdGamepad;
  report->rsize = sizeof(PCGamepadDpad_Data_t);
  for (const auto &mapping : mappings)
  {
    mapping->update(full_poll);
    mapping->update_hid(epin_buf);
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
  if (!m_eps_assigned)
  {
    m_eps_assigned = true;
    m_epin = next_epin();
    m_epout = next_epout();
    usb_instances_by_epnum[m_epin] = usb_instances[interface_id];
    usb_instances_by_epnum[m_epout] = usb_instances[interface_id];
  }
  uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
  assert(sizeof(desc) <= remaining);
  memcpy(dest, desc, sizeof(desc));
  return sizeof(desc);
}

void HIDGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
  // TODO: if we want to support multiple hid devices at once, this would need to be handled in some other way
  desc->bcdDevice = 0x0400 | subtype;
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

HIDConfigDevice::HIDConfigDevice()
{
  HIDConfigDevice::instance = this;
}
void HIDConfigDevice::initialize()
{
}
void HIDConfigDevice::process(bool full_poll)
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
  if (!profile_selected)
  {
    return;
  }
  auto selected = profiles.find(selected_profile);
  if (selected == profiles.end())
  {
    return;
  }
  for (auto &mapping : selected->second->mappings)
  {
    mapping->update(false);
  }
}

size_t HIDConfigDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
  return 0;
}

size_t HIDConfigDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
  if (!m_eps_assigned)
  {
    m_eps_assigned = true;
    m_epin = next_epin();
  }
  uint8_t desc[] = {TUD_HID_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_non_gamepad), m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
  assert(sizeof(desc) <= remaining);
  memcpy(dest, desc, sizeof(desc));
  return sizeof(desc);
}

void HIDConfigDevice::device_descriptor(tusb_desc_device_t *desc)
{
}
const uint8_t *HIDConfigDevice::report_descriptor()
{
  return desc_hid_report_non_gamepad;
}

uint16_t HIDConfigDevice::report_desc_len()
{
  return sizeof(desc_hid_report_non_gamepad);
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
      update(true);
      break;
    case ReportId::ReportIdKeepalive:
      lastKeepAlive = millis();
      tool_seen = true;
      break;
    case ReportId::ReportIdBootloader:
      reset_usb_boot(0, 0);
      break;
    case ReportId::ReportIdSetActiveProfile:
    {
      tool_seen = true;
      proto_SetProfileCommand cmd;
      pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
      if (!pb_decode(&inputStream, proto_SetProfileCommand_fields, &cmd))
      {
        printf("Didn't decode cmd?\r\n");
        break;
      }
      printf("Set id: %d\r\n", cmd.profileId);
      profile_selected = true;
      selected_profile = cmd.profileId;
      auto selected = profiles.find(selected_profile);
      if (selected == profiles.end())
      {
        break;
      }
      for (auto &mapping : selected->second->mappings)
      {
        mapping->update(true);
      }
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
  HIDConfigDevice *dev = HIDConfigDevice::instance;
  if (!dev || !dev->tool_seen)
  {
    return true;
  }
  return millis() - dev->lastKeepAlive > 500;
}

bool HIDConfigDevice::send_event_for(proto_Event event, uint32_t profile_id)
{

  HIDConfigDevice *dev = HIDConfigDevice::instance;
  if (!dev || dev->selected_profile != profile_id)
  {
    return false;
  }
  return HIDConfigDevice::send_event(event);
}

bool HIDConfigDevice::send_event(proto_Event event)
{
  HIDConfigDevice *dev = HIDConfigDevice::instance;
  if (!dev || !dev->m_eps_assigned || dev->tool_closed() || !tud_ready() || usbd_edpt_busy(TUD_OPT_RHPORT, dev->m_epin))
  {
    return false;
  }
  dev->epin_buf[0] = ReportId::ReportIdConfig;
  pb_ostream_t outputStream = pb_ostream_from_buffer(dev->epin_buf + 1, 63);
  pb_encode(&outputStream, proto_Event_fields, &event);
  usbd_edpt_xfer(TUD_OPT_RHPORT, dev->m_epin, dev->epin_buf, outputStream.bytes_written + 1);
  return true;
}

HIDConfigDevice *HIDConfigDevice::instance = nullptr;