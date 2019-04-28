#include "OutputHandler.h"
#include <avr/eeprom.h>
Output *OutputHandler::output;
void OutputHandler::process(Controller *controller) {
  if (bit_check(controller->buttons, START) &&
      bit_check(controller->buttons, SELECT)) {
    bootloader();
  }
  output->update(*controller);
}

void OutputHandler::init() {
#if OUTPUT_TYPE == XINPUT
  output = new XInputOutput();
#elif OUTPUT_TYPE == GAMEPAD
  output = new GamepadOutput();
#elif OUTPUT_TYPE == KEYBOARD
  output = new KeyboardOutput();
#endif
  output->init();
}

uint16_t get_descriptor(const uint8_t DescriptorType,
                        const uint8_t DescriptorNumber,
                        const void **const DescriptorAddress) {
  return OutputHandler::output->get_descriptor(DescriptorType, DescriptorNumber,
                                               DescriptorAddress);
}

void EVENT_USB_Device_Connect(void) { OutputHandler::output->usb_connect(); }
void EVENT_USB_Device_Disconnect(void) {
  OutputHandler::output->usb_disconnect();
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  OutputHandler::output->usb_configuration_changed();
}
void EVENT_USB_Device_ControlRequest(void) {
  OutputHandler::output->usb_control_request();
}
void EVENT_USB_Device_StartOfFrame(void) {
  OutputHandler::output->usb_start_of_frame();
}