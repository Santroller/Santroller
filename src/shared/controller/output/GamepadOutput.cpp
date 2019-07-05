#include "GamepadOutput.h"

static uint8_t PrevHIDReport[sizeof(USB_GamepadReport_Data_t)];

// Bindings to go from controller to ps3
static uint8_t buttonBindings[] = {15, 13, 12, 14, 0xff, 0xff, 8,
                                   9,  4,  5,  6,  7,    10,   11};
// Based on https://github.com/progmem/Switch-Fightstick
const USB_Descriptor_HIDReport_Datatype_t PROGMEM HIDReport_Datatype[] = {
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,       // USAGE (Gamepad)
    0xa1, 0x01,       // COLLECTION (Application)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x35, 0x00,       //   PHYSICAL_MINIMUM (0)
    0x45, 0x01,       //   PHYSICAL_MAXIMUM (1)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x95, 0x0e,       //   REPORT_COUNT (13)
    0x05, 0x09,       //   USAGE_PAGE (Button)
    0x19, 0x01,       //   USAGE_MINIMUM (Button 1)
    0x29, 0x0e,       //   USAGE_MAXIMUM (Button 13)
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

const size_t GamepadOutput::ReportDatatypeSize() {
  return sizeof(HIDReport_Datatype);
}
static USB_ClassInfo_HID_Device_t GamepadInterfacePs = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : HID_EPADDR,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : PrevHIDReport,
    PrevReportINBufferSize : sizeof(PrevHIDReport),
  },
};
USB_ClassInfo_HID_Device_t *GamepadOutput::createHIDInterface() {
  return &GamepadInterfacePs;
}

Controller last_controller_ps;

void GamepadOutput::update(Controller controller) {
  USB_USBTask();
  wdt_reset();
  last_controller_ps = controller;
  HID_Device_USBTask(HID_Interface);
}

bool GamepadOutput::hid_create_report(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {

  auto JoystickReport = (USB_GamepadReport_Data_t *)ReportData;
  for (uint8_t i = 0; i < sizeof(buttonBindings); i++) {
    if (buttonBindings[i] == 0xff)
      continue;
    auto bitSet = bit_check(last_controller_ps.buttons, buttonBindings[i]);
    bit_write(bitSet, JoystickReport->buttons, i);
    // if (i < 8) {
    //   JoystickReport->button_analogue[i] = bitSet ? 0xFF : 0x00;
    // }
  }
  switch (last_controller_ps.buttons & 0xF) {
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
  JoystickReport->l_x = (last_controller_ps.l_x / 256) + 128;
  JoystickReport->l_y = (last_controller_ps.l_y / 256) + 128;
  JoystickReport->r_x = (last_controller_ps.r_x / 256) + 128;
  JoystickReport->r_y = (last_controller_ps.r_y / 256) + 128;
  // bit_write(last_controller_ps.lt > 50, JoystickReport->buttons, 4);
  // bit_write(last_controller_ps.rt > 50, JoystickReport->buttons, 5);
  // JoystickReport->l2_axis = last_controller_ps.lt;
  // JoystickReport->r2_axis = last_controller_ps.rt;

  *ReportSize = sizeof(USB_GamepadReport_Data_t);

  return false;
}
uint8_t data[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
void GamepadOutput::usb_control_request() {
  if (USB_ControlRequest.wIndex == HID_Interface->Config.InterfaceNumber) {
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
      if (USB_ControlRequest.bRequest == HID_REQ_GetReport) {
        Endpoint_ClearSETUP();
        while (!(Endpoint_IsINReady()));
          for (uint8_t i = 0;i < sizeof(data); i++) {
            Endpoint_Write_8(data[i]);
          }
        // Endpoint_Write_8(0x21);
        // Endpoint_Write_8(0x26);
        // Endpoint_Write_8(0x01);
        // 06 = gh3, 07 = normal, 00 = rb
        // if (config.sub_type == PS3_CONTROLLER_SUBTYPE) {
        // Endpoint_Write_8(0x07);
        // }
        // if (config.sub_type == PS3_GUITAR_SUBTYPE) {
        // Endpoint_Write_8(0x06);
        // }
        // if (config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
        //   Endpoint_Write_8(0x00);
        // }
        // Endpoint_Write_8(0x00);
        // Endpoint_Write_8(0x00);
        // Endpoint_Write_8(0x00);
        // Endpoint_Write_8(0x00);
        Endpoint_ClearIN();
        Endpoint_ClearStatusStage();
        return;
      }
    }
  }
  HID_Device_ProcessControlRequest(HID_Interface);
}

uint16_t
GamepadOutput::get_hid_descriptor(const uint8_t DescriptorType,
                                  const uint8_t DescriptorNumber,
                                  const void **const DescriptorAddress,
                                  uint8_t *const DescriptorMemorySpace) {
  *DescriptorAddress = HIDReport_Datatype;
  *DescriptorMemorySpace = MEMSPACE_FLASH;
  return ReportDatatypeSize();
}