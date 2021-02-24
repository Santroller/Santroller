#define ARDUINO_MAIN
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "bootloader/bootloader.h"
#include "bsp/board.h"
#include "config/defines.h"
#include "controller/guitar_includes.h"
#include "eeprom/eeprom.h"
#include "input/input_handler.h"
#include "leds/leds.h"
#include "lib/usb/xinput_device.h"
#include "output/control_requests.h"
#include "output/descriptors.h"
#include "output/reports.h"
#include "output/serial_handler.h"
#include "pico/stdlib.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "rf/rf.h"
#include "stdbool.h"
#include "timer/timer.h"
#include "util/util.h"
#include <device/usbd_pvt.h>
#include <pico/unique_id.h>
#include <stdio.h>
#include <stdlib.h>
#include <tusb.h>

#define __INCLUDE_FROM_USB_DRIVER
#include <LUFA/Drivers/USB/Core/StdRequestType.h>
int validAnalog = 0;
uint8_t rhportToWrite;
tusb_control_request_t const *requestToWrite;
TU_ATTR_WEAK bool
tud_vendor_control_request_cb(uint8_t rhport,
                              tusb_control_request_t const *request) {
  if (request->bRequest == HID_REQ_GetReport &&
      (request->bmRequestType ==
       (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
      request->wIndex == INTERFACE_ID_XInput && request->wValue == 0x0000) {
    tud_control_xfer(rhport, request, capabilities1, sizeof(capabilities1));
  } else if (request->bRequest == REQ_GetOSFeatureDescriptor &&
             (request->bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             request->wIndex == EXTENDED_PROPERTIES_DESCRIPTOR &&
             request->wValue == INTERFACE_ID_Config) {
    tud_control_xfer(rhport, request, &ExtendedIDs, ExtendedIDs.TotalLength);
  } else if (request->bRequest == REQ_GetOSFeatureDescriptor &&
             request->bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE) &&
             request->wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {
    tud_control_xfer(rhport, request, &DevCompatIDs, DevCompatIDs.TotalLength);
  } else if (request->bRequest == HID_REQ_GetReport &&
             (request->bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE)) &&
             request->wIndex == 0x00 && request->wValue == 0x0000) {
    tud_control_xfer(rhport, request, ID, sizeof(ID));
  } else if (request->bRequest == HID_REQ_GetReport &&
             (request->bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             request->wIndex == INTERFACE_ID_XInput &&
             request->wValue == 0x0100) {
    tud_control_xfer(rhport, request, capabilities1, sizeof(capabilities2));
  } else if (request->bRequest == HID_REQ_GetReport &&
             request->bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
    rhportToWrite = rhport;
    requestToWrite = request;
    processHIDReadFeatureReport(request->wValue);
  } else if (request->bRequest == HID_REQ_SetReport &&
             request->bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    int cmd = request->wValue;
    uint8_t buf[66];
    tud_control_xfer(rhport, request, buf+2, request->wLength);
    processHIDWriteFeatureReport(cmd, request->wLength, buf + 2);
    if (config.rf.rfInEnabled) {
      uint8_t buf2[32];
      buf[0] = cmd;
      buf[1] = false;
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
}
uint8_t const *tud_descriptor_device_cb(void) {
  if (config.main.subType >= SWITCH_GAMEPAD && config.main.subType < MOUSE) {
    uint8_t offs = config.main.subType - SWITCH_GAMEPAD;
    deviceDescriptor.VendorID = vid[offs];
    deviceDescriptor.ProductID = pid[offs];
  }
  return (uint8_t const *)&deviceDescriptor;
}
uint8_t const *tud_hid_descriptor_report_cb() {
  if (config.main.subType <= KEYBOARD_ROCK_BAND_GUITAR) {
    return kbd_report_descriptor;
  } else {
    return ps3_report_descriptor;
  }
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // for multiple configurations
  uint8_t devt = config.main.subType;
  if (isGuitar(devt)) { devt = REAL_GUITAR_SUBTYPE; }
  if (isDrum(devt)) { devt = REAL_DRUM_SUBTYPE; }
  ConfigurationDescriptor.XInputReserved.subtype = devt;
  if (devt <= KEYBOARD_ROCK_BAND_GUITAR) {
    ConfigurationDescriptor.HIDDescriptor.HIDReportLength =
        sizeof(kbd_report_descriptor);
  }
  // TODO: if we ever implement this stuff, this needs to be implemented
  // again.
  // conf->XInputReserved2.subtype = XINPUT_ARCADE_PAD;
  // conf->XInputReserved3.subtype = XINPUT_DANCE_PAD;
  // conf->XInputReserved4.subtype = REAL_DRUM_SUBTYPE;
  return (uint8_t *)&ConfigurationDescriptor;
}
static uint16_t serialNumber[9];
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  serialNumber[0] = 18 | DTYPE_String << 8;
  if (index == 3) {
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);
    const char chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
      serialNumber[i + 1] =
          chars[(board_id.id[i >> 1] >> ((1 - (i & 1)) << 2)) & 0xF];
    }
    return serialNumber;
  }
  if (index < 3) {
    return (const uint16_t *)descriptorStrings[index];
  } else if (index == 0xEE) {
    return (uint16_t *)&OSDescriptorString;
  }
  return NULL;
}
Controller_t controller;
USB_Report_Data_t previousReport;
USB_Report_Data_t currentReport;
uint8_t size;
bool xinputEnabled = false;
void hid_task(void) {
  const uint32_t interval_ms = config.main.pollRate;
  static uint32_t start_ms = 0;
  if (config.rf.rfInEnabled) {
    tickRFInput((uint8_t *)&controller, sizeof(XInput_Data_t));
  } else {
    tickInputs(&controller);
    tickLEDs(&controller);
    if (millis() - start_ms < interval_ms) return;
  }
  fillReport(&currentReport, &size, &controller);
  if (memcmp(&currentReport, &previousReport, size) != 0) {
    uint8_t *data = (uint8_t *)&currentReport;
    uint8_t rid = *data;
    data++;
    size--;
    switch (rid) {
    case REPORT_ID_XINPUT:
      if (tud_xinput_n_ready(0)) {
        tud_xinput_n_report(0, 0, data, size);
        start_ms = millis();
      }
      break;
#ifndef MULTI_ADAPTOR
    case REPORT_ID_GAMEPAD:
      rid = 0;
    case REPORT_ID_KBD:
    case REPORT_ID_MOUSE:
      if (tud_hid_n_ready(0)) {
        tud_hid_n_report(0, rid, data, size);
        start_ms = millis();
      }
      break;
    case REPORT_ID_MIDI:
      tud_midi_n_send(0, data);
      start_ms = millis();
#endif
    }

    // Remote wakeup
    if (tud_suspended()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
  }
}

int main() {
  board_init();
  tusb_init();
  loadConfig();
  if (config.rf.rfInEnabled) {
    initRF(false, config.rf.id, generate_crc32());
  } else {
    initInputs();
  }
  initReports();
  while (1) {
    tud_task(); // tinyusb device task
    hid_task();
  }
}
void writeToUSB(const void *const Buffer, uint8_t Length) {
  tud_control_xfer(rhportToWrite, requestToWrite, (uint8_t*)Buffer + 1, Length - 1);
}
void stopReading(void) {}
bool tud_vendor_control_complete_cb(uint8_t rhport,
                                    tusb_control_request_t const *request) {
  (void)rhport;
  return true;
}
usbd_class_driver_t driver[] = {
    {.init = xinputd_init,
     .reset = xinputd_reset,
     .open = xinputd_open,
     .control_request = tud_vendor_control_request_cb,
     .control_complete = tud_vendor_control_complete_cb,
     .xfer_cb = xinputd_xfer_cb,
     .sof = NULL}};
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;
  return driver;
}

uint16_t tud_hid_get_report_cb(uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
}
void tud_hid_set_report_cb(uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
  
}