#pragma once
#include "../../config/eeprom.h"
#include "Output.h"
#include <LUFA/Drivers/USB/USB.h>
#include <avr/wdt.h>
enum InterfaceDescriptors_t { INTERFACE_ID_HID = 0 };
#define HID_EPADDR (ENDPOINT_DIR_IN | 1)
#define HID_EPSIZE 64
#define HID_REPORTSIZE 64
typedef struct {
  USB_Descriptor_Configuration_Header_t Config;
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t HID_GamepadHID;
  USB_Descriptor_Endpoint_t HID_ReportINEndpoint;
} USB_Descriptor_Configuration_t;

class HIDOutput : public Output {
public:
  virtual USB_ClassInfo_HID_Device_t *createHIDInterface() = 0;
  const virtual size_t ReportDatatypeSize() = 0;
  USB_ClassInfo_HID_Device_t *HID_Interface;
  USB_Descriptor_Configuration_t ConfigurationDescriptor;
  void init();
  void usb_connect();
  void usb_disconnect();
  void usb_configuration_changed();
  void usb_start_of_frame();
  virtual void update(Controller controller) = 0;
  virtual bool
  hid_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                    uint8_t *const ReportID, const uint8_t ReportType,
                    void *ReportData, uint16_t *const ReportSize) = 0;
  uint16_t get_descriptor(const uint8_t DescriptorType,
                          const uint8_t DescriptorNumber,
                          const void **const DescriptorAddress,
                          uint8_t *const DescriptorMemorySpace);
  virtual uint16_t get_hid_descriptor(const uint8_t DescriptorType,
                          const uint8_t DescriptorNumber,
                          const void **const DescriptorAddress,
                          uint8_t *const DescriptorMemorySpace) = 0;
};