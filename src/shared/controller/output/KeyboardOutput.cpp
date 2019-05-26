#include "KeyboardOutput.h"
/** HID class report descriptor. This is a special descriptor constructed
with
 * values from the USBIF HID class specification to describe the reports and
 * capabilities of the HID device. This descriptor is parsed by the host and
 its
 * contents used to determine what data (and in what encoding) the device
 will
 * send, and what it may be sent back from the host. Refer to the HID
 * specification for more details on HID report descriptors.
 */

static uint8_t PrevHIDReport[sizeof(USB_KeyboardReport_Data_t)];
const USB_Descriptor_HIDReport_Datatype_t PROGMEM HIDReport_Datatype[] = {
    HID_DESCRIPTOR_KEYBOARD(SIMULTANEOUS_KEYS)};
USB_ClassInfo_HID_Device_t interface = {
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
USB_ClassInfo_HID_Device_t *KeyboardOutput::createHIDInterface() {
  return &interface;
}
const size_t KeyboardOutput::ReportDatatypeSize() {
  return sizeof(HIDReport_Datatype);
}
uint8_t keys[SIMULTANEOUS_KEYS];
uint8_t usedKeys = 0;

void KeyboardOutput::update(Controller controller) {
  USB_USBTask();
  wdt_reset();
  usedKeys = 0;
  for (int i = 0; i <= XBOX_Y; i++) {
    auto binding = ((uint8_t *)&config.keys)[i];
    if (binding && bit_check(controller.buttons, i)) {
      keys[usedKeys++] = binding;
    }
  }
  CHECK_JOY_KEY(l_x);
  CHECK_JOY_KEY(l_y);
  CHECK_JOY_KEY(r_x);
  CHECK_JOY_KEY(r_y);
  CHECK_TRIGGER_KEY(lt);
  CHECK_TRIGGER_KEY(rt);
  HID_Device_USBTask(HID_Interface);
}

bool KeyboardOutput::hid_create_report(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  USB_KeyboardReport_Data_t *KeyboardReport =
      (USB_KeyboardReport_Data_t *)ReportData;
  for (int i = 0; i < usedKeys; i++) {
    KeyboardReport->KeyCode[i] = keys[i];
  }
  *ReportSize = sizeof(USB_KeyboardReport_Data_t);
  return false;
}

uint16_t
KeyboardOutput::get_hid_descriptor(const uint8_t DescriptorType,
                                   const uint8_t DescriptorNumber,
                                   const void **const DescriptorAddress,
                                   uint8_t *const DescriptorMemorySpace) {
  *DescriptorAddress = HIDReport_Datatype;
  *DescriptorMemorySpace = MEMSPACE_FLASH;
  return ReportDatatypeSize();
}