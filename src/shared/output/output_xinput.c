#include "output_xinput.h"
#include "usb/wcid.h"
#include <avr/wdt.h>
USB_XInputReport_Data_t gamepad_state;
extern USB_Descriptor_Device_t DeviceDescriptor;
Xinput_Descriptor_Configuration_t ConfigurationDescriptor = {
  Config : {
    Header : {
      Size : sizeof(USB_Descriptor_Configuration_Header_t),
      Type : DTYPE_Configuration
    },

    TotalConfigurationSize : sizeof(Xinput_Descriptor_Configuration_t),
    TotalInterfaces : 1,

    ConfigurationNumber : 1,
    ConfigurationStrIndex : NO_DESCRIPTOR,

    ConfigAttributes : USB_CONFIG_ATTR_REMOTEWAKEUP,

    MaxPowerConsumption : USB_CONFIG_POWER_MA(500)
  },

  Interface0 : {
    Header :
        {Size : sizeof(USB_Descriptor_Interface_t), Type : DTYPE_Interface},

    InterfaceNumber : 0,
    AlternateSetting : 0x00,

    TotalEndpoints : 2,

    Class : 0xFF,
    SubClass : 0x5D,
    Protocol : 0x01,

    InterfaceStrIndex : NO_DESCRIPTOR
  },

  XInputUnknown : {
    Header : {Size : sizeof(USB_HID_XBOX_Descriptor_HID_t), Type : 0x21},
    {0x10, 0x01},
    0,
    {0x25, 0x81, 0x14, 0x03, 0x03, 0x03, 0x04, 0x13, 0x02, 0x08, 0x03, 0x03}
  },

  DataInEndpoint0 : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : 0x81,
    Attributes : EP_TYPE_INTERRUPT,
    EndpointSize : XBOX_EPSIZE,
    PollingIntervalMS : 1
  },
  DataOutEndpoint0 : {
    Header : {Size : sizeof(USB_Descriptor_Endpoint_t), Type : DTYPE_Endpoint},

    EndpointAddress : 0x02,
    Attributes : EP_TYPE_INTERRUPT,
    EndpointSize : XBOX_EPSIZE,
    PollingIntervalMS : 1
  },
};
const USB_OSCompatibleIDDescriptor_t PROGMEM DevCompatIDs = {
  TotalLength : sizeof(USB_OSCompatibleIDDescriptor_t),
  Version : 0x0100,
  Index : EXTENDED_COMPAT_ID_DESCRIPTOR,
  TotalSections : 1,
  Reserved : {0},
  CompatID : {
    FirstInterfaceNumber : WCID_IF_NUMBER,
    Reserved : 0x04,
    CompatibleID : "XUSB10",
    SubCompatibleID : {0},
    Reserved2 : {0}
  }
};

void xinput_configuration_changed(void) {
  Endpoint_ConfigureEndpoint(JOYSTICK_EPADDR_IN, EP_TYPE_INTERRUPT, 20, 1);
  Endpoint_ConfigureEndpoint((ENDPOINT_DIR_IN | 3), EP_TYPE_INTERRUPT, 32, 1);
}
void xinput_start_of_frame(void) {}
void xinput_tick(controller_t controller) {
  USB_USBTask();
  wdt_reset();
  /* Device must be connected and configured for the task to run */
  if (USB_DeviceState != DEVICE_STATE_Configured) return;

  /* Select the Joystick Report Endpoint */
  Endpoint_SelectEndpoint(JOYSTICK_EPADDR_IN);

  /* Check to see if the host is ready for another packet */
  if (Endpoint_IsINReady()) {
    // We want to only overwrite the controller portion of the report, so we
    // work out what offset that is
    uint8_t start = offsetof(USB_XInputReport_Data_t, digital_buttons_1);
    uint8_t *casted = (uint8_t *)&controller;
    uint8_t *casted_state = (uint8_t *)(&gamepad_state) + start;
    memcpy(casted_state, casted, sizeof(controller));
    /* Write Joystick Report Data */
    Endpoint_Write_Stream_LE(&gamepad_state, 20, NULL);

    /* Finalize the stream transfer to send the last packet */
    Endpoint_ClearIN();
  }
}
void sendControl(uint8_t *out, uint8_t outSize) {
  Endpoint_ClearStall();
  Endpoint_ClearSETUP();
  /* Write the report data to the control endpoint */
  Endpoint_Write_Control_Stream_LE(out, outSize);
  Endpoint_ClearOUT();
}
void xinput_control_request(void) {
  /* Handle HID Class specific requests */
  switch (USB_ControlRequest.bRequest) {
  case HID_REQ_GetReport:
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
      Endpoint_ClearSETUP();

      /* Write the report data to the control endpoint */
      Endpoint_Write_Control_Stream_LE(&gamepad_state, 20);
      Endpoint_ClearOUT();
    }

    if (USB_ControlRequest.wLength == 0x04) {
      uint8_t data[] = {0x00, 0x12, 0x28, 0x61}; // DeviceID
      sendControl(data, sizeof(data));
    }
    if (USB_ControlRequest.wLength == 8 &&
        USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) {
      uint8_t data[] = {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      sendControl(data, sizeof(data));
    }
    if (USB_ControlRequest.wLength == 20 &&
        USB_ControlRequest.bmRequestType ==
            (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) {
      uint8_t data[20] = {0x00, 0x14, 0x3f, 0xf7, 0xff, 0xff, 0x00,
                          0x00, 0x00, 0x00, 0xc0, 0xff, 0xc0, 0xff,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Capabilities
      sendControl(data, sizeof(data));
    }

    break;
  case REQ_GetOSFeatureDescriptor:
    if ((USB_ControlRequest.bmRequestType &
         (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
            (REQDIR_DEVICETOHOST | REQTYPE_VENDOR) &&
        USB_ControlRequest.wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
      Endpoint_ClearSETUP();
      Endpoint_Write_Control_PStream_LE(&DevCompatIDs,
                                        DevCompatIDs.TotalLength);
      Endpoint_ClearOUT();
    }
  }
}
uint16_t xinput_get_descriptor(const uint8_t DescriptorType,
                               const uint8_t DescriptorNumber,
                               const void **const DescriptorAddress,
                               uint8_t *const DescriptorMemorySpace) {
  uint16_t Size = NO_DESCRIPTOR;
  const void *Address = NULL;
  uint8_t MemorySpace = MEMSPACE_RAM;
  switch (DescriptorType) {
  case DTYPE_Device:
    Address = &DeviceDescriptor;
    Size = sizeof(DeviceDescriptor);
    break;
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = sizeof(ConfigurationDescriptor);
    break;
  }
  *DescriptorMemorySpace = MemorySpace;
  *DescriptorAddress = Address;
  return Size;
}
void xinput_init(event_pointers *events) {
  events->configuration_changed = xinput_configuration_changed;
  events->start_of_frame = xinput_start_of_frame;
  events->control_request = xinput_control_request;
  events->tick = xinput_tick;
  events->get_descriptor = xinput_get_descriptor;
  ConfigurationDescriptor.DataInEndpoint0.PollingIntervalMS = config.pollrate;
  ConfigurationDescriptor.DataOutEndpoint0.PollingIntervalMS = config.pollrate;
  ConfigurationDescriptor.XInputUnknown.subtype = config.sub_type;
  memset(&gamepad_state, 0x00, sizeof(USB_XInputReport_Data_t));
  gamepad_state.rsize = 20;
}