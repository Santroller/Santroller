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
#include <hardware/sync.h>
#include <pico/unique_id.h>
#include <stdio.h>
#include <stdlib.h>
#include <tusb.h>

#define __INCLUDE_FROM_USB_DRIVER
#include <LUFA/Drivers/USB/Core/StdRequestType.h>
int validAnalog = 0;

bool isRF = false;
bool typeIsGuitar;
bool typeIsDrum;
bool typeIsDJ;
uint8_t inputType;
uint8_t pollRate;

CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[64];
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request) {
  if (request->bRequest == HID_REQ_GetReport &&
      (request->bmRequestType ==
       (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
      request->wIndex == INTERFACE_ID_XInput && request->wValue == 0x0000) {

    if (stage == CONTROL_STAGE_SETUP) {
      tud_control_xfer(rhport, request, capabilities1, sizeof(capabilities1));
    }
  } else if (request->bRequest == REQ_GetOSFeatureDescriptor &&
             (request->bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             request->wIndex == EXTENDED_PROPERTIES_DESCRIPTOR &&
             request->wValue == INTERFACE_ID_Config) {

    if (stage == CONTROL_STAGE_SETUP) {
      tud_control_xfer(rhport, request, &ExtendedIDs, ExtendedIDs.TotalLength);
    }
  } else if (request->bRequest == REQ_GetOSFeatureDescriptor &&
             request->bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE) &&
             request->wIndex == EXTENDED_COMPAT_ID_DESCRIPTOR) {

    if (stage == CONTROL_STAGE_SETUP) {
      tud_control_xfer(rhport, request, &DevCompatIDs,
                       DevCompatIDs.TotalLength);
    }
  } else if (request->bRequest == HID_REQ_GetReport &&
             (request->bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE)) &&
             request->wIndex == 0x00 && request->wValue == 0x0000) {

    if (stage == CONTROL_STAGE_SETUP) {
      uint32_t serial = micros();
      tud_control_xfer(rhport, request, &serial, sizeof(serial));
    }
  } else if (request->bRequest == HID_REQ_GetReport &&
             (request->bmRequestType ==
              (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) &&
             request->wIndex == INTERFACE_ID_XInput &&
             request->wValue == 0x0100) {

    if (stage == CONTROL_STAGE_SETUP) {
      tud_control_xfer(rhport, request, capabilities1, sizeof(capabilities2));
    }
  } else if (request->bRequest == HID_REQ_SetReport &&
             request->bmRequestType ==
                 (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
    if (stage == CONTROL_STAGE_SETUP) {
      tud_control_xfer(rhport, request, buf, request->wLength);
    } else if (stage == CONTROL_STAGE_ACK) {
      int cmd = request->wValue;
      processHIDWriteFeatureReport(cmd, request->wLength, buf);
      if (isRF) {
        uint8_t buf2[32];
        uint8_t buf3[32];
        memcpy(buf3 + 2, buf, 30);
        buf3[0] = cmd;
        buf3[1] = false;
        while (nrf24_txFifoFull()) {
          rf_interrupt = true;
          tickRFInput(buf2, 0);
          nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
        }
        nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
        nrf24_writeAckPayload(buf3, sizeof(buf3));
        rf_interrupt = true;
      }
    }
  } else if (request->bRequest == HID_REQ_GetReport &&
             request->bmRequestType ==
                 (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
    if (stage == CONTROL_STAGE_SETUP) {
      processHIDReadFeatureReport(request->wValue, rhport, request);
    }
  } else {
    return false;
  }
  return true;
}
uint8_t const *tud_descriptor_device_cb(void) {
  if (fullDeviceType >= SWITCH_GAMEPAD && fullDeviceType < MOUSE) {
    uint8_t offs = fullDeviceType - SWITCH_GAMEPAD;
    deviceDescriptor.VendorID = vid[offs];
    deviceDescriptor.ProductID = pid[offs];
  }
  return (uint8_t const *)&deviceDescriptor;
}
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  if (fullDeviceType <= KEYBOARD_ROCK_BAND_GUITAR) {
    return kbd_report_descriptor;
  } else {
    return ps3_report_descriptor;
  }
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // for multiple configurations
  ConfigurationDescriptor.XInputReserved.subtype = deviceType;
  if (fullDeviceType <= KEYBOARD_ROCK_BAND_GUITAR) {
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
    void *osDesc = &OSDescriptorString;
    return (uint16_t *)osDesc;
  }
  return NULL;
}
Controller_t controller;
USB_Report_Data_t previousReport;
USB_Report_Data_t currentReport;
uint8_t size;
void hid_task(void) {
  static uint32_t start_ms = 0;
  if (isRF) {
    tickRFInput((uint8_t *)&controller, sizeof(XInput_Data_t));
  } else {
    if (!tickInputs(&controller)) {
      return;
    }
    tickLEDs(&controller);
  }
  fillReport(&currentReport, &size, &controller);
  if (memcmp(&currentReport, &previousReport, size) != 0) {
    uint8_t *data = (uint8_t *)&currentReport;
    uint8_t rid = *data;
    switch (rid) {
    case REPORT_ID_XINPUT:
      if (tud_xinput_n_ready(0)) {
        tud_xinput_n_report(0, 0, data, size);
        start_ms = millis();
      }
      break;
    case REPORT_ID_GAMEPAD:
      rid = 0;
    case REPORT_ID_KBD:
    case REPORT_ID_MOUSE:
      data++;
      size--;
      if (tud_hid_n_ready(0)) {
        tud_hid_n_report(0, rid, data, size);
        start_ms = millis();
      }
      break;
    case REPORT_ID_MIDI:
      data++;
      size--;
      tud_midi_n_packet_write(0, data);
      start_ms = millis();
    }

    // Remote wakeup
    if (tud_suspended()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
  }
}
void initialise(void) {
  board_init();
  tusb_init();
  #ifndef NDEBUG
  uart_set_baudrate(uart0, 115200);
  #endif
  Configuration_t config;
  loadConfig(&config);
  fullDeviceType = config.main.subType;
  deviceType = fullDeviceType;
  pollRate = config.main.pollRate;
  inputType = config.main.inputType;
  typeIsDrum = isDrum(fullDeviceType);
  typeIsGuitar = isGuitar(fullDeviceType);
  typeIsDJ = isDJ(fullDeviceType);
  if (typeIsGuitar && deviceType <= XINPUT_TURNTABLE) {
    deviceType = REAL_GUITAR_SUBTYPE;
  }
  if (typeIsDrum && deviceType <= XINPUT_TURNTABLE) {
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
}
int main() {
  initialise();
  while (1) {
    tud_task(); // tinyusb device task
    hid_task();
  }
}
void writeToUSB(const void *const Buffer, uint8_t Length, uint8_t report,
                const void *request) {
  tud_control_xfer(report, request, (uint8_t *)Buffer + 1, Length - 1);
}
void stopReading(void) {}

usbd_class_driver_t driver[] = {{.init = xinputd_init,
                                 .reset = xinputd_reset,
                                 .open = xinputd_open,
                                 .control_xfer_cb = tud_vendor_control_xfer_cb,
                                 .xfer_cb = xinputd_xfer_cb,
                                 .sof = NULL}};
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;
  return driver;
}

static uint8_t id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  if (report_type == HID_REPORT_TYPE_FEATURE) {
    //  When requested, return the ps3 report ids so that we have console
    //  compatibility
    if (fullDeviceType <= PS3_ROCK_BAND_DRUMS) {
      id[3] = 0x00;
    } else if (fullDeviceType <= PS3_GUITAR_HERO_DRUMS) {
      id[3] = 0x06;
    }
    buffer = id;
    return sizeof(id);
  }
  return 0;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {}