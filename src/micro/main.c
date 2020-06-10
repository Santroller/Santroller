#include "bootloader/bootloader.h"
#include "config/eeprom.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "leds/leds.h"
#include "output/control_requests.h"
#include "output/descriptors.h"
#include "output/reports.h"
#include "output/serial_handler.h"
#include "stdbool.h"
#include "util/util.h"
Controller_t controller;
USB_Report_Data_t previousReport;
USB_Report_Data_t currentReport;
USB_ClassInfo_HID_Device_t hidInterface = {
  Config : {
    InterfaceNumber : INTERFACE_ID_HID,
    ReportINEndpoint : {
      Address : HID_EPADDR_IN,
      Size : HID_EPSIZE,
      Type : EP_TYPE_CONTROL,
      Banks : 1,
    },
    PrevReportINBuffer : &previousReport,
    PrevReportINBufferSize : sizeof(USB_Report_Data_t),
  },
};
USB_ClassInfo_MIDI_Device_t midiInterface = {
    .Config =
        {
            .StreamingInterfaceNumber = INTERFACE_ID_AudioStream,
            .DataINEndpoint =
                {
                    .Address = MIDI_EPADDR_IN,
                    .Size = HID_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = MIDI_EPADDR_OUT,
                    .Size = HID_EPSIZE,
                    .Banks = 1,
                },
        },
};
int main(void) {
  loadConfig();
  // config.main.subType = KEYBOARD_GAMEPAD;
  deviceType = config.main.subType;
  initInputs();
  initLEDs();
  initReports();
  USB_Init();
  sei();
  uint16_t size;
  // uint16_t bytesReceived;
  while (true) {
    tickInputs(&controller);
    tickLEDs(&controller);
    fillReport(&currentReport, &size, &controller);
    if (memcmp(&currentReport, &previousReport, size) != 0) {
      memcpy(&previousReport, &currentReport, size);
      Endpoint_SelectEndpoint(HID_EPADDR_IN);
      if (Endpoint_IsReadWriteAllowed()) {
        Endpoint_Write_Stream_LE(&currentReport, size, NULL);
        Endpoint_ClearIN();
      }
    }
    // if (foundPin) {
    //   foundPin = false;
    //   writeToSerial('d');
    //   writeToSerial(detectedPin);
    //   writeToSerial('\r');
    //   writeToSerial('\n');
    // }
    MIDI_Device_USBTask(&midiInterface);
  }
}

void writeToSerial(uint8_t data) {
  // CDC_Device_SendByte(&serialInterface, data);
}

void EVENT_USB_Device_ConfigurationChanged(void) {
  Endpoint_ConfigureEndpoint(XINPUT_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(HID_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
  Endpoint_ConfigureEndpoint(HID_EPADDR_OUT, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
  Endpoint_ConfigureEndpoint(MIDI_EPADDR_IN, EP_TYPE_BULK, HID_EPSIZE, 1);
}
void EVENT_USB_Device_ControlRequest(void) {
  deviceControlRequest();
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