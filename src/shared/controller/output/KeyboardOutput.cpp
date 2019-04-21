#include "KeyboardOutput.h"
#if OUTPUT_TYPE == KEYBOARD
extern "C" {
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

USB_ClassInfo_HID_Device_t Keyboard_HID_Interface = {
    .Config =
        {
            .InterfaceNumber = INTERFACE_ID_Keyboard,
            .ReportINEndpoint =
                {
                    .Address = KEYBOARD_EPADDR,
                    .Size = KEYBOARD_EPSIZE,
                    .Banks = 1,
                },
            .PrevReportINBuffer = PrevKeyboardHIDReportBuffer,
            .PrevReportINBufferSize = sizeof(PrevKeyboardHIDReportBuffer),
        },
};
}
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
const USB_Descriptor_HIDReport_Datatype_t PROGMEM KeyboardReport[] = {
    /* Use the HID class driver's standard Keyboard report.
     *   Max simultaneous keys: 6
     */
    HID_DESCRIPTOR_KEYBOARD(6)};

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {.Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t),
                          .Type = DTYPE_Configuration},

               .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
               .TotalInterfaces = 1,

               .ConfigurationNumber = 1,
               .ConfigurationStrIndex = NO_DESCRIPTOR,

               .ConfigAttributes =
                   (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

               .MaxPowerConsumption = USB_CONFIG_POWER_MA(500)},

    .HID_Interface = {.Header = {.Size = sizeof(USB_Descriptor_Interface_t),
                                 .Type = DTYPE_Interface},

                      .InterfaceNumber = INTERFACE_ID_Keyboard,
                      .AlternateSetting = 0x00,

                      .TotalEndpoints = 1,

                      .Class = HID_CSCP_HIDClass,
                      .SubClass = HID_CSCP_BootSubclass,
                      .Protocol = HID_CSCP_KeyboardBootProtocol,

                      .InterfaceStrIndex = NO_DESCRIPTOR},

    .HID_KeyboardHID = {.Header = {.Size = sizeof(USB_HID_Descriptor_HID_t),
                                   .Type = HID_DTYPE_HID},

                        .HIDSpec = VERSION_BCD(1, 1, 1),
                        .CountryCode = 0x00,
                        .TotalReportDescriptors = 1,
                        .HIDReportType = HID_DTYPE_Report,
                        .HIDReportLength = sizeof(KeyboardReport)},

    .HID_ReportINEndpoint = {.Header = {.Size =
                                            sizeof(USB_Descriptor_Endpoint_t),
                                        .Type = DTYPE_Endpoint},

                             .EndpointAddress = KEYBOARD_EPADDR,
                             .Attributes =
                                 (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC |
                                  ENDPOINT_USAGE_DATA),
                             .EndpointSize = KEYBOARD_EPSIZE,
                             .PollingIntervalMS = POLL_RATE},
};
const USB_Descriptor_HIDReport_Datatype_t PROGMEM GenericReport[] = {
    HID_DESCRIPTOR_VENDOR(0x00, 0x01, 0x02, 0x03, GENERIC_REPORT_SIZE)};
#define CHECK_KEY(key)                                                         \
  if (bit_check(controller.buttons, key)) {                                    \
    keys[usedKeys++] = KEY_##key;                                              \
  }
#define CHECK_FRET(key)                                                        \
  if (bit_check(controller.buttons, key)) {                                    \
    keys[usedKeys++] = KEY_##key##_FRET;                                       \
  }
#define CHECK_KEY2(key, condition)                                             \
  if (bit_check(controller.buttons, key) || condition) {                       \
    keys[usedKeys++] = KEY_##key;                                              \
  }
void Output::usb_connect() {}
void Output::usb_disconnect() {}

uint8_t keys[SIMULTANEOUS_KEYS];
uint8_t usedKeys = 0;

void Output::init() {
  wdt_enable(WDTO_2S);
  USB_Init();
  sei();
}
void Output::update(Controller controller) {
  USB_USBTask();
  wdt_reset();
  usedKeys = 0;
  if (controller.r_x < -8000) {
    keys[usedKeys++] = KEY_WHAMMY;
  }
  CHECK_FRET(GREEN);
  CHECK_FRET(RED);
  CHECK_FRET(YELLOW);
  CHECK_FRET(BLUE);
  CHECK_FRET(ORANGE);
  CHECK_KEY(UP);
  CHECK_KEY(DOWN);
  CHECK_KEY(LEFT);
  CHECK_KEY(RIGHT);
  CHECK_KEY(START);
  CHECK_KEY2(SELECT, controller.r_y == 32767);
  HID_Device_USBTask(&Keyboard_HID_Interface);
}

void Output::usb_configuration_changed() {
  bool ConfigSuccess = true;
  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
  USB_Device_EnableSOFEvents();
}

void Output::usb_control_request() {
  if (USB_ControlRequest.bRequest == 0x30) {
    bootloader();
  }
  HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

void Output::usb_start_of_frame() {
  HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

Output::Output() {}
bool Output::ready() { return true; }
bool CALLBACK_HID_Device_CreateHIDReport(
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

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {}

uint16_t Output::get_descriptor(const uint8_t DescriptorType,
                                const uint8_t DescriptorNumber,
                                const void **const DescriptorAddress) {
  uint16_t Size = NO_DESCRIPTOR;
  const void *Address = NULL;
  switch (DescriptorType) {
  case DTYPE_Configuration:
    Address = &ConfigurationDescriptor;
    Size = sizeof(ConfigurationDescriptor);
    break;
  case HID_DTYPE_HID:
    Address = &ConfigurationDescriptor.HID_KeyboardHID;
    Size = sizeof(USB_HID_Descriptor_HID_t);
    break;
  case HID_DTYPE_Report:
    Address = &KeyboardReport;
    Size = sizeof(KeyboardReport);
    break;
  }
  *DescriptorAddress = Address;
  return Size;
}
#endif