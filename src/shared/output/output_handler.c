#include "output_handler.h"
#include "bootloader/bootloader.h"
#include "output_xinput.h"
#include "output_hid.h"
event_pointers events;
void output_init(void) {
  if (config.sub_type >= KEYBOARD_SUBTYPE) {
    hid_init(&events);
  } else {
    xinput_init(&events);
  }
  USB_Init();
  sei();
}

void output_tick(controller_t controller) {
  events.tick(controller);
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  events.configuration_changed();
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
  events.control_request();
}
void EVENT_USB_Device_StartOfFrame(void) { events.start_of_frame(); }

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {
  // events.process_hid_report(HIDInterfaceInfo, ReportID, ReportType, ReportData,
  //                           ReportSize);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  return events.create_hid_report(HIDInterfaceInfo, ReportID, ReportType, ReportData,
                           ReportSize);
}