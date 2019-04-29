#pragma once
#include "../Controller.h"
#include <stdlib.h>
class Output {
public:
  virtual void init() = 0;
  virtual void update(Controller controller) = 0;
  virtual void usb_connect() = 0;
  virtual void usb_disconnect() = 0;
  virtual void usb_configuration_changed() = 0;
  virtual void usb_control_request() = 0;
  virtual void usb_start_of_frame() = 0;
  virtual uint16_t get_descriptor(const uint8_t DescriptorType,
                                  const uint8_t DescriptorNumber,
                                  const void **const DescriptorAddress,
                                  uint8_t *const DescriptorMemorySpace) = 0;
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *ptr) { free(ptr); }
  static Output *output;
};
