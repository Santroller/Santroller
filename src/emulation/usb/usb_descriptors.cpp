#include "tusb_option.h"
#include "emulation/usb/usb_descriptors.h"
#include <memory>
#include "managers/profile_manager.hpp"

#include "tusb.h"
#include "utils.h"
#include "emulation/usb/gh_arcade_device.h"
#include "emulation/usb/pdloader_device.h"
#include "emulation/usb/xinput_device.h"
#include "emulation/usb/xone_device.h"
#include "emulation/usb/ogxbox_device.h"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/ps3_device.h"
#include "emulation/usb/ps4_device.h"
#include "device/dcd.h"
#include "hardware/structs/usb.h"
#include "hid_reports.h"
#include <pico/unique_id.h>
#include "enums.pb.h"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include "main.hpp"
#include "mappings/base_mapping.hpp"
#include "devices/bt/bt_host.hpp"

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
  UsbDetectionState::instance().mark_device_descriptor_read(millis());
  memcpy(descriptor_buffer, &desc_device_init, sizeof(desc_device_init));
  ProfileManager::instance().for_each_usb_instance([](const auto &instance)
                                                   { instance->device_descriptor((tusb_desc_device_t *)descriptor_buffer); });
  return descriptor_buffer;
}

uint8_t const *tud_descriptor_bos_cb(void)
{
  if (ConfigManager::instance().get_requested_mode() == ModePdLoader)
  {
    return PDLoaderDevice::bos_descriptor();
  }
  return nullptr;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t interface)
{
  return std::static_pointer_cast<HIDDevice>(ProfileManager::instance().get_usb_instance(interface))->report_descriptor();
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index; // for multiple configurations
  tusb_desc_configuration_t *config = (tusb_desc_configuration_t *)descriptor_buffer;
  size_t current = sizeof(tusb_desc_configuration_t);
  size_t interfaces = 0;
  ProfileManager::instance().for_each_usb_instance([&current, &interfaces](const auto &instance)
                                                   {
    current += instance->config_descriptor(descriptor_buffer + current, sizeof(descriptor_buffer) - current);
    interfaces += instance->interface_count(); });
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

// Room for the board id, and a layout suffix ("-" and 8 hex digits)
static char serial[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 10];

// Windows keys devices on the serial, both for Steam's cache (which kept showing the previous set of gamepads
// after a reload added or removed one) and for each interface's driver binding (so switching a gamepad between
// XInput and HID kept the old driver on it). A layout gets its own serial instead. A single HID gamepad keeps
// the plain board id, as that's what every board enumerates as before it knows the host is windows.
static void update_serial()
{
  // Only fits 15 of the id's 16 digits, kept that way so existing serials don't change
  pico_get_unique_board_id_string(serial, PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2);
  const uint32_t hash = ProfileManager::instance().usb_layout_hash();
  if (!hash)
  {
    return;
  }
  const size_t len = strlen(serial);
  snprintf(serial + len, sizeof(serial) - len, "-%08lx", static_cast<unsigned long>(hash));
}
// array of pointer to string descriptors
char const *string_desc_arr[STRID_COUNT] =
    {
        (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
        "sanjay900",                // 1: Manufacturer, unless configured
        "Santroller",               // 2: Product, unless the device is named
        serial,                     // 3: Serials will use unique ID if possible
        "MSFT100\x20"};

static uint16_t _desc_str[100 + 1];
static char _desc_str_tmp[100 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;
  // printf("desc %02x %02x\r\n", index, langid);
  auto &detection = UsbDetectionState::instance();
  if (detection.seen_windows_xb1())
  {
    detection.mark_windows_string_seen();
  }
  size_t chr_count;
  const char *str = nullptr;
  // We only care about actual reads for this heuristic
  if (index != STRID_LANGID)
  {
    detection.mark_device_string_read();
  }
  switch (index)
  {
  case STRID_LANGID:
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
    break;

  case STRID_SERIAL:
    update_serial();

  default:
    if (index == 0xEE)
    {
      detection.mark_os_descriptor_read();
      index = STRID_MSFT;
    }

    // The real KeyboardMania has its name as the manufacturer and an empty product
    if (ConfigManager::instance().get_requested_mode() == ModeKeyboardMania &&
        (index == STRID_MANUFACTURER || index == STRID_PRODUCT))
    {
      str = index == STRID_MANUFACTURER ? "USB Multipurpose Controller" : "";
    }
    else if (index == STRID_MANUFACTURER && ConfigManager::instance().get_manufacturer_name())
    {
      str = ConfigManager::instance().get_manufacturer_name();
    }
    else if (index == STRID_PRODUCT && ConfigManager::instance().get_device_name())
    {
      str = ConfigManager::instance().get_device_name();
    }
    else if ((index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
    {
      str = string_desc_arr[index];
    }
    else
    {
      ProfileManager::instance().for_each_usb_instance([index, &str](const auto &instance)
                                                       {
        if (!str && instance->device_name(index, _desc_str_tmp))
        {
          str = _desc_str_tmp;
        } });
    }
    if (!str)
    {
      return 0;
    }
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
  std::shared_ptr<UsbDevice> dev;
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_OUT)
  {
    dev = ProfileManager::instance().get_usb_instance_by_epout(ep_addr);
  }
  else
  {
    dev = ProfileManager::instance().get_usb_instance_by_epin(ep_addr & (~0x80));
  }
  if (!dev)
  {
    return false;
  }
  return dev->interrupt_xfer(ep_addr, result, xferred_bytes);
}

uint16_t tud_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                  uint16_t max_len)
{
  return ProfileManager::instance().get_usb_instance(itf_desc->bInterfaceNumber)->open(itf_desc, max_len);
}

// The pad the last GET_REPORT went to, which vibration caps reads at wIndex 0 follow
static uint8_t s_last_xinput_itf = 0;

void tud_init(void)
{
  UsbDevice::mounted_since_init = false;
  tud_reset(TUD_OPT_RHPORT);
}
void tud_reset(uint8_t rhport)
{
  // printf("reset\r\n");
  UsbDevice::reset_ep();
  XInputGamepadDevice::reset();
  s_last_xinput_itf = 0;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
  if (ConfigManager::instance().get_requested_mode() == ModePdLoader &&
      request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
      request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
      request->bmRequestType_bit.direction == TUSB_DIR_IN &&
      request->bRequest == 1 && request->wIndex == 7)
  {
    if (stage == CONTROL_STAGE_SETUP)
    {
      return tud_control_xfer(rhport, request,
                              const_cast<uint8_t *>(PDLoaderDevice::ms_os_20_descriptor()),
                              PDLoaderDevice::ms_os_20_descriptor_length());
    }
    return true;
  }
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
            ProfileManager::instance().for_each_usb_instance([&current, &count](const auto &instance)
                                                             {
              size_t size = instance->compatible_section_descriptor(descriptor_buffer + current, sizeof(descriptor_buffer) - current);
              current += size;
              if (size)
              {
                count++;
              } });
            UsbDetectionState::instance().mark_windows_xb1_seen();
            compatible_descriptor->TotalSections = count;
            compatible_descriptor->TotalLength = current;
            tud_control_xfer(rhport, request, descriptor_buffer, current);
          }
          return true;
        }
        if (request->bRequest == HID_REQ_CONTROL_GET_REPORT && request->wIndex == 0x0000 && request->wValue == 0x0001 &&
            ProfileManager::instance().xinput_receiver())
        {
          // receiver id, read when xusb22 starts a receiver
          if (stage == CONTROL_STAGE_SETUP)
          {
            static uint8_t id[7];
            XInputGamepadDevice::receiver_id(id);
            tud_control_xfer(rhport, request, id, sizeof(id));
          }
          return true;
        }
        if (request->bRequest == HID_REQ_CONTROL_GET_REPORT && request->wIndex == 0x0000 && request->wValue == SERIAL_NUMBER_WVALUE)
        {
          // xinput serial read, once per pad, so hand them out in turn like the caps reads
          if (stage == CONTROL_STAGE_SETUP)
          {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if (now - XInputGamepadDevice::last_serial_query_time > 500 ||
                XInputGamepadDevice::lastIntfSerial >= TU_ARRAY_SIZE(XInputGamepadDevice::xinputInterfaces) ||
                XInputGamepadDevice::xinputInterfaces[XInputGamepadDevice::lastIntfSerial] == 0xFF)
            {
              XInputGamepadDevice::lastIntfSerial = 0;
            }
            XInputGamepadDevice::last_serial_query_time = now;
            uint8_t itf = XInputGamepadDevice::xinputInterfaces[XInputGamepadDevice::lastIntfSerial];
            uint32_t serial = XInputGamepadDevice::controller_serial(itf);

            printf("controller_serial: itf=%u serial=%08x\n", itf, serial);
            tud_control_xfer(rhport, request, &serial, sizeof(serial));
          }
          // served, on to the next pad for the next read
          else if (stage == CONTROL_STAGE_ACK)
          {
            XInputGamepadDevice::lastIntfSerial++;
          }
          return true;
        }
      }
    }
  }
  if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
  {
    auto wIndex = request->wIndex & 0xFF;
    // Only redirect when there are XInput interfaces to redirect to. XID uses the same request for GET_REPORT
    // and GET_CAPABILITIES, and redirecting those to nothing stalls them, so older OG Xbox games (eg JSRF)
    // never start reading the controller.
    bool has_xinput = XInputGamepadDevice::xinputInterfaces[0] != 0xFF;
    // On an actual 360, the console always uses wIndex 0x00. This is the security interface so redirect the request to the right place
    if (has_xinput && wIndex == 0x00 && request->wValue == INPUT_CAPABILITIES_WVALUE && request->bRequest == HID_REQ_CONTROL_GET_REPORT)
    {
      if (XInputGamepadDevice::xinputInterfaces[1] == 0xFF)
      {
        wIndex = XInputGamepadDevice::xinputInterfaces[0];
      }
      else
      {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - XInputGamepadDevice::last_caps_query_time > 500)
        {
          XInputGamepadDevice::lastIntfInput = 0;
        }
        XInputGamepadDevice::last_caps_query_time = now;

        if (XInputGamepadDevice::lastIntfInput >= TU_ARRAY_SIZE(XInputGamepadDevice::xinputInterfaces) ||
            XInputGamepadDevice::xinputInterfaces[XInputGamepadDevice::lastIntfInput] == 0xFF)
        {
          XInputGamepadDevice::lastIntfInput = 0;
        }

        wIndex = XInputGamepadDevice::xinputInterfaces[XInputGamepadDevice::lastIntfInput];
        // caps read, on to the next set for the next read
        if (stage == CONTROL_STAGE_ACK)
        {
          XInputGamepadDevice::lastIntfInput++;
          if (XInputGamepadDevice::lastIntfInput >= TU_ARRAY_SIZE(XInputGamepadDevice::xinputInterfaces) ||
              XInputGamepadDevice::xinputInterfaces[XInputGamepadDevice::lastIntfInput] == 0xFF)
          {
            XInputGamepadDevice::lastIntfInput = 0;
          }
        }
      }

      if (wIndex == 0xFF)
      {
        wIndex = XInputGamepadDevice::xinputInterfaces[0];
      }
    }
    if (has_xinput && wIndex == 0x00 && request->wValue == VIBRATION_CAPABILITIES_WVALUE && request->bRequest == HID_REQ_CONTROL_GET_REPORT)
    {
      // vibration caps follow a pad's caps read, so send them to that pad. Interface 0 is never xinput (it is
      // the security interface), so 0 here means no caps read has happened yet
      wIndex = s_last_xinput_itf ? s_last_xinput_itf : XInputGamepadDevice::xinputInterfaces[0];
    }
    if (request->bRequest == HID_REQ_CONTROL_GET_REPORT)
    {
      for (const auto itf : XInputGamepadDevice::xinputInterfaces)
      {
        if (itf != 0xFF && itf == wIndex)
        {
          s_last_xinput_itf = wIndex;
        }
      }
    }
    auto dev = ProfileManager::instance().get_usb_instance(wIndex);
    if (!dev)
    {
      return false;
    }
    // printf("xfer\r\n");
    return dev->control_transfer(stage, request);
  }
  else if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT)
  {
    uint8_t ep_addr = request->wIndex & 0xFF;
    std::shared_ptr<UsbDevice> dev;
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_OUT)
    {
      dev = ProfileManager::instance().get_usb_instance_by_epout(ep_addr);
    }
    else
    {
      dev = ProfileManager::instance().get_usb_instance_by_epin(ep_addr & (~0x80));
    }
    if (!dev)
    {
      return false;
    }
    // printf("xfer\r\n");
    return dev->control_transfer(stage, request);
  }
  return false;
}

