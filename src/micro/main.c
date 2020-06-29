#include "bootloader/bootloader.h"
#include "config/eeprom.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "leds/leds.h"
#include "output/control_requests.h"
#include "output/descriptors.h"
#include "output/reports.h"
#include "output/reports/xinput.h"
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
#ifndef MULTI_ADAPTOR
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
#endif
bool xinputEnabled = false;
int main(void) {
  loadConfig();
  config.main.inputType = PS2;
  config.main.subType = KEYBOARD_GAMEPAD;
  // config.midi.channel[XBOX_A] = 1;
  // config.midi.midiType[XBOX_A] = NOTE;
  // config.midi.note[XBOX_A] = 0x5F;
  deviceType = config.main.subType;
  initInputs();
  initLEDs();
  initReports();
  USB_Init();
  sei();
  uint16_t size;
  while (true) {
    tickInputs(&controller);
    tickLEDs(&controller);
    fillReport(&currentReport, &size, &controller);
    if (memcmp(&currentReport, &previousReport, size) != 0) {
      memcpy(&previousReport, &currentReport, size);
      uint8_t *data = (uint8_t *)&currentReport;
      uint8_t rid = *data;
      switch (rid) {
      case REPORT_ID_XINPUT:
        Endpoint_SelectEndpoint(XINPUT_EPADDR_IN);
        break;
      case REPORT_ID_MIDI:
        Endpoint_SelectEndpoint(MIDI_EPADDR_IN);
        // The "reportid" is actually not a real thing on midi, so we need to
        // strip it before we send data.
        data++;
        size--;
        break;
      case REPORT_ID_GAMEPAD:
        // Consoles don't support multiple report ids, so we strip them here
        // too. PS3's technically do support them, but at the cost of not being
        // able to identify the controller.
        Endpoint_SelectEndpoint(HID_EPADDR_IN);
        data++;
        size--;
        break;
      case REPORT_ID_KBD:
      case REPORT_ID_MOUSE:
        Endpoint_SelectEndpoint(HID_EPADDR_IN);
        break;
      }
      if (Endpoint_IsReadWriteAllowed()) {
        Endpoint_Write_Stream_LE(data, size, NULL);
        Endpoint_ClearIN();
      }
    }
#ifndef MULTI_ADAPTOR
    MIDI_Device_USBTask(&midiInterface);
#endif
  }
}

void EVENT_USB_Device_ConfigurationChanged(void) {
  Endpoint_ConfigureEndpoint(XINPUT_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(HID_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
  Endpoint_ConfigureEndpoint(HID_EPADDR_OUT, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
#ifndef MULTI_ADAPTOR
  Endpoint_ConfigureEndpoint(MIDI_EPADDR_IN, EP_TYPE_BULK, HID_EPSIZE, 1);
  Endpoint_ConfigureEndpoint(XINPUT_EPADDR_OUT, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(MIDI_EPADDR_OUT, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
#else
  Endpoint_ConfigureEndpoint(XINPUT_2_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(XINPUT_3_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(XINPUT_4_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
#endif
}
void EVENT_USB_Device_ControlRequest(void) { deviceControlRequest(); }
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