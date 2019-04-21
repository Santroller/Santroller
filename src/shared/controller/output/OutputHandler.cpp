#include "OutputHandler.h"
Output OutputHandler::output;
void OutputHandler::process(Controller *controller) {
  if (bit_check(controller->buttons, START) &&
      bit_check(controller->buttons, SELECT)) {
    bootloader();
  }
  output.update(*controller);
}

void OutputHandler::init() {
  output.init();
}

bool OutputHandler::ready() {
  return output.ready();
}
extern "C" {
void EVENT_USB_Device_Connect(void) { OutputHandler::output.usb_connect(); }
void EVENT_USB_Device_Disconnect(void) { OutputHandler::output.usb_disconnect(); }
void EVENT_USB_Device_ConfigurationChanged(void) {
  OutputHandler::output.usb_configuration_changed();
}
void EVENT_USB_Device_ControlRequest(void) { OutputHandler::output.usb_control_request(); }
void EVENT_USB_Device_StartOfFrame(void) { OutputHandler::output.usb_start_of_frame(); }
}