#include "GamepadOutput.h"

#if OUTPUT_TYPE == GAMEPAD

/** HID class report descriptor. This is a special descriptor constructed with
 * values from the USBIF HID class specification to describe the reports and
 * capabilities of the HID device. This descriptor is parsed by the host and its
 * contents used to determine what data (and in what encoding) the device will
 * send, and what it may be sent back from the host. Refer to the HID
 * specification for more details on HID report descriptors.
 */
#define GAMEPAD_BTN_COUNT 0x10
const USB_Descriptor_HIDReport_Datatype_t PROGMEM HIDReport_Datatype[] = {
    HID_RI_USAGE_PAGE(8, 0x01),
    HID_RI_USAGE(8, 0x04),
    HID_RI_COLLECTION(8, 0x01),
    HID_RI_USAGE(8, 0x01),
    HID_RI_COLLECTION(8, 0x00),
    HID_RI_USAGE(8, 0x30),
    HID_RI_USAGE(8, 0x31),
    HID_RI_LOGICAL_MINIMUM(16, -32767),
    HID_RI_LOGICAL_MAXIMUM(16, 32767),
    HID_RI_REPORT_SIZE(8, 16),
    HID_RI_REPORT_COUNT(8, 2),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_RI_END_COLLECTION(0),
    HID_RI_USAGE_PAGE(8, 0x09),
    HID_RI_USAGE_MINIMUM(8, 0x01),
    HID_RI_USAGE_MAXIMUM(8, GAMEPAD_BTN_COUNT),
    HID_RI_LOGICAL_MINIMUM(8, 0x00),
    HID_RI_LOGICAL_MAXIMUM(8, 0x01),
    HID_RI_REPORT_SIZE(8, 0x01),
    HID_RI_REPORT_COUNT(8, GAMEPAD_BTN_COUNT),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_RI_END_COLLECTION(0)};
const size_t PROGMEM HIDReport_Datasize = sizeof(HIDReport_Datatype);

const size_t PrevHIDReport_size = sizeof(USB_GamepadReport_Data_t);
uint16_t last_controller_buttons = 0;
uint16_t last_controller_r_x = 0;
uint16_t last_controller_r_y = 0;

void Output::update(Controller controller) {
  USB_USBTask();
  wdt_reset();

  // grab button state from controller
  last_controller_buttons = controller.buttons;
  last_controller_r_x = controller.r_x;
  last_controller_r_y = controller.r_y;
  HID_Device_USBTask(&HID_Interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {

  USB_GamepadReport_Data_t *JoystickReport =
      (USB_GamepadReport_Data_t *)ReportData;

  // update report
  JoystickReport->Button = last_controller_buttons;

  JoystickReport->r_x = last_controller_r_x;
  JoystickReport->r_y = last_controller_r_y;

  *ReportSize = sizeof(USB_GamepadReport_Data_t);

  return false;
}
#endif