volatile uint32_t UsbDevice::suspend_generation = 0;

void UsbDevice::disconnect_bluetooth_controllers()
{
  auto disconnect = [](const std::shared_ptr<Device> &device)
  {
    if (device && device->is_bluetooth_host_interface())
    {
      std::static_pointer_cast<BluetoothHostInterface>(device)->disconnect();
    }
  };
  for (const auto &profile : profiles)
  {
    if (!profile->disconnect_bluetooth_on_suspend)
    {
      continue;
    }
    for (const auto &device : profile->devices)
    {
      disconnect(device.second);
    }
    for (const auto &device : profile->claimed_devices)
    {
      disconnect(device.second);
    }
  }
}

void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  UsbDevice::suspend_generation++;
}

bool UsbDevice::mounted_since_init = false;
bool UsbDevice::s_host_suspended = false;
bool UsbDevice::s_bus_idle = false;
static uint16_t s_last_sof = 0;
static uint32_t s_last_sof_at = 0;
// A full speed host sends a frame every ms, so this many without one means the bus has gone idle
static constexpr uint32_t BUS_IDLE_MS = 10;

bool UsbDevice::update_host_suspended()
{
  if (tud_mounted())
  {
    mounted_since_init = true;
  }
  // SIE_STATUS's suspended bit can't be read for this, as tinyusb's interrupt handler clears it straight away
  uint16_t sof = usb_hw->sof_rd & USB_SOF_RD_BITS;
  uint32_t now = millis();
  if (sof != s_last_sof)
  {
    s_last_sof = sof;
    s_last_sof_at = now;
  }
  s_bus_idle = now - s_last_sof_at > BUS_IDLE_MS;
  // A 360 going into standby resets the bus before idling it, which leaves tinyusb unconnected and
  // never reporting the suspend, so look at the bus itself. The bus also idles between attaching
  // and the host's first reset (eg after switching into 360 mode on Windows), so only trust this
  // once the host has configured us.
  bool bus_suspended = ConfigManager::instance().get_current_mode() == ModeXbox360 && mounted_since_init && s_bus_idle;
  // tud_suspend_cb never runs for this, so count it as a new suspend here
  if (bus_suspended && !tud_suspended() && !s_host_suspended)
  {
    suspend_generation++;
  }
  s_host_suspended = tud_suspended() || bus_suspended;
  return s_host_suspended;
}

