
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "bootloader/bootloader.h"
#include "controller/guitar_includes.h"
#include "eeprom/eeprom.h"
#include "input/input_handler.h"
#include "leds/leds.h"
#include "output/control_requests.h"
#include "output/descriptors.h"
#include "output/reports.h"
#include "output/serial_handler.h"
#include "pins/pins.h"
#include "rf/rf.h"
#include "stdbool.h"
#include "timer/timer.h"
#include "usb/usb.h"
#include "util/util.h"
#include <stdlib.h>
#define ARDUINO_MAIN
#include "pins_arduino.h"
Controller_t prevController;
Controller_t controller;
USB_Report_Data_t currentReport;
uint8_t size;
bool xinputEnabled = false;
bool isRF = false;
bool typeIsGuitar;
bool typeIsDrum;
long lastPoll = 0;
uint8_t inputType;
uint8_t pollRate;
void initialise(void) {
  Configuration_t config = loadConfig();
  fullDeviceType = config.main.subType;
  deviceType = fullDeviceType;
  pollRate = config.main.pollRate;
  inputType = config.main.inputType;
  typeIsDrum = isDrum(fullDeviceType);
  typeIsGuitar = isGuitar(fullDeviceType);
  if (typeIsGuitar && deviceType <= XINPUT_ARCADE_PAD) {
    deviceType = REAL_GUITAR_SUBTYPE;
  }
  if (typeIsDrum && deviceType <= XINPUT_ARCADE_PAD) {
    deviceType = REAL_DRUM_SUBTYPE;
  }
  setupMicrosTimer();
  if (config.rf.rfInEnabled) {
    initRF(false, config.rf.id, generate_crc32());
    isRF = true;
  } else {
    initInputs(&config);
  }
  initReports(&config);
  initLEDs(&config);
  USB_Init();
  sei();
}
int main(void) {
  initialise();
  uint8_t cSize = sizeof(XInput_Data_t);
  while (true) {
    USB_USBTask();
    if (isRF) {
      tickRFInput((uint8_t *)&controller, cSize);
    } else {
      tickInputs(&controller);
      tickLEDs(&controller);
      if (millis() - lastPoll < pollRate) { continue; }
    }
    if (memcmp(&controller, &prevController, cSize) != 0 &&
        Endpoint_IsINReady()) {
      fillReport(&currentReport, &size, &controller);
      if (size) {
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
          // The wii does not support multiple report ids. So we also strip it
          // here
          Endpoint_SelectEndpoint(HID_EPADDR_IN);
          data++;
          size--;
          break;
        case REPORT_ID_KBD:
        case REPORT_ID_MOUSE:
          Endpoint_SelectEndpoint(HID_EPADDR_IN);
          break;
        }
        memcpy(&prevController, &controller, cSize);
        Endpoint_Write_Stream_LE(data, size, NULL);
        Endpoint_ClearIN();
      }
    }
  }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  Endpoint_ConfigureEndpoint(XINPUT_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(HID_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
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
  uint8_t buf[66];
  buf[0] = cmd;
  buf[1] = false;
  Endpoint_ClearSETUP();
  Endpoint_Read_Control_Stream_LE(buf + 2, data_len);
  processHIDWriteFeatureReport(cmd, data_len, buf + 2);
  Endpoint_ClearStatusStage();
  if (isRF) {
    uint8_t buf2[32];
    while (nrf24_txFifoFull()) {
      rf_interrupt = true;
      tickRFInput(buf2, 0);
      nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
    }
    nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
    nrf24_writeAckPayload(buf, 32);
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