#include "output_ps3.h"
#include "output_handler.h"
#include "usb/Descriptors.h"
#include <stdlib.h>
// Data that we need to send in order for the PS3 to identify our controller
// correctly.
const static uint8_t PROGMEM ps3_init_data[] = {0x21, 0x26, 0x01};
static const uint8_t PROGMEM switchButtonBindings[] = {
    15, 13, 12, 14, 0xff, 0xff, 8, 9, 4, 5, 6, 7, 10, 11};
// Bindings to go from controller to ps3
static const uint8_t PROGMEM ps3ButtonBindings[] = {
    15, 12, 13, 14, 0xff, 0xff, 8, 9, 5, 4, 6, 7, 10, 11};
static const uint8_t PROGMEM ps3AxisBindings[] = {
    XBOX_DPAD_UP, XBOX_DPAD_RIGHT, XBOX_DPAD_DOWN, XBOX_DPAD_LEFT, 0xFF,
    0xFF,         XBOX_LB,         XBOX_RB,        XBOX_Y,         XBOX_B,
    XBOX_A,       XBOX_X};
static const uint8_t PROGMEM ghAxisBindings[] = {
    XBOX_DPAD_RIGHT, XBOX_DPAD_LEFT, XBOX_DPAD_UP, XBOX_DPAD_DOWN,
    XBOX_X,          XBOX_A,         XBOX_B,       XBOX_Y,
    XBOX_LB,         0xff,           0xff,         0xff,
};
static const uint8_t PROGMEM hat_bindings[] = {
    0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
static const uint8_t *currentBindings;
static const uint8_t *currentAxisBindings;
static uint8_t currentBindingLen;
static const USB_Descriptor_HIDReport_Datatype_t PROGMEM
    ps3_report_descriptor[] = {
        0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
        0x09, 0x05, // Usage (Game Pad)
        0xA1, 0x01, // Collection (Application)
        0x15, 0x00, //   Logical Minimum (0)
        0x25, 0x01, //   Logical Maximum (1)
        0x35, 0x00, //   Physical Minimum (0)
        0x45, 0x01, //   Physical Maximum (1)
        0x75, 0x01, //   Report Size (1)
        0x95, 0x0D, //   Report Count (13)
        0x05, 0x09, //   Usage Page (Button)
        0x19, 0x01, //   Usage Minimum (0x01)
        0x29, 0x0D, //   Usage Maximum (0x0D)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0x95, 0x03, //   Report Count (3)
        0x81, 0x01, //   Input (Const,Array,Abs,No Wrap,Linear,Preferred
                    //   State,No Null Position)
        0x05, 0x01, //   Usage Page (Generic Desktop Ctrls)
        0x25, 0x07, //   Logical Maximum (7)
        0x46, 0x3B, 0x01, //   Physical Maximum (315)
        0x75, 0x04,       //   Report Size (4)
        0x95, 0x01,       //   Report Count (1)
        0x65, 0x14, //   Unit (System: English Rotation, Length: Centimeter)
        0x09, 0x39, //   Usage (Hat switch)
        0x81, 0x42, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null
                    //   State)
        0x65, 0x00, //   Unit (None)
        0x95, 0x01, //   Report Count (1)
        0x81, 0x01, //   Input (Const,Array,Abs,No Wrap,Linear,Preferred
                    //   State,No Null Position)
        0x26, 0xFF, 0x00, //   Logical Maximum (255)
        0x46, 0xFF, 0x00, //   Physical Maximum (255)
        0x09, 0x30,       //   Usage (X)
        0x09, 0x31,       //   Usage (Y)
        0x09, 0x32,       //   Usage (Z)
        0x09, 0x35,       //   Usage (Rz)
        0x75, 0x08,       //   Report Size (8)
        0x95, 0x04,       //   Report Count (4)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
        0x09, 0x20,       //   Usage (0x20)
        0x09, 0x21,       //   Usage (0x21)
        0x09, 0x22,       //   Usage (0x22)
        0x09, 0x23,       //   Usage (0x23)
        0x09, 0x24,       //   Usage (0x24)
        0x09, 0x25,       //   Usage (0x25)
        0x09, 0x26,       //   Usage (0x26)
        0x09, 0x27,       //   Usage (0x27)
        0x09, 0x28,       //   Usage (0x28)
        0x09, 0x29,       //   Usage (0x29)
        0x09, 0x2A,       //   Usage (0x2A)
        0x09, 0x2B,       //   Usage (0x2B)
        0x95, 0x0C,       //   Report Count (12)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0x0A, 0x21, 0x26, //   Usage (0x2621)
        0x95, 0x08,       //   Report Count (8)
        0xB1, 0x02, //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position,Non-volatile)
        0x0A, 0x21, 0x26, //   Usage (0x2621)
        0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position,Non-volatile)
        0x26, 0xFF, 0x03, //   Logical Maximum (1023)
        0x46, 0xFF, 0x03, //   Physical Maximum (1023)
        0x09, 0x2C,       //   Usage (0x2C)
        0x09, 0x2D,       //   Usage (0x2D)
        0x09, 0x2E,       //   Usage (0x2E)
        0x09, 0x2F,       //   Usage (0x2F)
        0x75, 0x10,       //   Report Size (16)
        0x95, 0x04,       //   Report Count (4)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0xC0,       // End Collection

};
bool ps3_create_report(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                       uint8_t *const ReportID, const uint8_t ReportType,
                       void *ReportData, uint16_t *const ReportSize) {

  USB_PS3Report_Data_t *JoystickReport = (USB_PS3Report_Data_t *)ReportData;
  uint8_t button;
  for (uint8_t i = 0; i < currentBindingLen; i++) {
    button = pgm_read_byte(currentBindings + i);
    if (button == 0xff) continue;
    bool bit_set = bit_check(controller.buttons, button);
    bit_write(bit_set, JoystickReport->buttons, i);
  }
  for (uint8_t i = 0; i < sizeof(ps3AxisBindings); i++) {
    button = pgm_read_byte(currentAxisBindings + i);
    if (button == 0xff) continue;
    bool bit_set = bit_check(controller.buttons, button);
    JoystickReport->axis[i] = bit_set ? 0xFF : 0x00;
  }
  button = controller.buttons & 0xF;
  if (button > 0x0a) {
    JoystickReport->hat = 0x08;
  } else {
    JoystickReport->hat = pgm_read_byte(hat_bindings + button);
  }
  if (config.sub_type == PS3_GUITAR_GH_SUBTYPE ||
      config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
    JoystickReport->l_x = 0x80;
    JoystickReport->l_y = 0x80;
    // r_y is tap, so lets disable it.
    JoystickReport->r_y = 0x7d;
    // XINPUT guitars use LB for orange, PS3 uses L
    bit_write(bit_check(controller.buttons, XBOX_LB), JoystickReport->buttons,
              SWITCH_L);
  }
  bool tilt = controller.r_y == 32767;
  if (config.sub_type == PS3_GUITAR_GH_SUBTYPE) {
    JoystickReport->r_x = (controller.r_x >> 8) + 128;
    JoystickReport->accel[0] = tilt ? 0x84 : 0xf7;
    JoystickReport->accel[1] = 0x01;
  }
  if (config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
    JoystickReport->r_x = 128 - (controller.r_x >> 8);
    // RB PS3 guitars use R for a tilt bit
    bit_write(tilt, JoystickReport->buttons, SWITCH_R);
    // Swap y and x, as RB controllers have them inverted
    bit_write(bit_check(controller.buttons, XBOX_Y), JoystickReport->buttons,
              SWITCH_X);
    bit_write(bit_check(controller.buttons, XBOX_X), JoystickReport->buttons,
              SWITCH_Y);
  }
  if (config.sub_type == PS3_GAMEPAD_SUBTYPE) {
    bit_write(controller.lt > 50, JoystickReport->buttons, SWITCH_L);
    bit_write(controller.rt > 50, JoystickReport->buttons, SWITCH_R);
    JoystickReport->axis[4] = controller.lt;
    JoystickReport->axis[5] = controller.rt;
    JoystickReport->l_x = (controller.l_x >> 8) + 128;
    JoystickReport->l_y = (controller.l_y >> 8) + 128;
    JoystickReport->r_x = (controller.r_x >> 8) + 128;
    JoystickReport->r_y = (controller.r_y >> 8) + 128;
  } else {
    //Map start+select to home
    if (bit_check(controller.buttons, XBOX_START) &&
        bit_check(controller.buttons, XBOX_BACK)) {
      bit_set(JoystickReport->buttons, SWITCH_HOME);
    }
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
        switch (config.sub_type) {
        case PS3_DRUM_GH_SUBTYPE:
        case PS3_GUITAR_GH_SUBTYPE:
          Endpoint_Write_8(0x06);
          break;
        case PS3_DRUM_RB_SUBTYPE:
        case PS3_GUITAR_RB_SUBTYPE:
          Endpoint_Write_8(0x00);
          break;
        default:
          Endpoint_Write_8(0x07);
          break;
        }
        for (uint8_t i = 0; i < 4; i++) { Endpoint_Write_8(0x00); }
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
  } else {
    currentAxisBindings = ps3AxisBindings;
    currentBindings = ps3ButtonBindings;
    currentBindingLen = sizeof(ps3ButtonBindings);
    if (config.sub_type > PS3_GAMEPAD_SUBTYPE) {
      DeviceDescriptor.VendorID = 0x12ba;
      currentBindings = ps3ButtonBindings;
      currentBindingLen = sizeof(ps3ButtonBindings) - 2;
    }
    if (config.sub_type == PS3_GUITAR_GH_SUBTYPE) {
      DeviceDescriptor.ProductID = 0x0100;
      currentAxisBindings = ghAxisBindings;
    } else if (config.sub_type == PS3_GUITAR_RB_SUBTYPE) {
      DeviceDescriptor.ProductID = 0x0200;
    } else if (config.sub_type == PS3_DRUM_GH_SUBTYPE) {
      DeviceDescriptor.ProductID = 0x0120;
      currentAxisBindings = ghAxisBindings;
    } else if (config.sub_type == PS3_DRUM_RB_SUBTYPE) {
      DeviceDescriptor.ProductID = 0x0210;
    }
  }
}