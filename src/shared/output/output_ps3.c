#include "output_ps3.h"
#include "output_hid.h"
#include "usb/Descriptors.h"
// Data that we need to send in order for the PS3 to identify our controller
// correctly.
const static uint8_t PROGMEM ps3_init_data[] = {0x21, 0x26, 0x01, 0x07,
                                                0x00, 0x00, 0x00, 0x00};
// Bindings to go from controller to ps3
static const uint8_t PROGMEM ps3ButtonBindings[] = {
    15, 13, 12, 14, 0xff, 0xff, 8, 9, 4, 5, 6, 7, 10, 11};
static uint8_t prev_ps3_report[sizeof(USB_PS3Report_Data_t)];
const USB_Descriptor_HIDReport_Datatype_t PROGMEM ps3_report_descriptor[] = {
    HID_RI_USAGE_PAGE(8, 1), /* Generic Desktop */
    HID_RI_USAGE(8, 5),      /* Joystick */
    HID_RI_COLLECTION(8, 1), /* Application */
    // Buttons (2 bytes)
    HID_RI_LOGICAL_MINIMUM(8, 0), HID_RI_LOGICAL_MAXIMUM(8, 1),
    HID_RI_PHYSICAL_MINIMUM(8, 0), HID_RI_PHYSICAL_MAXIMUM(8, 1),
    // The Switch will allow us to expand the original HORI descriptors to a
    // full 16 buttons. The Switch will make use of 14 of those buttons.
    HID_RI_REPORT_SIZE(8, 1), HID_RI_REPORT_COUNT(8, 16),
    HID_RI_USAGE_PAGE(8, 9), HID_RI_USAGE_MINIMUM(8, 1),
    HID_RI_USAGE_MAXIMUM(8, 16), HID_RI_INPUT(8, 2),
    // HAT Switch (1 nibble)
    HID_RI_USAGE_PAGE(8, 1), HID_RI_LOGICAL_MAXIMUM(8, 7),
    HID_RI_PHYSICAL_MAXIMUM(16, 315), HID_RI_REPORT_SIZE(8, 4),
    HID_RI_REPORT_COUNT(8, 1), HID_RI_UNIT(8, 20), HID_RI_USAGE(8, 57),
    HID_RI_INPUT(8, 66),
    // There's an additional nibble here that's utilized as part of the Switch
    // Pro Controller. I believe this -might- be separate U/D/L/R bits on the
    // Switch Pro Controller, as they're utilized as four button descriptors on
    // the Switch Pro Controller.
    HID_RI_UNIT(8, 0), HID_RI_REPORT_COUNT(8, 1), HID_RI_INPUT(8, 1),
    // Joystick (4 bytes)
    HID_RI_LOGICAL_MAXIMUM(16, 255), HID_RI_PHYSICAL_MAXIMUM(16, 255),
    HID_RI_USAGE(8, 48), HID_RI_USAGE(8, 49), HID_RI_USAGE(8, 50),
    HID_RI_USAGE(8, 53), HID_RI_REPORT_SIZE(8, 8), HID_RI_REPORT_COUNT(8, 4),
    HID_RI_INPUT(8, 2),
    // Vendor Specific (PS3)
    HID_RI_USAGE_PAGE(16, 255), HID_RI_USAGE(8, 0x20), HID_RI_USAGE(8, 0x21),
    HID_RI_USAGE(8, 0x22), HID_RI_USAGE(8, 0x23), HID_RI_USAGE(8, 0x24),
    HID_RI_USAGE(8, 0x25), HID_RI_USAGE(8, 0x26), HID_RI_USAGE(8, 0x27),
    HID_RI_USAGE(8, 0x28), HID_RI_USAGE(8, 0x29), HID_RI_USAGE(8, 0x2a),
    HID_RI_USAGE(8, 0x2b), HID_RI_REPORT_COUNT(8, 12),
    // End of vendor specific
    // Vendor Specific (Switch) (1 byte)
    HID_RI_USAGE_PAGE(16, 65280), HID_RI_USAGE(8, 32),
    HID_RI_REPORT_COUNT(8, 1),
    // End of Switch Vendor Specific
    HID_RI_INPUT(8, 2),
    // Output (8 bytes)
    // Original observation of this suggests it to be a mirror of the inputs
    // that we sent. The Switch requires us to have these descriptors available.
    HID_RI_USAGE(16, 9761), HID_RI_REPORT_COUNT(8, 8), HID_RI_OUTPUT(8, 2),
    HID_RI_END_COLLECTION(0)};

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
  if (USB_ControlRequest.wIndex == interface.Config.InterfaceNumber) {
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
    //TODO: check if this is actually a requirement
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