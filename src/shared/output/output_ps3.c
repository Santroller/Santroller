#include "output_ps3.h"
#include "output_handler.h"
#include "usb/Descriptors.h"
#include <stdlib.h>
// Data that we need to send in order for the PS3 to identify our controller
// correctly.
const static uint8_t PROGMEM ps3_init_data[] = {0x21, 0x26, 0x01, 0x07,
                                                0x00, 0x00, 0x00, 0x00};
static const uint8_t PROGMEM switchButtonBindings[] = {
    15, 13, 12, 14, 0xff, 0xff, 8, 9, 4, 5, 6, 7, 10, 11};
// Bindings to go from controller to ps3
static const uint8_t PROGMEM ps3ButtonBindings[] = {
    15, 12, 13, 14, 0xff, 0xff, 8, 9, 5, 4, 6, 7, 10, 11};
static const uint8_t PROGMEM hat_bindings[] = {0x08, 0x00, 0x4, 0x8, 0x6, 0x7,
                                       0x5,  0x8,  0x2, 0x1, 0x3};
static const uint8_t *currentBindings;
static uint8_t currentBindingLen;
static const USB_Descriptor_HIDReport_Datatype_t PROGMEM
    ps3_report_descriptor[] = {
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
  uint8_t button;
  for (uint8_t i = 0; i < currentBindingLen; i++) {
    button = pgm_read_byte(currentBindings + i);
    if (button == 0xff) continue;
    bool bitSet = bit_check(controller.buttons, button);
    bit_write(bitSet, JoystickReport->buttons, i);
    // if (i < 8) { JoystickReport->button_analogue[i] = bitSet ? 0xFF : 0x00; }
  }
  button = controller.buttons & 0xF;
  if (button > 0x0a) {
    JoystickReport->hat = 0x08;
  } else {
    JoystickReport->hat = pgm_read_byte(hat_bindings+button);
  }
  JoystickReport->l_x = (controller.l_x >> 8) + 128;
  JoystickReport->l_y = (controller.l_y >> 8) + 128;
  JoystickReport->r_x = (controller.r_x >> 8) + 128;
  JoystickReport->r_y = (controller.r_y >> 8) + 128;
  if (config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
    JoystickReport->r_x = -JoystickReport->r_x;
  }
  if (config.sub_type > PS3_GAMEPAD_SUBTYPE) {
    // XINPUT guitars use LB for orange, PS3 uses L
    bit_write(bit_check(controller.buttons, XBOX_LB), JoystickReport->buttons,
              4);
    // PS3 guitars use ZL for a tilt bit i think
    bit_write(JoystickReport->r_y > 50, JoystickReport->buttons, 6);
    JoystickReport->r2_axis = JoystickReport->r_y;
  } else {
    bit_write(controller.lt > 50, JoystickReport->buttons, 4);
    bit_write(controller.rt > 50, JoystickReport->buttons, 5);
    JoystickReport->l2_axis = controller.lt;
    JoystickReport->r2_axis = controller.rt;
  }
  *ReportSize = sizeof(USB_PS3Report_Data_t);

  return false;
}
void ps3_control_request(void) {
  if (config.sub_type != SWITCH_GAMEPAD_SUBTYPE &&
      USB_ControlRequest.wIndex == interface.Config.InterfaceNumber) {
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
  if (config.sub_type == SWITCH_GAMEPAD_SUBTYPE) {
    DeviceDescriptor.VendorID = 0x0F0D;
    DeviceDescriptor.ProductID = 0x0092;
    currentBindings = switchButtonBindings;
    currentBindingLen = sizeof(switchButtonBindings);
  }
  if (config.sub_type == PS3_GAMEPAD_SUBTYPE) {
    currentBindings = ps3ButtonBindings;
    currentBindingLen = sizeof(ps3ButtonBindings);
  }
  if (config.sub_type > PS3_GAMEPAD_SUBTYPE) {
    DeviceDescriptor.VendorID = 0x12ba;
    currentBindings = ps3ButtonBindings;
    currentBindingLen = sizeof(ps3ButtonBindings) - 2;
  }
  if (config.sub_type == PS3_GUITAR_GH_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0100;
  } else if (config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0200;
  } else if (config.sub_type == PS3_DRUM_GH_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0120;
  } else if (config.sub_type == PS3_DRUM_RB_SUBTYPE) {
    DeviceDescriptor.ProductID = 0x0210;
  }
}