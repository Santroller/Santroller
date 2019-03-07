#include "XInputOutput.h"
void XInputOutput::usb_connect() {}
void XInputOutput::usb_disconnect() {}

XInputOutput::XInputOutput() {
  memset(&gamepad_state, 0x00, sizeof(USB_JoystickReport_Data_t));
  gamepad_state.rsize = 20;
}
void XInputOutput::init() {
  wdt_enable(WDTO_2S);
  USB_Init();
  sei();
}
void XInputOutput::usb_configuration_changed() {
  Endpoint_ConfigureEndpoint(JOYSTICK_EPADDR_IN, EP_TYPE_INTERRUPT, 20, 1);
  Endpoint_ConfigureEndpoint((ENDPOINT_DIR_IN | 3), EP_TYPE_INTERRUPT, 32, 1);
}
void XInputOutput::usb_control_request() {
  const void *DescriptorPointer;
  uint16_t DescriptorSize;
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

    break;
  case REQ_GetOSFeatureDescriptor:
    if ((USB_ControlRequest.bmRequestType &
         (CONTROL_REQTYPE_DIRECTION | CONTROL_REQTYPE_TYPE)) ==
        (REQDIR_DEVICETOHOST | REQTYPE_VENDOR)) {

      DescriptorSize = USB_GetOSFeatureDescriptor(
          USB_ControlRequest.wValue >> 8, USB_ControlRequest.wIndex,
          USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT,
          &DescriptorPointer);
      if (DescriptorSize == NO_DESCRIPTOR)
        return;
      Endpoint_ClearSETUP();
      Endpoint_Write_Control_PStream_LE(DescriptorPointer, DescriptorSize);
      Endpoint_ClearOUT();
    }
  }
}
void XInputOutput::usb_start_of_frame() {}
void XInputOutput::update(Controller controller) {
  bit_write(bit_check(controller.buttons, UP), gamepad_state.digital_buttons_1,
            XBOX_DPAD_UP);
  bit_write(bit_check(controller.buttons, DOWN),
            gamepad_state.digital_buttons_1, XBOX_DPAD_DOWN);
  bit_write(bit_check(controller.buttons, LEFT),
            gamepad_state.digital_buttons_1, XBOX_DPAD_LEFT);
  bit_write(bit_check(controller.buttons, RIGHT),
            gamepad_state.digital_buttons_1, XBOX_DPAD_RIGHT);
  bit_write(bit_check(controller.buttons, START),
            gamepad_state.digital_buttons_1, XBOX_START);
  bit_write(bit_check(controller.buttons, SELECT),
            gamepad_state.digital_buttons_1, XBOX_BACK);
  bit_write(bit_check(controller.buttons, GREEN),
            gamepad_state.digital_buttons_2, XBOX_A);
  bit_write(bit_check(controller.buttons, RED),
            gamepad_state.digital_buttons_2, XBOX_B);
  bit_write(bit_check(controller.buttons, YELLOW),
            gamepad_state.digital_buttons_2, XBOX_Y);
  bit_write(bit_check(controller.buttons, BLUE),
            gamepad_state.digital_buttons_2, XBOX_X);
  bit_write(bit_check(controller.buttons, ORANGE),
            gamepad_state.digital_buttons_2, XBOX_LB);
  bit_write(bit_check(controller.buttons, RB), gamepad_state.digital_buttons_2,
            XBOX_RB);
  bit_write(bit_check(controller.buttons, HOME),
            gamepad_state.digital_buttons_2, XBOX_HOME);

  gamepad_state.l_x = controller.l_x;
  gamepad_state.l_y = controller.l_y;
  gamepad_state.r_x = controller.r_x;
  gamepad_state.r_y = controller.r_y;
  gamepad_state.lt = controller.lt;
  gamepad_state.rt = controller.rt;
  /* Device must be connected and configured for the task to run */
  if (USB_DeviceState != DEVICE_STATE_Configured)
    return;

  /* Select the Joystick Report Endpoint */
  Endpoint_SelectEndpoint(JOYSTICK_EPADDR_IN);

  /* Check to see if the host is ready for another packet */
  if (Endpoint_IsINReady()) {
    /* Write Joystick Report Data */
    Endpoint_Write_Stream_LE(&gamepad_state, 20, NULL);

    /* Finalize the stream transfer to send the last packet */
    Endpoint_ClearIN();
  }
  USB_USBTask();
  wdt_reset();
}