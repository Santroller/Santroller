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

uint16_t HIDDevice::open(tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
  TU_VERIFY(TUSB_CLASS_HID == desc_itf->bInterfaceClass, 0);

  uint16_t const drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t) +
                                      desc_itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
  TU_VERIFY(max_len >= drv_len, 0);

  uint8_t const *p_desc = (uint8_t const *)desc_itf;
  p_desc = tu_desc_next(p_desc);
  TU_VERIFY(HID_DESC_TYPE_HID == tu_desc_type(p_desc), 0);
  hid_descriptor = (tusb_hid_descriptor_hid_t const *)p_desc;
  p_desc = tu_desc_next(p_desc);

  TU_VERIFY(usbd_open_edpt_pair(TUD_OPT_RHPORT, p_desc, desc_itf->bNumEndpoints, TUSB_XFER_INTERRUPT, &m_epout, &m_epin), 0);

  if (desc_itf->bInterfaceSubClass == HID_SUBCLASS_BOOT)
  {
    itf_protocol = desc_itf->bInterfaceProtocol;
  }

  protocol_mode = HID_PROTOCOL_REPORT;
  if (m_epout)
  {
    TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, CFG_TUD_HID_EP_BUFSIZE, false), drv_len);
  }

  return drv_len;
}
bool HIDDevice::ready()
{
  return tud_ready() && !usbd_edpt_busy(TUD_OPT_RHPORT, m_epin);
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
  return usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, len, false);
}
bool HIDDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
  {
    return true;
  }

  if (XFER_RESULT_SUCCESS == result)
  {
    set_report(epout_buf[0], HID_REPORT_TYPE_OUTPUT, epout_buf, (uint16_t)xferred_bytes);
  }

  TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, CFG_TUD_HID_EP_BUFSIZE, false));
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
    if (stage == CONTROL_STAGE_SETUP)
    {
      uint8_t const desc_type = tu_u16_high(request->wValue);

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
        return false;
      }
    }
  }
  else if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS)
  {
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
