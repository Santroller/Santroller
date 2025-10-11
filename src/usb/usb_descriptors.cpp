/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "hid_reports.h"
#include <pico/unique_id.h>
#include "enums.pb.h"
#include "config.hpp"
#include "main.hpp"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t desc_device =
    {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = USB_VERSION_BCD(2, 0, 0),
        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor = USB_VID,
        .idProduct = USB_PID,
        .bcdDevice = 0x0100,

        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,

        .bNumConfigurations = 0x01};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void)
{
  desc_device.bcdDevice = 0x0400 | current_type;
  desc_device.bDeviceClass = 0;
  desc_device.bDeviceSubClass = 0;
  desc_device.bDeviceProtocol = 0;
  switch (mode)
  {
  case ConsoleMode::Switch:
    desc_device.idVendor = 0x0f0d;
    desc_device.idProduct = 0x0092;
    break;
  case ConsoleMode::Ps3:
    desc_device.idVendor = 0x054c;
    desc_device.idProduct = 0x0268;
    break;
  case ConsoleMode::XboxOne:
    desc_device.idVendor = 0x045e;
    desc_device.idProduct = 0x02ea;
    desc_device.bDeviceClass = 0xff;
    desc_device.bDeviceSubClass = 0x47;
    desc_device.bDeviceProtocol = 0xd0;
    break;
  case ConsoleMode::WiiRb:
    desc_device.idVendor = 0x1bad;
    desc_device.idProduct = 0x3010;
    break;
  default:
    desc_device.idVendor = USB_VID;
    desc_device.idProduct = USB_PID;
    break;
  }
  return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] =
    {
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdLoaded)),
        TUD_HID_REPORT_DESC_GAME_CONTROLLER(HID_REPORT_ID(ReportIdGamepad))};

uint8_t const desc_hid_report_non_gamepad[] =
    {
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdLoaded))};
uint8_t const desc_hid_report_ps3[] =
    {
        TUD_HID_REPORT_DESC_PS3_THIRDPARTY_GAMEPAD()};
uint8_t const desc_hid_report_ps3_gamepad[] =
    {
        TUD_HID_REPORT_DESC_PS3_FIRSTPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};
uint8_t const desc_hid_report_ps4[] =
    {
        TUD_HID_REPORT_DESC_PS4_FIRSTPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
  (void)instance;
  switch (mode)
  {
  case ConsoleMode::Hid:
    return desc_hid_report;
  case ConsoleMode::Ps4:
    return desc_hid_report_ps4;
  case ConsoleMode::Ps3:
    if (current_type == Gamepad)
    {
      return desc_hid_report_ps3_gamepad;
    }
    return desc_hid_report_ps3;
  case ConsoleMode::OgXbox:
  case ConsoleMode::Xbox360:
  case ConsoleMode::XboxOne:
    return desc_hid_report_non_gamepad;
  default:
    return desc_hid_report_ps3;
  }
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum
{
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
  STRID_XSM3,
  STRID_MSFT
};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

// #define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_XONE_GAMEPAD_DESC_LEN + TUD_XINPUT_SECURITY_DESC_LEN)

#define CONFIG_TOTAL_LEN_XINPUT (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_XINPUT_GAMEPAD_DESC_LEN + TUD_XINPUT_SECURITY_DESC_LEN)
#define CONFIG_TOTAL_LEN_OGXBOX (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_OGXBOX_GAMEPAD_DESC_LEN + TUD_XINPUT_SECURITY_DESC_LEN)
#define CONFIG_TOTAL_LEN_XBOXONE (TUD_CONFIG_DESC_LEN + TUD_XONE_GAMEPAD_DESC_LEN)
#define CONFIG_TOTAL_LEN_HID (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN + TUD_XINPUT_SECURITY_DESC_LEN + TUD_XINPUT_PLUGIN_MODULE_DESC_LEN)
#define CONFIG_TOTAL_LEN_PS3 (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)
#define EPNUM_HID_IN 0x81
#define EPNUM_HID_OUT 0x02
#define EPNUM_XINPUT_IN 0x83
#define EPNUM_XINPUT_OUT 0x04
#define EPNUM_XINPUT_PMD_IN 0x85
uint8_t const desc_configuration_ogxbox[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_OGXBOX, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_non_gamepad), EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 1),
        TUD_OGXBOX_GAMEPAD_DESCRIPTOR(ITF_NUM_XINPUT, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT),
        TUD_XINPUT_SECURITY_DESCRIPTOR(ITF_NUM_XINPUT_SECURITY, STRID_XSM3)};

