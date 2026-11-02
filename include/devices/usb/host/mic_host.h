#pragma once
#include "devices/usb/host/host.hpp"
#include "host/usbh.h"
#include "host/usbh_pvt.h"

// A USB Audio Class 1 microphone (or the microphone half of a headset), streamed into MicStream.
// Takes the Audio Control interface and the Audio Streaming interfaces after it.
class MicHost : public UsbHostInterface
{
public:
  ~MicHost();
  MicHost(uint8_t dev_addr, uint8_t interface, uint16_t id);
  bool set_config();
  bool xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
  void disconnect() override;
  static std::shared_ptr<UsbHostInterface> open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len);
  bool tick_digital(proto_Output &type) { return false; }
  uint16_t tick_analog(proto_Output &type) { return 0; }

private:
  static constexpr uint8_t MAX_RATES = 8;
  // The streaming alt setting picked from the descriptors
  uint8_t m_as_itf = 0;
  uint8_t m_alt = 0;
  uint8_t m_channels = 0;
  uint8_t m_subframe = 0;
  bool m_rate_control = false;
  // Discrete rates, or the min and max of a continuous range
  bool m_continuous = false;
  uint8_t m_rate_count = 0;
  uint32_t m_rates[MAX_RATES];
  uint32_t m_rate = 0;
  tusb_desc_endpoint_t m_ep;
  bool pick_rate();
  void queue_xfer();
  // Records from the isochronous ring: a 16 bit length, then the packet (0xFFFF is a lost packet)
  CFG_TUSB_MEM_ALIGN uint8_t m_buf[512];
  int16_t m_samples[256];
};
