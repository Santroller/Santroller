#pragma once
#include "../Controller.h"
class Output {
public:
  Output();
  bool ready();
  void init();
  void update(Controller controller);
  void usb_connect();
  void usb_disconnect();
  void usb_configuration_changed();
  void usb_control_request();
  void usb_start_of_frame();
  uint16_t get_descriptor(const uint8_t DescriptorType, const uint8_t DescriptorNumber, const void **const DescriptorAddress);
};