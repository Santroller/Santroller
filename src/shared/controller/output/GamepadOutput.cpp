#include "GamepadOutput.h"

static uint8_t PrevHIDReport[sizeof(USB_GamepadReport_Data_t)];

// Bindings to go from controller to ps3
static uint8_t buttonBindings[] = {5, 6, 7,  4,  0,  3,  1, 2,
                                   9, 8, 15, 13, 12, 14, 10};

const USB_Descriptor_HIDReport_Datatype_t PROGMEM HIDReport_Datatype[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04,       // Usage (Joystick)
    0xA1, 0x01,       // Collection (Physical)
    0xA1, 0x02,       //   Collection (Application)
    0x85, 0x01,       //     Report ID (1)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x01,       //     Report Count (1)
    0x15, 0x00,       //     Logical Minimum (0)
    0x26, 0xFF, 0x00, //     Logical Maximum (255)
    0x81, 0x03, //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position) NOTE: reserved byte
    0x75, 0x01, //     Report Size (1)
    0x95, 0x13, //     Report Count (19)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x35, 0x00, //     Physical Minimum (0)
    0x45, 0x01, //     Physical Maximum (1)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x13, //     Usage Maximum (0x13)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position)
    0x75, 0x01, //     Report Size (1)
    0x95, 0x0D, //     Report Count (13)
    0x06, 0x00, 0xFF, //     Usage Page (Vendor Defined 0xFF00)
    0x81, 0x03, //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position) NOTE: 32 bit integer, where 0:18 are
                //     buttons and 19:31 are reserved
    0x15, 0x00, //     Logical Minimum (0)
    0x26, 0xFF, 0x00, //     Logical Maximum (255)
    0x05, 0x01,       //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x01,       //     Usage (Pointer)
    0xA1, 0x00,       //     Collection (Undefined)
    0x75, 0x08,       //       Report Size (8)
    0x95, 0x04,       //       Report Count (4)
    0x35, 0x00,       //       Physical Minimum (0)
    0x46, 0xFF, 0x00, //       Physical Maximum (255)
    0x09, 0x30,       //       Usage (X)
    0x09, 0x31,       //       Usage (Y)
    0x09, 0x32,       //       Usage (Z)
    0x09, 0x35,       //       Usage (Rz)
    0x81, 0x02, //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //       Null Position) NOTE: four joysticks
    0xC0,       //     End Collection
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x27, //     Report Count (39)
    0x09, 0x01, //     Usage (Pointer)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x30, //     Report Count (48)
    0x09, 0x01, //     Usage (Pointer)
    0x91, 0x02, //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position,Non-volatile)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x30, //     Report Count (48)
    0x09, 0x01, //     Usage (Pointer)
    0xB1, 0x02, //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position,Non-volatile)
    0xC0,       //   End Collection
    0xA1, 0x02, //   Collection (Application)
    0x85, 0x02, //     Report ID (2)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x30, //     Report Count (48)
    0x09, 0x01, //     Usage (Pointer)
    0xB1, 0x02, //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position,Non-volatile)
    0xC0,       //   End Collection
    0xA1, 0x02, //   Collection (Application)
    0x85, 0xEE, //     Report ID (238)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x30, //     Report Count (48)
    0x09, 0x01, //     Usage (Pointer)
    0xB1, 0x02, //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position,Non-volatile)
    0xC0,       //   End Collection
    0xA1, 0x02, //   Collection (Application)
    0x85, 0xEF, //     Report ID (239)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x30, //     Report Count (48)
    0x09, 0x01, //     Usage (Pointer)
    0xB1, 0x02, //     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position,Non-volatile)
    0xC0,       //   End Collection
    0xC0,       // End Collection
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
  auto reportAnalogue =
      (uint8_t *)(JoystickReport +
                  offsetof(USB_GamepadReport_Data_t, dpad_up_a));
  auto bitSet = false;
  for (uint8_t i = 0; i > sizeof(buttonBindings); i++) {
    bitSet = bit_check(last_controller.buttons, i);
    bit_write(bitSet, JoystickReport->buttons, buttonBindings[i]);
    if (i > 4 && i < 16) {
      reportAnalogue[i - 4] = bitSet ? 0xFF : 0x00;
    }
  }
  JoystickReport->lx = last_controller.l_x;
  JoystickReport->ly = last_controller.l_y;
  JoystickReport->rx = last_controller.r_x;
  JoystickReport->ry = last_controller.r_y;
  JoystickReport->l2_a = last_controller.lt;
  JoystickReport->r2_a = last_controller.rt;
  bit_write(last_controller.lt > 0, JoystickReport->buttons, 16);
  bit_write(last_controller.rt > 0, JoystickReport->buttons, 17);

  *ReportSize = sizeof(USB_GamepadReport_Data_t);

  return false;
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