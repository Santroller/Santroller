#include "GamepadOutput.h"

static uint8_t PrevHIDReport[sizeof(USB_GamepadReport_Data_t)];

// Bindings to go from controller to ps3
static uint8_t buttonBindings[] = {15, 13, 12, 14, 0xff, 0xff, 8,
                                   9,  4,  5,  6,  7,    10,   11};
// Based on https://github.com/progmem/Switch-Fightstick
const USB_Descriptor_HIDReport_Datatype_t PROGMEM HIDReport_Datatype[] = {
    HID_RI_USAGE_PAGE(8, 1), /* Generic Desktop */
    HID_RI_USAGE(8, 5),      /* Joystick */
    HID_RI_COLLECTION(8, 1), /* Application */
    // Buttons (2 bytes)
    HID_RI_LOGICAL_MINIMUM(8, 0),
    HID_RI_LOGICAL_MAXIMUM(8, 1),
    HID_RI_PHYSICAL_MINIMUM(8, 0),
    HID_RI_PHYSICAL_MAXIMUM(8, 1),
    // The Switch will allow us to expand the original HORI descriptors to a
    // full 16 buttons. The Switch will make use of 14 of those buttons.
    HID_RI_REPORT_SIZE(8, 1),
    HID_RI_REPORT_COUNT(8, 16),
    HID_RI_USAGE_PAGE(8, 9),
    HID_RI_USAGE_MINIMUM(8, 1),
    HID_RI_USAGE_MAXIMUM(8, 16),
    HID_RI_INPUT(8, 2),
    // HAT Switch (1 nibble)
    HID_RI_USAGE_PAGE(8, 1),
    HID_RI_LOGICAL_MAXIMUM(8, 7),
    HID_RI_PHYSICAL_MAXIMUM(16, 315),
    HID_RI_REPORT_SIZE(8, 4),
    HID_RI_REPORT_COUNT(8, 1),
    HID_RI_UNIT(8, 20),
    HID_RI_USAGE(8, 57),
    HID_RI_INPUT(8, 66),
    // There's an additional nibble here that's utilized as part of the Switch
    // Pro Controller. I believe this -might- be separate U/D/L/R bits on the
    // Switch Pro Controller, as they're utilized as four button descriptors on
    // the Switch Pro Controller.
    HID_RI_UNIT(8, 0),
    HID_RI_REPORT_COUNT(8, 1),
    HID_RI_INPUT(8, 1),
    // Joystick (4 bytes)
    HID_RI_LOGICAL_MAXIMUM(16, 255),
    HID_RI_PHYSICAL_MAXIMUM(16, 255),
    HID_RI_USAGE(8, 48),
    HID_RI_USAGE(8, 49),
    HID_RI_USAGE(8, 50),
    HID_RI_USAGE(8, 53),
    HID_RI_REPORT_SIZE(8, 8),
    HID_RI_REPORT_COUNT(8, 4),
    HID_RI_INPUT(8, 2),
    // ??? Vendor Specific (1 byte)
    // This byte requires additional investigation.
    HID_RI_USAGE_PAGE(16, 65280),
    HID_RI_USAGE(8, 32),
    HID_RI_REPORT_COUNT(8, 1),
    HID_RI_INPUT(8, 2),
    // Output (8 bytes)
    // Original observation of this suggests it to be a mirror of the inputs
    // that we sent. The Switch requires us to have these descriptors available.
    HID_RI_USAGE(16, 9761),
    HID_RI_REPORT_COUNT(8, 8),
    HID_RI_OUTPUT(8, 2),
    HID_RI_END_COLLECTION(0),
};

const size_t GamepadOutput::ReportDatatypeSize() {
  return sizeof(HIDReport_Datatype);
}
USB_ClassInfo_HID_Device_t GamepadInterface = {
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
  return &GamepadInterface;
}

Controller last_controller;

void GamepadOutput::update(Controller controller) {
  USB_USBTask();
  wdt_reset();
  last_controller = controller;
  HID_Device_USBTask(HID_Interface);
}

bool GamepadOutput::hid_create_report(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {

  auto JoystickReport = (USB_GamepadReport_Data_t *)ReportData;
  for (uint8_t i = 0; i < sizeof(buttonBindings); i++) {
    if (buttonBindings[i] == 0xff)
      continue;
    auto bitSet = bit_check(last_controller.buttons, buttonBindings[i]);
    bit_write(bitSet, JoystickReport->buttons, i);
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
  JoystickReport->l_x = (last_controller.l_x / 256) + 128;
  JoystickReport->l_y = (last_controller.l_y / 256) + 128;
  JoystickReport->r_x = (last_controller.r_x / 256) + 128;
  JoystickReport->r_y = (last_controller.r_y / 256) + 128;
  bit_write(last_controller.lt > 50, JoystickReport->buttons, 4);
  bit_write(last_controller.rt > 50, JoystickReport->buttons, 5);

  *ReportSize = sizeof(USB_GamepadReport_Data_t);

  return false;
}

void GamepadOutput::usb_control_request() {
  if (USB_ControlRequest.wIndex == HID_Interface->Config.InterfaceNumber) {
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
      if (USB_ControlRequest.bRequest == HID_REQ_GetReport) {
        Endpoint_ClearSETUP();
        while (!(Endpoint_IsINReady()))
          ;
        Endpoint_Write_8(0x21);
        Endpoint_Write_8(0x26);
        Endpoint_Write_8(0x01);
        // 06 = gh3, 07 = normal, 00 = rb
        if (config.sub_type == PS3_CONTROLLER_SUBTYPE) {
          Endpoint_Write_8(0x07);
        }
        if (config.sub_type == PS3_GUITAR_SUBTYPE) {
          Endpoint_Write_8(0x06);
        }
        if (config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
          Endpoint_Write_8(0x00);
        }
        Endpoint_Write_8(0x00);
        Endpoint_Write_8(0x00);
        Endpoint_Write_8(0x00);
        Endpoint_Write_8(0x00);
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