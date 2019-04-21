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
const USB_Descriptor_HIDReport_Datatype_t PROGMEM GamepadReport[] = {
            HID_RI_USAGE_PAGE(8, 0x01),                     \
            HID_RI_USAGE(8, 0x04),                          \
            HID_RI_COLLECTION(8, 0x01),                     \
                HID_RI_USAGE(8, 0x01),                      \
                HID_RI_COLLECTION(8, 0x00),                 \
                  HID_RI_USAGE(8, 0x30),                    \
                  HID_RI_USAGE(8, 0x31),                    \
                  HID_RI_LOGICAL_MINIMUM(16, -32767),       \
                  HID_RI_LOGICAL_MAXIMUM(16,  32767),       \
                  HID_RI_REPORT_SIZE(8, 16),                \
                  HID_RI_REPORT_COUNT(8, 2),                \
                  HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE), \
                HID_RI_END_COLLECTION(0),                   \
                HID_RI_USAGE_PAGE(8, 0x09),                 \
                HID_RI_USAGE_MINIMUM(8, 0x01),              \
                HID_RI_USAGE_MAXIMUM(8, GAMEPAD_BTN_COUNT), \
                HID_RI_LOGICAL_MINIMUM(8, 0x00),            \
                HID_RI_LOGICAL_MAXIMUM(8, 0x01),            \
                HID_RI_REPORT_SIZE(8, 0x01),                \
                HID_RI_REPORT_COUNT(8, GAMEPAD_BTN_COUNT),  \
                HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE), \
            HID_RI_END_COLLECTION(0)
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

            .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
            .TotalInterfaces        = 1,

            .ConfigurationNumber    = 1,
            .ConfigurationStrIndex  = NO_DESCRIPTOR,

            .ConfigAttributes       = (USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_SELFPOWERED),

            .MaxPowerConsumption    = USB_CONFIG_POWER_MA(500)
        },

    .HID_Interface =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

            .InterfaceNumber        = INTERFACE_ID_Gamepad,
            .AlternateSetting       = 0x00,

            .TotalEndpoints         = 1,

            .Class                  = HID_CSCP_HIDClass,
            .SubClass               = HID_CSCP_NonBootSubclass,
            .Protocol               = HID_CSCP_NonBootProtocol,

            .InterfaceStrIndex      = NO_DESCRIPTOR
        },

    .HID_GamepadHID =
        {
            .Header                 = {.Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID},

            .HIDSpec                = VERSION_BCD(1,1,1),
            .CountryCode            = 0x00,
            .TotalReportDescriptors = 1,
            .HIDReportType          = HID_DTYPE_Report,
            .HIDReportLength        = sizeof(GamepadReport)
        },

    .HID_ReportINEndpoint =
        {
            .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

            .EndpointAddress        = JOYSTICK_EPADDR,
            .Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
            .EndpointSize           = JOYSTICK_EPSIZE,
            .PollingIntervalMS      = POLL_RATE
        }
};
/** Buffer to hold the previously generated HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevGamepadHIDReportBuffer[sizeof(USB_GamepadReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Gamepad_HID_Interface =
    {
        .Config =
            {
                .InterfaceNumber              = INTERFACE_ID_Gamepad,
                .ReportINEndpoint             =
                    {
                        .Address              = JOYSTICK_EPADDR,
                        .Size                 = JOYSTICK_EPSIZE,
                        .Banks                = 1,
                    },
                .PrevReportINBuffer           = PrevGamepadHIDReportBuffer,
                .PrevReportINBufferSize       = sizeof(PrevGamepadHIDReportBuffer),
            },
    };

uint16_t last_controller_buttons = 0;
uint16_t last_controller_r_x = 0;
uint16_t last_controller_r_y = 0;

Output::Output() {}

void Output::init() {
  wdt_enable(WDTO_2S);
  USB_Init();
  sei();
}

void Output::update(Controller controller) {
  USB_USBTask();
  wdt_reset();

  // grab button state from controller
  last_controller_buttons = controller.buttons;
  last_controller_r_x = controller.r_x;
  last_controller_r_y = controller.r_y;
  HID_Device_USBTask(&Gamepad_HID_Interface);
}

bool Output::ready() {
    return true;
}

void Output::usb_connect() {}

void Output::usb_disconnect() {}

void Output::usb_configuration_changed() {
  bool ConfigSuccess = true;
  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Gamepad_HID_Interface);
  USB_Device_EnableSOFEvents();
}

void Output::usb_control_request() {
  HID_Device_ProcessControlRequest(&Gamepad_HID_Interface);
}

void Output::usb_start_of_frame() {
  HID_Device_MillisecondElapsed(&Gamepad_HID_Interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {

  USB_GamepadReport_Data_t* JoystickReport = (USB_GamepadReport_Data_t*)ReportData;

  // update report
  JoystickReport->Button = last_controller_buttons;

  JoystickReport->r_x = last_controller_r_x;
  JoystickReport->r_y = last_controller_r_y;

  *ReportSize = sizeof(USB_GamepadReport_Data_t);

  return false;
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {
  // Unused (but mandatory for the HID class driver) in this class, since there are no Host->Device reports
}
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
    Address = &ConfigurationDescriptor.HID_GamepadHID;
    Size = sizeof(USB_HID_Descriptor_HID_t);
    break;
  case HID_DTYPE_Report:
    Address = &GamepadReport;
    Size = sizeof(GamepadReport);
    break;
  }
  *DescriptorAddress = Address;
  return Size;
}
#endif