#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "bootloader/bootloader.h"
#include "config/eeprom.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/rf.h"
#include "leds/leds.h"
#include "output/control_requests.h"
#include "output/descriptors.h"
#include "output/reports.h"
#include "output/reports/xinput.h"
#include "output/serial_handler.h"
#include "pins_arduino.h"
#include "stdbool.h"
#include "util/util.h"
#include <stdlib.h>
Controller_t controller;
USB_Report_Data_t previousReport;
USB_Report_Data_t currentReport;
uint8_t size;
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
long lastPoll = 0;
int main(void) {
  loadConfig();
  deviceType = config.main.subType;
  config.rf.rfInEnabled = true;
  setupMicrosTimer();
  if (config.rf.rfInEnabled) {
    config.rf.id = 0xc2292dde;
    // initRF(false, config.rf.id, generate_crc32());
    initRF(false, 0xc2292dde, 0x8581f888);
  } else {
    initInputs();
  }
  initReports();
  USB_Init();
  sei();
  while (true) {
    if (config.rf.rfInEnabled) {
      tickRFInput(&controller);
    } else {
      tickInputs(&controller);
    }
    tickLEDs(&controller);
    if (millis() - lastPoll > config.main.pollRate || config.rf.rfInEnabled) {
      fillReport(&currentReport, &size, &controller);
      if (memcmp(&currentReport, &previousReport, size) != 0 &&
          Endpoint_IsINReady()) {
        lastPoll = millis();
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
          // too. PS3's technically do support them, but at the cost of not
          // being able to identify the controller.
          Endpoint_SelectEndpoint(HID_EPADDR_IN);
          data++;
          size--;
          break;
        case REPORT_ID_KBD:
        case REPORT_ID_MOUSE:
          Endpoint_SelectEndpoint(HID_EPADDR_IN);
          break;
        }
        memcpy(&previousReport, &currentReport, size);
        Endpoint_Write_Stream_LE(data, size, NULL);
        Endpoint_ClearIN();

#ifndef MULTI_ADAPTOR
        MIDI_Device_USBTask(&midiInterface);
#endif
      }
    }
    USB_USBTask();
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
void processHIDWriteFeatureReportControl(uint8_t cmd, uint8_t data_len) {
  uint8_t buf[64];
  Endpoint_ClearSETUP();
  Endpoint_Read_Control_Stream_LE(buf, data_len);
  processHIDWriteFeatureReport(cmd, data_len, buf);
  Endpoint_ClearStatusStage();
  uint8_t buf2[32];
  uint8_t packet = 0;
  for (int i = 0; i < data_len; i += 30) {
    memset(buf2, 0, sizeof(buf2));
    buf2[0] = cmd;
    buf2[1] = packet++;
    uint8_t count = data_len - i;
    memcpy(buf2 + 2, buf + i, count > 30 ? 30 : count);
    while (nrf24_txFifoFull()) {
      rf_interrupt = true;
      tickRFInput(&controller);
      nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
    }
    nrf24_writeAckPayload(buf2, 32);
    rf_interrupt = true;
  }
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
void writeToUSB(const void *const Buffer, uint8_t Length) {
  Endpoint_ClearSETUP();
  Endpoint_Write_Control_Stream_LE(Buffer + 1, Length - 1);
  Endpoint_ClearStatusStage();
}