#include "output_xinput.h"
#include "usb/Descriptors.h"
#include "usb/wcid.h"
#include <avr/wdt.h>
USB_XInputReport_Data_t gamepad_state;
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
  Endpoint_ConfigureEndpoint(HID_EPADDR_IN, EP_TYPE_INTERRUPT, 20, 1);
}
void xinput_start_of_frame(void) {}
void xinput_tick(controller_t controller) {
  /* Device must be connected and configured for the task to run */
  if (USB_DeviceState != DEVICE_STATE_Configured) return;

  /* Select the Joystick Report Endpoint */
  Endpoint_SelectEndpoint(HID_EPADDR_IN);

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
void xinput_init(event_pointers *events) {
  events->configuration_changed = xinput_configuration_changed;
  events->start_of_frame = xinput_start_of_frame;
  events->control_request = xinput_control_request;
  events->tick = xinput_tick;
  // ConfigurationDescriptor.Config.TotalConfigurationSize += sizeof(USB_HID_XBOX_Descriptor_HID_t);
  ConfigurationDescriptor.XInputUnknown.subtype = config.sub_type;
  // ConfigurationDescriptor.HID_Interface.Class = 0xFF;
  // ConfigurationDescriptor.HID_Interface.SubClass = 0x5D;
  // ConfigurationDescriptor.HID_Interface.Protocol = 0x01;
  memset(&gamepad_state, 0x00, sizeof(USB_XInputReport_Data_t));
  gamepad_state.rsize = 20;
}