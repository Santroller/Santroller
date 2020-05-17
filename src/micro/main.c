#include "../shared/Arduino.h"
#include "../shared/config/eeprom.h"
#include "../shared/input/input_handler.h"
#include "../shared/output/bootloader/bootloader.h"
#include "../shared/output/control_requests.h"
#include "../shared/output/reports.h"
#include "../shared/output/serial_handler.h"
#include "../shared/output/usb/Descriptors.h"
#include "../shared/util.h"
#include "../shared/input/input_direct.h"
#include "stdbool.h"
controller_t controller;
output_report_size_t previousReport;
output_report_size_t currentReport;
USB_ClassInfo_CDC_Device_t serialInterface = {
    .Config =
        {
            .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .DataINEndpoint =
                {
                    .Address = CDC_TX_EPADDR,
                    .Size = CDC_TX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_RX_EPSIZE,
                    .Banks = 1,
                },
            .NotificationEndpoint =
                {
                    .Address = CDC_NOTIFICATION_EPADDR,
                    .Size = CDC_NOTIFICATION_EPSIZE,
                    .Banks = 1,
                },
        },
};
USB_ClassInfo_HID_Device_t hidInterface = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : DEVICE_EPADDR_IN,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : &previousReport,
    PrevReportINBufferSize : sizeof(output_report_size_t),
  },
};
USB_ClassInfo_MIDI_Device_t midiInterface = {
    .Config =
        {
            .StreamingInterfaceNumber = INTERFACE_ID_AudioStream,
            .DataINEndpoint =
                {
                    .Address = DEVICE_EPADDR_IN,
                    .Size = HID_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = DEVICE_EPADDR_OUT,
                    .Size = HID_EPSIZE,
                    .Banks = 1,
                },
        },
};
int main(void) {
  loadConfig();
  device_type = config.main.sub_type;
  initInputs();
  initReports();
  USB_Init();
  sei();
  uint16_t size;
  uint16_t bytesReceived;
  while (true) {
    tickInputs(&controller);
    fillReport(&currentReport, &size, &controller);
    if (memcmp(&currentReport, &previousReport, size) != 0) {
      memcpy(&previousReport, &currentReport, size);
      Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
      if (Endpoint_IsReadWriteAllowed()) {
        Endpoint_Write_Stream_LE(&currentReport, size, NULL);
        Endpoint_ClearIN();
      }
    }

    bytesReceived = CDC_Device_BytesReceived(&serialInterface);
    while (bytesReceived--) {
      processSerialData(CDC_Device_ReceiveByte(&serialInterface) &
                     0xff);
    }
    if (pinDetected) {
      pinDetected = false;
      writeToSerial('d');
      writeToSerial(detectedPin);
      writeToSerial('\r');
      writeToSerial('\n');
    }
    CDC_Device_USBTask(&serialInterface);
    MIDI_Device_USBTask(&midiInterface);
  }
}

void writeToSerial(uint8_t data) {
  CDC_Device_SendByte(&serialInterface, data);
}

void EVENT_USB_Device_ConfigurationChanged(void) {
  if (device_type >= MIDI_GUITAR) {
    MIDI_Device_ConfigureEndpoints(&midiInterface);
  } else {
    HID_Device_ConfigureEndpoints(&hidInterface);
  }
  CDC_Device_ConfigureEndpoints(&serialInterface);
}
void EVENT_USB_Device_ControlRequest(void) {
  // Handle control requests for the device, such as xinput and hid requests
  deviceControlRequest();
  CDC_Device_ProcessControlRequest(&serialInterface);
}
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  // Jump to the bootloader on a 1200bps touch, this is how the arduino ide
  // signals for a 32u4 to jump into bootloader mode.
  bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
                    CDC_CONTROL_LINE_OUT_DTR) == 0;
  if (HostReady && CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200) {
    bootloader();
  }
}