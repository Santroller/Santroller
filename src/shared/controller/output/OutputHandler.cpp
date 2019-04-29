#include "OutputHandler.h"
#include <avr/eeprom.h>
Output *Output::output;
void OutputHandler::process(Controller *controller) {
  if (bit_check(controller->buttons, START) &&
      bit_check(controller->buttons, SELECT)) {
    serial();
  }
  Output::output->update(*controller);
}

void OutputHandler::init() {
  if (check_serial()) {
    Output::output = new SerialOutput();
  } else {
#if OUTPUT_TYPE == XINPUT
    Output::output = new XInputOutput();
#elif OUTPUT_TYPE == GAMEPAD
    Output::output = new GamepadOutput();
#elif OUTPUT_TYPE == KEYBOARD
    Output::output = new KeyboardOutput();
#endif
  }
  Output::output->init();
}

uint16_t get_descriptor(const uint8_t DescriptorType,
                        const uint8_t DescriptorNumber,
                        const void **const DescriptorAddress,
                        uint8_t *const DescriptorMemorySpace) {
  return Output::output->get_descriptor(DescriptorType, DescriptorNumber,
                                        DescriptorAddress,
                                        DescriptorMemorySpace);
}

void EVENT_USB_Device_Connect(void) { Output::output->usb_connect(); }
void EVENT_USB_Device_Disconnect(void) { Output::output->usb_disconnect(); }
void EVENT_USB_Device_ConfigurationChanged(void) {
  Output::output->usb_configuration_changed();
}
void EVENT_USB_Device_ControlRequest(void) {
  switch (USB_ControlRequest.bRequest) {
  case 0x30:
    bootloader();
    break;
  case 0x31:
    serial();
    break;
  }
  Output::output->usb_control_request();
}
void EVENT_USB_Device_StartOfFrame(void) {
  Output::output->usb_start_of_frame();
}