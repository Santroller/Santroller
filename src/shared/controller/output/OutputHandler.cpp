#include "OutputHandler.h"
#include <avr/eeprom.h>
Output *Output::output;
void OutputHandler::process(Controller *controller) {
  if (bit_check(controller->buttons, XBOX_START) &&
      bit_check(controller->buttons, XBOX_BACK)) {
    bootloader();
  }
  Output::output->update(*controller);
}

void OutputHandler::init() {
  if(config.sub_type == PS3_SUBTYPE) {
    Output::output = new GamepadOutput();
  } else if(config.sub_type == KEYBOARD_SUBTYPE) {
    Output::output = new KeyboardOutput();
  } else {
    Output::output = new XInputOutput();
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
    return;
  case 0x31:
    Endpoint_ClearSETUP();
    Endpoint_Read_Control_EStream_LE(&config_pointer, sizeof(config_t));
    eeprom_read_block(&config, &config_pointer, sizeof(config_t));
    Endpoint_ClearIN();
    reboot();
    return;
  case 0x32:
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_EStream_LE(&config_pointer, sizeof(config_t));
    Endpoint_ClearOUT();
    return;
  }
  Output::output->usb_control_request();
}
void EVENT_USB_Device_StartOfFrame(void) {
  Output::output->usb_start_of_frame();
}