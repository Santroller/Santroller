#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "usb/device/gh_arcade_device.h"
#include "usb/device/xinput_device.h"
#include "usb/device/xone_device.h"
#include "usb/device/ogxbox_device.h"
#include "usb/device/hid_device.h"
#include "usb/device/ps3_device.h"
#include "usb/device/ps4_device.h"
#include "hid_reports.h"
#include <pico/unique_id.h>
#include "enums.pb.h"
#include "config.hpp"
#include "main.hpp"

bool usb_device_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                        uint32_t xferred_bytes);

uint16_t tud_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                  uint16_t max_len);
void tud_init(void);
void tud_reset(uint8_t rhport);
usbd_class_driver_t drivers[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "Santroller_Device",
#endif
        .init = tud_init,
        .reset = tud_reset,
        .open = tud_open,
        .control_xfer_cb = tud_vendor_control_xfer_cb,
        .xfer_cb = usb_device_xfer_cb,
        .sof = NULL}};

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

uint8_t const *tud_descriptor_device_cb(void)
{
  memcpy(descriptor_buffer, &desc_device_init, sizeof(desc_device_init));
  for (auto &instance : usb_instances)
  {
    instance.second->device_descriptor((tusb_desc_device_t *)descriptor_buffer);
  }
  return descriptor_buffer;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t interface)
{
  return std::static_pointer_cast<HIDDevice>(usb_instances[interface])->report_descriptor();
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index; // for multiple configurations
  printf("Current mode: %d %d\r\n", mode, current_type);
  tusb_desc_configuration_t *config = (tusb_desc_configuration_t *)descriptor_buffer;
  size_t current = sizeof(tusb_desc_configuration_t);
  size_t interfaces = 0;
  for (auto &instance : usb_instances)
  {
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
// TODO: need to implement the fact that for powergig we need to set the name to `Seven45 Drum Controller` or `Seven45 Guitar Controller`

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

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
  *driver_count = TU_ARRAY_SIZE(drivers);
  return drivers;
}

const OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR ExtendedIDs = {
  TotalLength : sizeof(OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR),
  Version : 0x0100,
  Index : DESC_EXTENDED_PROPERTIES_DESCRIPTOR,
  TotalSections : 1,
  SectionSize : 132,
  ExtendedID : {
    PropertyDataType : 1,
    PropertyNameLength : 40,
    PropertyName : {'D', 'e', 'v', 'i', 'c', 'e', 'I', 'n', 't', 'e',
                    'r', 'f', 'a', 'c', 'e', 'G', 'U', 'I', 'D', '\0'},
    PropertyDataLength : 78,
    PropertyData :
        {'{', 'D', 'F', '5', '9', '0', '3', '7', 'D', '-', '7', 'C', '9',
         '2', '-', '4', '1', '5', '5', '-', 'A', 'C', '1', '2', '-', '7',
         'D', '7', '0', '0', 'A', '3', '1', '3', 'D', '7', '9', '}', '\0'}
  }
};

const OS_COMPATIBLE_ID_DESCRIPTOR DevCompatIDHeader = {
  TotalLength : 0,
  Version : 0x0100,
  Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
  TotalSections : 1,
  Reserved : {0},
  CompatID : {}
};

bool usb_device_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                        uint32_t xferred_bytes)
{

  auto it = usb_instances_by_epnum.find(ep_addr);
  if (it == usb_instances_by_epnum.end())
  {
    return false;
  }
  auto dev = it->second;
  return dev->interrupt_xfer(ep_addr, result, xferred_bytes);
}

uint16_t tud_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                  uint16_t max_len)
{
  return usb_instances[itf_desc->bInterfaceNumber]->open(itf_desc, max_len);
}

void tud_init(void)
{
  tud_reset(TUD_OPT_RHPORT);
}
void tud_reset(uint8_t rhport)
{
  UsbDevice::reset_ep();
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
  printf("control req %02x %02x %02x %02x %04x %04x\r\n", request->bmRequestType_bit.direction, request->bmRequestType_bit.type, request->bmRequestType_bit.recipient, request->bRequest, request->wIndex & 0xFF, request->wValue);
  if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
  {

    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
    {
      if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
      {
        if (request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && request->wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR)
        {
          if (stage == CONTROL_STAGE_SETUP)
          {
            tud_control_xfer(rhport, request, (void *)&ExtendedIDs, ExtendedIDs.TotalLength);
          }
          return true;
        }
      }
      else if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE)
      {
        if (request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && request->wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR)
        {
          if (stage == CONTROL_STAGE_SETUP)
          {
            OS_COMPATIBLE_ID_DESCRIPTOR *compatible_descriptor = (OS_COMPATIBLE_ID_DESCRIPTOR *)descriptor_buffer;
            memcpy(descriptor_buffer, &DevCompatIDHeader, sizeof(DevCompatIDHeader));
            size_t current = sizeof(DevCompatIDHeader);
            size_t count = 0;
            for (auto &instance : usb_instances)
            {
              size_t size = instance.second->compatible_section_descriptor(descriptor_buffer + current, sizeof(descriptor_buffer) - current);
              current += size;
              if (size)
              {
                count++;
              }
            }
            compatible_descriptor->TotalSections = count;
            compatible_descriptor->TotalLength = current;
            tud_control_xfer(rhport, request, descriptor_buffer, current);
          }
          return true;
        }
        if (request->bRequest == HID_REQ_CONTROL_GET_REPORT && request->wIndex == 0x0000 && request->wValue == SERIAL_NUMBER_WVALUE)
        {
          // xinput serial read
          if (stage == CONTROL_STAGE_SETUP)
          {
            uint32_t serial = to_us_since_boot(get_absolute_time());
            tud_control_xfer(rhport, request, &serial, sizeof(serial));
          }
          return true;
        }
      }
    }
  }
  if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
  {
    auto it = usb_instances.find(request->wIndex & 0xFF);
    if (it == usb_instances.end())
    {
      return false;
    }
    auto dev = it->second;
    return dev->control_transfer(stage, request);
  }
  else if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT)
  {
    auto it = usb_instances_by_epnum.find(request->wIndex & 0xFF);
    if (it == usb_instances_by_epnum.end())
    {
      return false;
    }
    auto dev = it->second;
    return dev->control_transfer(stage, request);
  }
  return false;
}

uint8_t UsbDevice::m_last_epin = 0x81;
uint8_t UsbDevice::m_last_epout = 0x01;