
#pragma once
#include <queue>
#include "class/hid/hid.h"

#include "commands.pb.h"
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

class HIDDevice : public UsbDevice
{
public:
  virtual ~HIDDevice() {};
  virtual const uint8_t *report_descriptor() = 0;
  virtual uint16_t report_desc_len() = 0;
  virtual uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) = 0;
  virtual void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) = 0;
  bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
  bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
  uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
  bool send_report(uint8_t len, uint8_t report_id, void const *report);
  bool ready();

protected:
  const tusb_hid_descriptor_hid_t *hid_descriptor;
  uint8_t itf_protocol;  // Boot mouse or keyboard
  uint8_t protocol_mode; // Boot (0) or Report protocol (1)
  uint8_t idle_rate;     // up to application to handle idle rate
  bool clearedIn = false;
  bool clearedOut = false;
  uint8_t m_epin = 0;
  uint8_t m_epout = 0;
  uint8_t m_strid = 0;
  CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
  CFG_TUSB_MEM_ALIGN uint8_t ctrl_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
  uint8_t initialReport[CFG_TUD_XINPUT_TX_BUFSIZE];
  uint8_t lastReport[CFG_TUD_XINPUT_TX_BUFSIZE];
};

class HIDConfigDevice : public HIDDevice
{
public:
  HIDConfigDevice();
  void initialize();
  void process();
  static bool tool_closed();
  size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
  size_t config_descriptor(uint8_t *desc, size_t remaining);
  size_t device_name(uint8_t idx, char *desc);
  void device_descriptor(tusb_desc_device_t *desc);
  const uint8_t *report_descriptor();
  uint16_t report_desc_len();
  uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
  void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
  static bool send_event(proto_Event event);
  static std::shared_ptr<HIDConfigDevice> instance;

private:
  void process_events();
  void handle_command(proto_Command command);
  proto_EventList list;
  proto_PinDetectType m_detect_type;
  uint32_t detect_done = 0;
  uint32_t lastKeepAlive = 0;
  uint32_t start = 0;
  uint32_t selected_profile = 0;
  uint8_t current_pos = 1;
  uint32_t m_valid_pins;
  bool tool_seen = false;
  bool just_loaded = false;
  bool profile_selected = false;
  bool profile_changed = false;
  uint16_t last_adc_vals[NUM_ADC_CHANNELS];
  bool last_digital_vals[NUM_BANK0_GPIOS];
  uint8_t fw_update_tmp[256];
  proto_FirmwareUpdate update_state;
};
class HIDGamepadDevice : public HIDDevice
{
public:
  HIDGamepadDevice();
  void initialize();
  void process();
  size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
  size_t config_descriptor(uint8_t *desc, size_t remaining);
  size_t device_name(uint8_t idx, char *desc);
  void device_descriptor(tusb_desc_device_t *desc);
  const uint8_t *report_descriptor();
  uint16_t report_desc_len();
  uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
  void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);

private:
  int _FFB_effect_index;
  int _FFB_effect_duration;
  int _loop_count;
  int periodic_magnitude;
  int constant_force;
  int vibration_on;
};