uint8_t const desc_configuration_xone[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_XBOXONE, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        TUD_XONE_GAMEPAD_DESCRIPTOR(ITF_NUM_XINPUT, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT)};

uint8_t const desc_configuration_xinput[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_XINPUT, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_non_gamepad), EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 1),

        TUD_XINPUT_GAMEPAD_DESCRIPTOR(ITF_NUM_XINPUT, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT, 0x07),
        // TUD_OGXBOX_GAMEPAD_DESCRIPTOR(ITF_NUM_OGXBOX, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT),
        // TUD_XONE_GAMEPAD_DESCRIPTOR(ITF_NUM_XONE, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT),
        TUD_XINPUT_SECURITY_DESCRIPTOR(ITF_NUM_XINPUT_SECURITY, STRID_XSM3)};
uint8_t const desc_configuration_hid[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_HID, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_OUT, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 1),
        TUD_XINPUT_PLUGIN_MODULE_DESCRIPTOR(ITF_NUM_XINPUT, EPNUM_XINPUT_PMD_IN),
        // TUD_XINPUT_GAMEPAD_DESCRIPTOR(ITF_NUM_XINPUT, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT, 0x07),
        // TUD_OGXBOX_GAMEPAD_DESCRIPTOR(ITF_NUM_OGXBOX, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT),
        // TUD_XONE_GAMEPAD_DESCRIPTOR(ITF_NUM_XONE, EPNUM_XINPUT_IN, EPNUM_XINPUT_OUT),
        TUD_XINPUT_SECURITY_DESCRIPTOR(ITF_NUM_XINPUT_SECURITY, STRID_XSM3)};
uint8_t const desc_configuration_ps3[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN_PS3, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps3), EPNUM_HID_OUT, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 1)};
uint8_t const desc_configuration_ps3_gamepad[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN_PS3, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps3_gamepad), EPNUM_HID_OUT, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 1)};
uint8_t const desc_configuration_ps4[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN_PS3, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
        // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
        TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps4), EPNUM_HID_OUT, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 1)};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index; // for multiple configurations
  printf("Current mode: %d %d\r\n", mode, current_type);
  // This example use the same configuration for both high and full speed mode
  switch (mode)
  {
  case ConsoleMode::Hid:
    return desc_configuration_hid;
  case ConsoleMode::Ps4:
    return desc_configuration_ps4;
  case ConsoleMode::Ps3:
    if (current_type == Gamepad)
    {
      return desc_configuration_ps3_gamepad;
    }
    return desc_configuration_ps3;
  case ConsoleMode::Xbox360:
    return desc_configuration_xinput;
  case ConsoleMode::XboxOne:
    return desc_configuration_xone;
  case ConsoleMode::OgXbox:
    return desc_configuration_ogxbox;
  default:
    return desc_configuration_ps3;
  }
}

static char serial[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2];
// array of pointer to string descriptors
char const *string_desc_arr[] =
    {
        (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
        "sanjay900",                // 1: Manufacturer
        "Santroller",               // 2: Product
        serial,                     // 3: Serials will use unique ID if possible
        "Xbox Security Method 3, Version 1.00, \xa9 2005 Microsoft Corporation. All rights reserved.",
        "MSFT100\x20"};

static uint16_t _desc_str[100 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;
  size_t chr_count;
  // We only care about actual reads for this heuristic
  if (index != STRID_LANGID)
  {
    seenReadAnyDeviceString = true;
  }
  switch (index)
  {
  case STRID_LANGID:
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
    break;

  case STRID_SERIAL:
    pico_get_unique_board_id_string(serial, sizeof(serial));

  default:
    if (index == 0xEE)
    {
      seenOsDescriptorRead = true;
      index = STRID_MSFT;
    }

    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
      return NULL;

    const char *str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
    if (chr_count > max_count)
      chr_count = max_count;

    // Convert ASCII string into UTF-16
    for (size_t i = 0; i < chr_count; i++)
    {
      _desc_str[1 + i] = str[i];
    }
    break;
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

  return _desc_str;
}