void UsbDevice::process_suspended(bool full_poll, bool send_events)
{
  if (m_suspend_generation != suspend_generation)
  {
    m_suspend_generation = suspend_generation;
    disconnect_bluetooth_controllers();
  }
  for (const auto &profile : profiles)
  {
    for (const auto &led : profile->leds)
    {
      led->off();
    }
    for (const auto &mapping : profile->mappings)
    {
      mapping->update(full_poll, send_events);
    }
  }
}

bool UsbDevice::s_wake_armed = false;
bool UsbDevice::s_standby_wake_armed = false;
uint32_t UsbDevice::s_wake_generation = 0;

void UsbDevice::wake_console_in_standby(bool in_standby, bool pressed)
{
  // Disarmed until it's in standby with nothing held, so a button held as it turns off doesn't turn it back on
  if (!in_standby || !pressed)
  {
    s_standby_wake_armed = in_standby;
    return;
  }
  if (!s_standby_wake_armed)
  {
    return;
  }
  s_standby_wake_armed = false;
  // It's still polling, so there's nothing to resume
  XInputGamepadDevice::hold_guide_for_wake();
}

void UsbDevice::wake_host(bool pressed)
{
  if (s_wake_generation != suspend_generation)
  {
    s_wake_generation = suspend_generation;
    s_wake_armed = false;
  }
  if (!pressed)
  {
    s_wake_armed = true;
    return;
  }
  if (!s_wake_armed)
  {
    return;
  }
  s_wake_armed = false;
  if (ConfigManager::instance().get_current_mode() == ModeXbox360)
  {
    // The 360 suspends without ever sending SET_FEATURE(DEVICE_REMOTE_WAKEUP), but still
    // powers on when a wired controller signals resume, so ignore the host's setting.
    dcd_remote_wakeup(TUD_OPT_RHPORT);
    XInputGamepadDevice::hold_guide_for_wake();
    return;
  }
  // fails harmlessly if the host didn't enable remote wakeup before suspending
  tud_remote_wakeup();
}

uint8_t UsbDevice::m_last_epin = 0x81;
uint8_t UsbDevice::m_last_epout = 0x01;
uint8_t UsbDevice::m_last_strid = 0x00;