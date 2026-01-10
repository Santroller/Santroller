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

static std::map<uint8_t, std::shared_ptr<HIDDevice>> hidDevicesByEpNum;
static std::map<uint8_t, std::shared_ptr<HIDDevice>> hidDevicesByItf;

void hidd_init(void)
{
  hidd_reset(0);
}

bool hidd_deinit(void)
{
  return true;
}

void hidd_reset(uint8_t rhport)
{
  (void)rhport;
  hidDevicesByEpNum.clear();
  hidDevicesByItf.clear();
}

uint16_t hidd_open(uint8_t rhport, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
  TU_VERIFY(TUSB_CLASS_HID == desc_itf->bInterfaceClass, 0);

  // len = interface + hid + n*endpoints
  uint16_t const drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t) +
                                      desc_itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
  TU_ASSERT(max_len >= drv_len, 0);

  uint8_t const *p_desc = (uint8_t const *)desc_itf;

  std::shared_ptr<HIDDevice> dev = std::static_pointer_cast<HIDDevice>(usb_instances[desc_itf->bInterfaceNumber]);
  //------------- HID descriptor -------------//
  p_desc = tu_desc_next(p_desc);
  TU_ASSERT(HID_DESC_TYPE_HID == tu_desc_type(p_desc), 0);
  dev->hid_descriptor = (tusb_hid_descriptor_hid_t const *)p_desc;

  //------------- Endpoint Descriptor -------------//
  p_desc = tu_desc_next(p_desc);

  TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, desc_itf->bNumEndpoints, TUSB_XFER_INTERRUPT, &dev->m_epout, &dev->m_epin), 0);

  if (desc_itf->bInterfaceSubClass == HID_SUBCLASS_BOOT)
  {
    dev->itf_protocol = desc_itf->bInterfaceProtocol;
  }

  dev->protocol_mode = HID_PROTOCOL_REPORT; // Per Specs: default is report mode

  // Prepare for output endpoint
  if (dev->m_epout)
  {
    TU_ASSERT(usbd_edpt_xfer(rhport, dev->m_epout, dev->epout_buf, CFG_TUD_HID_EP_BUFSIZE), drv_len);
    hidDevicesByEpNum[dev->m_epout] = dev;
  }
  if (dev->m_epin)
  {
    hidDevicesByEpNum[dev->m_epin] = dev;
  }
  hidDevicesByItf[desc_itf->bInterfaceNumber] = dev;

  return drv_len;
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool hidd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{

  if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT && request->bRequest == TUSB_REQ_CLEAR_FEATURE)
  {

    auto it = hidDevicesByEpNum.find(request->wIndex);
    if (it == hidDevicesByEpNum.end())
    {
      return false;
    }
    auto dev = it->second;
    dev->clearedIn |= tu_edpt_dir(request->wIndex) == TUSB_DIR_IN;
    dev->clearedOut |= tu_edpt_dir(request->wIndex) == TUSB_DIR_OUT;
  }
  TU_VERIFY(request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE);
  auto it = hidDevicesByItf.find(request->wIndex);
  if (it == hidDevicesByItf.end())
  {
    return false;
  }

  auto dev = it->second;
  if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD)
  {
    //------------- STD Request -------------//
    if (stage == CONTROL_STAGE_SETUP)
    {
      uint8_t const desc_type = tu_u16_high(request->wValue);
      // uint8_t const desc_index = tu_u16_low (request->wValue);

      if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESC_TYPE_HID)
      {
        TU_VERIFY(dev->hid_descriptor);
        TU_VERIFY(tud_control_xfer(rhport, request, (void *)(uintptr_t)dev->hid_descriptor, dev->hid_descriptor->bLength));
      }
      else if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESC_TYPE_REPORT)
      {
        TU_VERIFY(tud_control_xfer(rhport, request, (void *)(uintptr_t)dev->report_descriptor(), dev->report_desc_len()));
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

        uint8_t *report_buf = dev->ctrl_buf;
        uint16_t req_len = tu_min16(request->wLength, CFG_TUD_HID_EP_BUFSIZE);
        uint16_t xferlen = 0;

        xferlen += dev->get_report(report_id, (hid_report_type_t)report_type, report_buf, req_len);

        tud_control_xfer(rhport, request, dev->ctrl_buf, xferlen);
      }
      break;

    case HID_REQ_CONTROL_SET_REPORT:
      if (stage == CONTROL_STAGE_SETUP)
      {
        TU_VERIFY(request->wLength <= CFG_TUD_HID_EP_BUFSIZE);
        tud_control_xfer(rhport, request, dev->ctrl_buf, request->wLength);
      }
      else if (stage == CONTROL_STAGE_ACK)
      {
        uint8_t const report_type = tu_u16_high(request->wValue);
        uint8_t const report_id = tu_u16_low(request->wValue);

        uint8_t const *report_buf = dev->ctrl_buf;
        uint16_t report_len = tu_min16(request->wLength, CFG_TUD_HID_EP_BUFSIZE);

        dev->set_report(report_id, (hid_report_type_t)report_type, report_buf, report_len);
      }
      break;

    case HID_REQ_CONTROL_SET_IDLE:
      if (stage == CONTROL_STAGE_SETUP)
      {
        dev->idle_rate = tu_u16_high(request->wValue);
        tud_control_status(rhport, request);
      }
      break;

    case HID_REQ_CONTROL_GET_IDLE:
      if (stage == CONTROL_STAGE_SETUP)
      {
        tud_control_xfer(rhport, request, &dev->idle_rate, 1);
      }
      break;

    case HID_REQ_CONTROL_GET_PROTOCOL:
      if (stage == CONTROL_STAGE_SETUP)
      {
        tud_control_xfer(rhport, request, &dev->protocol_mode, 1);
      }
      break;

    case HID_REQ_CONTROL_SET_PROTOCOL:
      if (stage == CONTROL_STAGE_SETUP)
      {
        tud_control_status(rhport, request);
      }
      else if (stage == CONTROL_STAGE_ACK)
      {
        dev->protocol_mode = (uint8_t)request->wValue;
      }
      break;

    default:
      return false; // stall unsupported request
    }
  }
  else
  {
    return false; // stall unsupported request
  }

  return true;
}

