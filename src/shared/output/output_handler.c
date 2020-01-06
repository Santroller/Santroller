#include "output_handler.h"
#include "avr/wdt.h"
#include "bootloader/bootloader.h"
#include "output_keyboard.h"
#include "output_ps3.h"
#include "output_serial.h"
#include "output_xinput.h"
#include "usb/Descriptors.h"
void (*control_request)(void);
void (*create_hid_report)(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo,
                            uint8_t *const ReportID, const uint8_t ReportType,
                            void *ReportData, uint16_t *const ReportSize);
USB_ClassInfo_HID_Device_t interface = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : HID_EPADDR_IN,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : NULL,
    PrevReportINBufferSize : sizeof(output_report_size_t),
  },
};
USB_HID_Descriptor_HID_t hid_descriptor = {
  Header : {Size : sizeof(USB_HID_Descriptor_HID_t), Type : HID_DTYPE_HID},

  HIDSpec : VERSION_BCD(1, 1, 1),
  CountryCode : 0x00,
  TotalReportDescriptors : 1,
  HIDReportType : HID_DTYPE_Report,
  HIDReportLength : 0
};
void hid_init(void) {
  memmove(&ConfigurationDescriptor.Controller.HID.Endpoints,
          &ConfigurationDescriptor.Controller.XInput.Endpoints,
          sizeof(ConfigurationDescriptor.Controller.XInput.Endpoints));
  // And now adjust the total size as the HID layout is actually smaller
  ConfigurationDescriptor.Config.TotalConfigurationSize -=
      sizeof(USB_HID_XBOX_Descriptor_HID_t) - sizeof(USB_HID_Descriptor_HID_t);

  hid_descriptor.HIDReportLength = hid_report_size;
  ConfigurationDescriptor.Controller.HID.HIDDescriptor = hid_descriptor;
  // Report that we have an HID device
  ConfigurationDescriptor.Interface0.Class = HID_CSCP_HIDClass;
  ConfigurationDescriptor.Interface0.SubClass = HID_CSCP_NonBootSubclass;
  ConfigurationDescriptor.Interface0.Protocol = HID_CSCP_NonBootProtocol;
}
void output_init(void) {
  ConfigurationDescriptor.Controller.XInput.Endpoints.DataInEndpoint0
      .PollingIntervalMS = config.main.poll_rate;
  if (config.main.sub_type >= KEYBOARD_SUBTYPE) {
    if (config.main.sub_type == KEYBOARD_SUBTYPE) {
      keyboard_init();
    } else {
      ps3_init();
    }
    hid_init();
  } else {
    xinput_init();
  }

  USB_Init();
  sei();
}

void output_tick() {
  wdt_reset();
  HID_Device_USBTask(&interface);
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  HID_Device_ConfigureEndpoints(&interface);
  USB_Device_EnableSOFEvents();
  serial_configuration_changed();
}
void EVENT_USB_Device_ControlRequest(void) {
  if (control_request) {
    control_request();
  } else {
    HID_Device_ProcessControlRequest(&interface);
  }
  serial_control_request();
}
void EVENT_USB_Device_StartOfFrame(void) {
  HID_Device_MillisecondElapsed(&interface);
}

bool CALLBACK_HID_Device_CreateHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
    const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  create_hid_report(HIDInterfaceInfo, ReportID, ReportType, ReportData,
                           ReportSize);
  return true;
}

void CALLBACK_HID_Device_ProcessHIDReport(
    USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
    const uint8_t ReportType, const void *ReportData,
    const uint16_t ReportSize) {}

static char *FW = ARDWIINO_BOARD;
int16_t process_serial(USB_ClassInfo_CDC_Device_t *VirtualSerial_CDC_Interface) {
  int16_t b = CDC_Device_ReceiveByte(VirtualSerial_CDC_Interface);
  uint8_t *from;
  uint8_t *to;
  uint16_t size = 0;
  bool w = false;
  switch (b) {
  case MAIN_CMD_W:
    w = true;
  case MAIN_CMD_R:
    from = (uint8_t*)&config.main;
    to = (uint8_t*)&config_pointer.main;
    size = sizeof(main_config_t);
    break;
  case PIN_CMD_W:
    w = true;
  case PIN_CMD_R:
    from = (uint8_t*)&config.pins;
    to = (uint8_t*)&config_pointer.pins;
    size = sizeof(pins_t);
    break;
  case AXIS_CMD_W:
    w = true;
  case AXIS_CMD_R:
    from = (uint8_t*)&config.axis;
    to = (uint8_t*)&config_pointer.axis;
    size = sizeof(axis_config_t);
    break;
  case KEY_CMD_W:
    w = true;
  case KEY_CMD_R:
    from = (uint8_t*)&config.keys;
    to = (uint8_t*)&config_pointer.keys;
    size = sizeof(keys_t);
    break;
  case CONTROLLER_CMD_R:
    from = (uint8_t*)&controller;
    size = sizeof(controller_t);
    break;
  case FW_CMD_R:
    from = (uint8_t*)FW;
    size = strlen(FW);
    break;
  case REBOOT_CMD:
    reboot();
    break;
  }
  if (size > 0) {
    if (w) {
      CDC_Device_SendString(VirtualSerial_CDC_Interface, "READY");
      while (size > 0) {
        eeprom_update_byte(to++, CDC_Device_ReceiveByte(VirtualSerial_CDC_Interface));
        size--;
      }
      CDC_Device_SendString(VirtualSerial_CDC_Interface, "OK");
    } else {
      CDC_Device_SendData(VirtualSerial_CDC_Interface, from, size);
    }
  }
  CDC_Device_USBTask(VirtualSerial_CDC_Interface);
  USB_USBTask();
  return b;
}