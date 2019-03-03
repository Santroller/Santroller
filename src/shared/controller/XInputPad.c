#include "XInputPad.h"
USB_JoystickReport_Data_t gamepad_state;

void (*padUSBConnectEventCallback)(void) = NULL;

void (*padUSBDisconnectEventCallback)(void) = NULL;

/** Event handler for the USB_Connect event. This indicates that the device is
 * enumerating via the status LEDs and starts the library USB task to begin the
 * enumeration and USB management process.
 */
void EVENT_USB_Device_Connect(void) {
  if (padUSBConnectEventCallback)
    padUSBConnectEventCallback();
}

/** Event handler for the USB_Disconnect event. This indicates that the device
 * is no longer connected to a host via the status LEDs and stops the USB
 * management and joystick reporting tasks.
 */
void EVENT_USB_Device_Disconnect(void) {
  if (padUSBDisconnectEventCallback)
    padUSBDisconnectEventCallback();
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the
 * host set the current configuration of the USB device after enumeration - the
 * device endpoints are configured and the joystick reporting task started.
 */
void EVENT_USB_Device_ConfigurationChanged(void) {
  /* Setup HID Report Endpoint */
  Endpoint_ConfigureEndpoint(JOYSTICK_EPADDR_IN, EP_TYPE_INTERRUPT, 20, 1);
  Endpoint_ConfigureEndpoint((ENDPOINT_DIR_IN | 3), EP_TYPE_INTERRUPT, 32, 1);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and
 * process control requests sent to the device from the USB host before passing
 * along unhandled control requests to the library for processing internally.
 */
void EVENT_USB_Device_ControlRequest(void) {
  const void* DescriptorPointer;
	uint16_t    DescriptorSize;
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

/** Function to manage HID report generation and transmission to the host. */
void HID_Task(void) {
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
}

void xbox_reset_pad_status(void) {
  memset(&gamepad_state, 0x00, sizeof(USB_JoystickReport_Data_t));

  gamepad_state.rsize = 20;
}

void xbox_send_pad_state(void) {
  HID_Task();
  USB_USBTask();
}

void xbox_reset_watchdog(void) { wdt_reset(); }

void xbox_init(bool watchdog) {
  if (watchdog) {
    wdt_enable(WDTO_2S);
  } else {
    wdt_disable();
  }

  xbox_reset_pad_status();

  USB_Init();

  sei();
}

void xbox_set_connect_callback(void (*callbackPtr)(void)) {
  padUSBConnectEventCallback = callbackPtr;
}

void xbox_set_disconnect_callback(void (*callbackPtr)(void)) {
  padUSBDisconnectEventCallback = callbackPtr;
}