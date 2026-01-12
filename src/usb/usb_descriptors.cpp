#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "usb/device/xinput_device.h"
#include "usb/device/hid_device.h"
#include "hid_reports.h"
#include <pico/unique_id.h>
#include "enums.pb.h"
#include "config.hpp"
#include "main.hpp"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t desc_device_init =
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

uint8_t descriptor_buffer[0x7ff];
// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void)
{
  memcpy(descriptor_buffer, &desc_device_init, sizeof(desc_device_init));
  for (auto& instance : usb_instances) {
    instance.second->device_descriptor((tusb_desc_device_t*)descriptor_buffer);
  }
  return descriptor_buffer;
}

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t interface)
{
  return std::static_pointer_cast<HIDDevice>(usb_instances[interface])->report_descriptor();
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index; // for multiple configurations
  printf("Current mode: %d %d\r\n", mode, current_type);
  tusb_desc_configuration_t* config = (tusb_desc_configuration_t*)descriptor_buffer;
  size_t current = sizeof(tusb_desc_configuration_t);
  size_t interfaces = 0;
  for (auto& instance : usb_instances) {
    current += instance.second->config_descriptor(descriptor_buffer + current, sizeof(descriptor_buffer) - current);
    interfaces++;
  }
  config->bDescriptorType = TUSB_DESC_CONFIGURATION;
  config->bLength = sizeof(tusb_desc_configuration_t);
  config->wTotalLength = current;
  config->bNumInterfaces = interfaces;
  config->bConfigurationValue = 1;
  config->iConfiguration = 0;
  config->bmAttributes = TU_BIT(7) | TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP;
  config->bMaxPower = 50;
  return descriptor_buffer;
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
        "MSFT100\x20",
        "RT GH CONTROLLER ",
        "RT-GH LED ",
        "RT-GH INPUT "};

static uint16_t _desc_str[100 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;
  printf("desc %02x %02x\r\n", index, langid);
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