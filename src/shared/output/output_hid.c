#include "output_hid.h"
#include "avr/wdt.h"
#include "output_keyboard.h"
#include "output_ps3.h"
#include "output_switch.h"

controller_t last_controller;
const void *hid_report_address;
uint16_t hid_report_size;
USB_ClassInfo_HID_Device_t interface = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : HID_EPADDR,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : NULL,
    PrevReportINBufferSize : 0,
  },
};

USB_Descriptor_Configuration_t configuration_descriptor = {
  Config : {
    Header : {
      Size : sizeof(USB_Descriptor_Configuration_Header_t),
      Type : DTYPE_Configuration
    },

    TotalConfigurationSize : sizeof(USB_Descriptor_Configuration_t),
    TotalInterfaces : 1,

    ConfigurationNumber : 1,
    ConfigurationStrIndex : NO_DESCRIPTOR,

    ConfigAttributes : USB_CONFIG_ATTR_RESERVED,

    MaxPowerConsumption : USB_CONFIG_POWER_MA(500)
  },

  HID_Interface : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : INTERFACE_ID_HID,
    AlternateSetting : 0x00,

    TotalEndpoints : 1,

    Class : HID_CSCP_HIDClass,
    SubClass : HID_CSCP_NonBootSubclass,
    Protocol : HID_CSCP_NonBootProtocol,

    InterfaceStrIndex : NO_DESCRIPTOR
  },

  HID_GamepadHID : {
    Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},

    HIDSpec : VERSION_BCD(1, 1, 1),
    CountryCode : 0x00,
    TotalReportDescriptors : 1,
    HIDReportType : HID_DTYPE_Report,
    HIDReportLength : 0
  },

  HID_ReportINEndpoint : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : HID_EPADDR,
    Attributes :
        (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
    EndpointSize : HID_EPSIZE,
    PollingIntervalMS : POLL_RATE
  }
};
USB_Descriptor_Device_t DeviceDescriptor = {
  Header : {Size : sizeof(USB_Descriptor_Device_t), Type : DTYPE_Device},

  USBSpecification : VERSION_BCD(2, 0, 0),
  Class : USB_CSCP_NoDeviceClass,
  SubClass : USB_CSCP_NoDeviceSubclass,
  Protocol : USB_CSCP_NoDeviceProtocol,
  Endpoint0Size : HID_EPSIZE,
  VendorID : 0x1209,
  ProductID : 0x2882,
  ReleaseNumber : 0x3122,

  ManufacturerStrIndex : 0x01,
  ProductStrIndex : 0x02,
  SerialNumStrIndex : 0x03,

  NumberOfConfigurations : 0x01
};

void hid_tick(controller_t controller) {
  USB_USBTask();
  wdt_reset();
  memcpy(&last_controller, &controller, sizeof(controller_t));
  HID_Device_USBTask(&interface);
}
void hid_control_request(void) { HID_Device_ProcessControlRequest(&interface); }
void hid_configuration_changed(void) {
  HID_Device_ConfigureEndpoints(&interface);
  USB_Device_EnableSOFEvents();
}

void hid_start_of_frame(void) { HID_Device_MillisecondElapsed(&interface); }
uint16_t hid_get_descriptor(const uint8_t DescriptorType,
                            const uint8_t DescriptorNumber,
                            const void **const DescriptorAddress,
                            uint8_t *const DescriptorMemorySpace) {
  uint8_t memorySpace = MEMSPACE_RAM;
  uint16_t Size = NO_DESCRIPTOR;
  const void *Address = NULL;
  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = sizeof(DeviceDescriptor);
    break;
  case DTYPE_Configuration:
    Address = &configuration_descriptor;
    Size = sizeof(USB_Descriptor_Configuration_t);
    break;
  case HID_DTYPE_HID:
    Address = &configuration_descriptor.HID_GamepadHID;
    Size = sizeof(USB_HID_Descriptor_HID_t);
    break;
  case HID_DTYPE_Report:
    Address = hid_report_address;
    Size = hid_report_size;
    memorySpace = MEMSPACE_FLASH;
    break;
  }
  *DescriptorAddress = Address;
  *DescriptorMemorySpace = memorySpace;
  return Size;
}
void hid_process_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                        uint8_t *const ReportID, const uint8_t ReportType,
                        void *ReportData, uint16_t *const ReportSize) {}
void hid_init(event_pointers *events) {
  events->tick = hid_tick;
  events->get_descriptor = hid_get_descriptor;
  events->control_request = hid_control_request;
  events->start_of_frame = hid_start_of_frame;
  events->configuration_changed = hid_configuration_changed;
  events->process_hid_report = hid_process_report;
  if (config.sub_type == SWITCH_GAMEPAD_SUBTYPE) {
    switch_init(events, &hid_report_address, &hid_report_size, &interface,
                &DeviceDescriptor);
  } else if (config.sub_type == KEYBOARD_SUBTYPE) {
    keyboard_init(events, &hid_report_address, &hid_report_size, &interface,
                  &DeviceDescriptor);
  } else if (config.sub_type >= PS3_GAMEPAD_SUBTYPE) {
    ps3_init(events, &hid_report_address, &hid_report_size, &interface,
             &DeviceDescriptor);
  }
  configuration_descriptor.HID_GamepadHID.HIDReportLength = hid_report_size;
}