bool hidd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{

  auto it = hidDevicesByEpNum.find(ep_addr);
  if (it == hidDevicesByEpNum.end())
  {
    return false;
  }
  auto dev = it->second;

  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
  {
    // Input report
    if (XFER_RESULT_SUCCESS == result)
    {
      // TODO: do we need report_complete at all?
    }
  }
  else
  {
    // Output report
    if (XFER_RESULT_SUCCESS == result)
    {
      dev->set_report(0, HID_REPORT_TYPE_OUTPUT, dev->epout_buf, (uint16_t)xferred_bytes);
    }

    // prepare for new transfer
    TU_ASSERT(usbd_edpt_xfer(rhport, dev->m_epout, dev->epout_buf, CFG_TUD_HID_EP_BUFSIZE));
  }

  return true;
}


const uint8_t gh5_mapping[] = {
    0x80, 0x15, 0x4D, 0x30, 0x9A, 0x99, 0x66,
    0x65, 0xC9, 0xC7, 0xC8, 0xC6, 0xAF, 0xAD,
    0xAE, 0xAC, 0xFF, 0xFB, 0xFD, 0xF9, 0xFE,
    0xFA, 0xFC, 0xF8, 0xE6, 0xE2, 0xE4, 0xE0,
    0xE5, 0xE1, 0xE3, 0xDF};
const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
uint8_t const desc_hid_report[] =
    {
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdLoaded)),
        TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad))};

uint8_t const desc_hid_report_non_gamepad[] =
    {
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdLoaded))};

HIDGamepadDevice::HIDGamepadDevice()
{
}
void HIDGamepadDevice::initialize()
{
}
void HIDGamepadDevice::process(bool full_poll)
{
  if (!tud_ready() || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
    return;
  PCGamepad_Data_t *report = (PCGamepad_Data_t *)epin_buf;
  for (const auto &mapping : mappings)
  {
    mapping->update(full_poll);
    mapping->update_hid(epin_buf);
  }

  // convert bitmask dpad to actual hid dpad
  report->dpad = dpad_bindings[report->dpad];
  if (current_type == GuitarHeroGuitar)
  {
    // convert bitmask slider to actual hid slider
    PCGuitarHeroGuitar_Data_t *reportGh = (PCGuitarHeroGuitar_Data_t *)report;
    reportGh->slider = gh5_mapping[reportGh->slider];
  }

  if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
  {
    return;
  }

  usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(PCGamepad_Data_t));
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
    m_epout = next_epin();
  }
  uint8_t desc[] = {TUD_HID_DESCRIPTOR(m_interface, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
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
}

uint16_t HIDGamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
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
  return;
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
  uint8_t desc[] = {TUD_HID_DESCRIPTOR(m_interface, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_non_gamepad), m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
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
      lastKeepAlive = millis();
      write_config(buffer, bufsize, start);
      start += bufsize;
      break;
    case ReportId::ReportIdConfigInfo:
      lastKeepAlive = millis();
      start = 0;
      write_config_info(buffer, bufsize);
      break;
    case ReportId::ReportIdLoaded:
      lastKeepAlive = millis();
      update(true);
      break;
    case ReportId::ReportIdKeepalive:
      lastKeepAlive = millis();
      break;
    case ReportId::ReportIdBootloader:
      reset_usb_boot(0, 0);
      break;
    case ReportId::ReportIdPs3F4:
      newMode = ModePs3;
      break;
    }
  }
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
  return millis() - lastKeepAlive > 500;
}

void HIDConfigDevice::send_event(proto_Event event)
{
  HIDConfigDevice* dev = HIDConfigDevice::instance;
  if (!dev) {
    return;
  }
  // Haven't received data from the tool recently so don't send out events for it
  if (dev->tool_closed() || !tud_ready())
  {
    return;
  }
  // Make sure events are always sent out
  while (!usbd_edpt_busy(TUD_OPT_RHPORT, dev->m_epin))
  {
    if (dev->tool_closed() || !tud_ready())
    {
      return;
    }
    tud_task();
  }
  dev->epin_buf[0] = ReportId::ReportIdConfig;
  pb_ostream_t outputStream = pb_ostream_from_buffer(dev->epin_buf + 1, 63);
  pb_encode(&outputStream, proto_Event_fields, &event);
  usbd_edpt_xfer(TUD_OPT_RHPORT, dev->m_epin, dev->epin_buf, outputStream.bytes_written + 1);
}

HIDConfigDevice* HIDConfigDevice::instance = nullptr;