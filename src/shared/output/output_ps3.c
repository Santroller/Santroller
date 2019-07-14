#include "output_ps3.h"
#include "output_handler.h"
#include "usb/Descriptors.h"
#include <stdlib.h>
// Data that we need to send in order for the PS3 to identify our controller
// correctly.
const static uint8_t PROGMEM ps3_init_data[] = {0x21, 0x26, 0x01, 0x07,
                                                0x00, 0x00, 0x00, 0x00};
// Bindings to go from controller to ps3
static const uint8_t PROGMEM ps3ButtonBindings[] = {
    15, 13, 12, 14, 0xff, 0xff, 8, 9, 4, 5, 6, 7, 10, 11};
static uint8_t prev_ps3_report[sizeof(USB_PS3Report_Data_t)];
static const USB_Descriptor_HIDReport_Datatype_t PROGMEM ps3_report_descriptor[] = {
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,       // USAGE (Gamepad)
    0xa1, 0x01,       // COLLECTION (Application)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x35, 0x00,       //   PHYSICAL_MINIMUM (0)
    0x45, 0x01,       //   PHYSICAL_MAXIMUM (1)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x95, 0x0e,       //   REPORT_COUNT (16)
    0x05, 0x09,       //   USAGE_PAGE (Button)
    0x19, 0x01,       //   USAGE_MINIMUM (Button 1)
    0x29, 0x0e,       //   USAGE_MAXIMUM (Button 16)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x95, 0x02,       //   REPORT_COUNT (3)
    0x81, 0x01,       //   INPUT (Cnst,Ary,Abs)
    0x05, 0x01,       //   USAGE_PAGE (Generic Desktop)
    0x25, 0x07,       //   LOGICAL_MAXIMUM (7)
    0x46, 0x3b, 0x01, //   PHYSICAL_MAXIMUM (315)
    0x75, 0x04,       //   REPORT_SIZE (4)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x65, 0x14,       //   UNIT (Eng Rot:Angular Pos)
    0x09, 0x39,       //   USAGE (Hat switch)
    0x81, 0x42,       //   INPUT (Data,Var,Abs,Null)
    0x65, 0x00,       //   UNIT (None)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x81, 0x01,       //   INPUT (Cnst,Ary,Abs)
    0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
    0x46, 0xff, 0x00, //   PHYSICAL_MAXIMUM (255)
    0x09, 0x30,       //   USAGE (X)
    0x09, 0x31,       //   USAGE (Y)
    0x09, 0x32,       //   USAGE (Z)
    0x09, 0x35,       //   USAGE (Rz)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x04,       //   REPORT_COUNT (6)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x06, 0x00, 0xff, //   USAGE_PAGE (Vendor Specific)
    0x09, 0x20,       //   Unknown
    0x09, 0x21,       //   Unknown
    0x09, 0x22,       //   Unknown
    0x09, 0x23,       //   Unknown
    0x09, 0x24,       //   Unknown
    0x09, 0x25,       //   Unknown
    0x09, 0x26,       //   Unknown
    0x09, 0x27,       //   Unknown
    0x09, 0x28,       //   Unknown
    0x09, 0x29,       //   Unknown
    0x09, 0x2a,       //   Unknown
    0x09, 0x2b,       //   Unknown
    0x95, 0x0c,       //   REPORT_COUNT (12)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x0a, 0x21, 0x26, //   Unknown
    0x95, 0x08,       //   REPORT_COUNT (8)
    0xb1, 0x02,       //   FEATURE (Data,Var,Abs)
    0xc0              // END_COLLECTION
};
bool ps3_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                       uint8_t *const ReportID, const uint8_t ReportType,
                       void *ReportData, uint16_t *const ReportSize) {

  USB_PS3Report_Data_t *JoystickReport = (USB_PS3Report_Data_t *)ReportData;
  for (uint8_t i = 0; i < sizeof(ps3ButtonBindings); i++) {
    if (ps3ButtonBindings[i] == 0xff) continue;
    bool bitSet = bit_check(last_controller.buttons, ps3ButtonBindings[i]);
    bit_write(bitSet, JoystickReport->buttons, i);
    // if (i < 8) {
    //   JoystickReport->button_analogue[i] = bitSet ? 0xFF : 0x00;
    // }
  }
  switch (last_controller.buttons & 0xF) {
  case 0x01: // Top
    JoystickReport->hat = 0x00;
    break;
  case 0x09: // Top-Right
    JoystickReport->hat = 0x01;
    break;
  case 0x08: // Right
    JoystickReport->hat = 0x02;
    break;
  case 0x0a: // Bottom-Right
    JoystickReport->hat = 0x03;
    break;
  case 0x02: // Bottom
    JoystickReport->hat = 0x04;
    break;
  case 0x06: // Bottom-Left
    JoystickReport->hat = 0x05;
    break;
  case 0x04: // Left
    JoystickReport->hat = 0x06;
    break;
  case 0x05: // Top-Left
    JoystickReport->hat = 0x07;
    break;
  default:
    JoystickReport->hat = 0x08;
  }
  JoystickReport->l_x = (last_controller.l_x >> 2) + 128;
  JoystickReport->l_y = (last_controller.l_y >> 2) + 128;
  JoystickReport->r_x = (last_controller.r_x >> 2) + 128;
  JoystickReport->r_y = (last_controller.r_y >> 2) + 128;
  // bit_write(last_controller.lt > 50, JoystickReport->buttons, 4);
  // bit_write(last_controller.rt > 50, JoystickReport->buttons, 5);
  // JoystickReport->l2_axis = last_controller.lt;
  // JoystickReport->r2_axis = last_controller.rt;

  *ReportSize = sizeof(USB_PS3Report_Data_t);

  return false;
}
void ps3_control_request(void) {
  if (config.sub_type != SWITCH_GAMEPAD_SUBTYPE && USB_ControlRequest.wIndex == interface.Config.InterfaceNumber) {
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
      if (USB_ControlRequest.bRequest == HID_REQ_GetReport) {
        Endpoint_ClearSETUP();
        while (!(Endpoint_IsINReady()))
          ;
        for (uint8_t i = 0; i < sizeof(ps3_init_data); i++) {
          Endpoint_Write_8(ps3_init_data[i]);
        }
        Endpoint_ClearIN();
        Endpoint_ClearStatusStage();
        return;
      }
    }
  }
  HID_Device_ProcessControlRequest(&interface);
}

void ps3_init(event_pointers *events, USB_ClassInfo_HID_Device_t *hid_device) {
  events->create_hid_report = ps3_create_report;
  events->control_request = ps3_control_request;
  hid_report_address = ps3_report_descriptor;
  hid_report_size = sizeof(ps3_report_descriptor);
  hid_device->Config.PrevReportINBuffer = &prev_ps3_report;
  hid_device->Config.PrevReportINBufferSize = sizeof(prev_ps3_report);
  if (config.sub_type == SWITCH_GAMEPAD_SUBTYPE) {
    // TODO: check if this is actually a requirement
    DeviceDescriptor.VendorID = 0x0F0D;
    DeviceDescriptor.ProductID = 0x0092;
  }
  if (config.sub_type > PS3_GAMEPAD_SUBTYPE) {
    DeviceDescriptor.VendorID = 0x12ba;
  }
  if (config.sub_type == PS3_GUITAR_GH_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0100;
  }
  if (config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0200;
  }
  if (config.sub_type == PS3_DRUM_GH_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0120;
  }
  if (config.sub_type == PS3_DRUM_RB_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0210;
  }
}