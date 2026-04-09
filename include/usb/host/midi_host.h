#pragma once
#include "host.hpp"
#include "host/usbh.h"
#include "host/usbh_pvt.h"

class MidiHost : public UsbHostInterface
{
public:
  ~MidiHost();
  MidiHost(uint8_t dev_addr, uint8_t interface, uint16_t id);
  bool set_config();
  bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
  static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t* out_len);
  bool tick_digital(UsbButtonType type);
  uint16_t tick_analog(UsbAxisType type);

private:
  uint8_t m_ep_in;
  uint8_t m_ep_out;
  uint8_t m_ep_in_size;
  uint8_t m_ep_out_size;
  uint8_t rx_cable_count; // IN endpoint CS descriptor bNumEmbMIDIJack value
  uint8_t tx_cable_count; // OUT endpoint CS descriptor bNumEmbMIDIJack value
};