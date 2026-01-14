
#pragma once

#include "class/hid/hid.h"

#include "usb/usb_descriptors.h"
#include "device.hpp"
#include "events.pb.h"

#include "tusb.h"

//--------------------------------------------------------------------+
// Class Driver Default Configure & Validation
//--------------------------------------------------------------------+

#if !defined(CFG_TUD_HID_EP_BUFSIZE) & defined(CFG_TUD_HID_BUFSIZE)
// TODO warn user to use new name later on
// #warning CFG_TUD_HID_BUFSIZE is renamed to CFG_TUD_HID_EP_BUFSIZE, please update to use the new name
#define CFG_TUD_HID_EP_BUFSIZE CFG_TUD_HID_BUFSIZE
#endif

#ifndef CFG_TUD_HID_EP_BUFSIZE
#define CFG_TUD_HID_EP_BUFSIZE 64
#endif
extern const uint8_t gh5_mapping[32];
extern const uint8_t dpad_bindings[11];
//--------------------------------------------------------------------+
// Internal Class Driver API
//--------------------------------------------------------------------+
void hidd_init(void);
bool hidd_deinit(void);
void hidd_reset(uint8_t rhport);
uint16_t hidd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc, uint16_t max_len);
bool hidd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
bool hidd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t event, uint32_t xferred_bytes);

uint16_t tud_hid_generic_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
void tud_hid_generic_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
bool tud_hid_generic_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
void tud_hid_generic_init(void);

class HIDDevice : public UsbDevice
{
public:
  uint8_t m_epin;
  uint8_t m_epout;

  CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t ctrl_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
  const tusb_hid_descriptor_hid_t *hid_descriptor;
  uint8_t itf_protocol;  // Boot mouse or keyboard
  uint8_t protocol_mode; // Boot (0) or Report protocol (1)
  uint8_t idle_rate;     // up to application to handle idle rate
  bool clearedIn;
  bool clearedOut;
  virtual const uint8_t *report_descriptor() = 0;
  virtual uint16_t report_desc_len() = 0;
  virtual uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) = 0;
  virtual void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) = 0;
};

class HIDConfigDevice : public HIDDevice
{
public:
  HIDConfigDevice();
  void initialize();
  void process(bool full_poll);
  bool tool_closed();
  size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
  size_t config_descriptor(uint8_t *desc, size_t remaining);
  void device_descriptor(tusb_desc_device_t *desc);
  const uint8_t *report_descriptor();
  uint16_t report_desc_len();
  uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
  void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
  static void send_event(proto_Event event);
  static void send_event_for(proto_Event event, uint32_t profile_id);

private:
  uint32_t lastKeepAlive = 0;
  uint32_t start = 0;
  uint32_t selected_profile = 0;
  static HIDConfigDevice* instance;
};
class HIDGamepadDevice : public HIDDevice
{
public:
  HIDGamepadDevice();
  void initialize();
  void process(bool full_poll);
  size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
  size_t config_descriptor(uint8_t *desc, size_t remaining);
  void device_descriptor(tusb_desc_device_t *desc);
  const uint8_t *report_descriptor();
  uint16_t report_desc_len();
  uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
  void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
};