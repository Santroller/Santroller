#include "GamepadOutput.h"

static uint8_t PrevHIDReport[sizeof(USB_GamepadReport_Data_t)];

/** HID class report descriptor. This is a special descriptor constructed with
 * values from the USBIF HID class specification to describe the reports and
 * capabilities of the HID device. This descriptor is parsed by the host and its
 * contents used to determine what data (and in what encoding) the device will
 * send, and what it may be sent back from the host. Refer to the HID
 * specification for more details on HID report descriptors.
 */
#define GAMEPAD_BTN_COUNT 14

const USB_Descriptor_HIDReport_Datatype_t PROGMEM HIDReport_Datatype[] = {
    HID_RI_USAGE_PAGE(8, 0x01),
    HID_RI_USAGE(8, 0x04),
    HID_RI_COLLECTION(8, 0x01),
    HID_RI_USAGE_PAGE(8, 0x09),
    HID_RI_USAGE_MINIMUM(8, 0x01),
    HID_RI_USAGE_MAXIMUM(8, GAMEPAD_BTN_COUNT),
    HID_RI_LOGICAL_MINIMUM(8, 0x00),
    HID_RI_LOGICAL_MAXIMUM(8, 0x01),
    HID_RI_REPORT_SIZE(8, 0x01),
    HID_RI_REPORT_COUNT(8, GAMEPAD_BTN_COUNT),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_RI_USAGE(8, 0x01),
    HID_RI_COLLECTION(8, 0x00),
    HID_RI_USAGE(8, 0x32), // LT+RT = Z
    HID_RI_USAGE(8, 0x30), // l_x = X
    HID_RI_USAGE(8, 0x31), // l_y = Y
    HID_RI_USAGE(8, 0x33), // r_x = Rx
    HID_RI_USAGE(8, 0x34), // r_y = Ry
    HID_RI_LOGICAL_MINIMUM(16, -32767),
    HID_RI_LOGICAL_MAXIMUM(16, 32767),
    HID_RI_REPORT_SIZE(8, 16),
    HID_RI_REPORT_COUNT(8, 5),
    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
    HID_RI_END_COLLECTION(0),
    HID_RI_END_COLLECTION(0)};

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
  memcpy(JoystickReport, &last_controller, sizeof(Controller));

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