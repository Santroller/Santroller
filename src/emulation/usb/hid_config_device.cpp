#include "tusb_option.h"
#include "managers/config_manager.hpp"
#include "managers/device_manager.hpp"
#include "config/config.hpp"
#include "config/FlashPROM.h"
#include "managers/profile_manager.hpp"
#include "leds/led_mappings.hpp"

#include "commands.pb.h"
#include "enums.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "emulation/bt/bt_config_service.h"
#include "devices/bt/bt_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "emulation/usb/ps3_device.h"
#include "emulation/usb/ps4_device.h"
#include "devices/crkd_drum.hpp"
#include "device/usbd.h"
#include "hid_reports.h"
#include "device/usbd_pvt.h"
#include "pico/bootrom.h"
#include "utils.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "devices/secondary_pico.hpp"
#include "secondary_pico.hpp"
#include "config/input_detector.hpp"
#include "math.h"
#include <pico_fota_bootloader/core.h>
#include <algorithm>
#include <string.h>
#include <vector>

static const char version[] = GIT_HASH;
// 0 when it was built without the git history
static constexpr uint32_t build_number = GIT_BUILD;
static const char type[] = PICO_BOARD;
static constexpr uint16_t firmware_upload_report_size = 63;
static constexpr uint32_t firmware_write_alignment = 256;
static constexpr uint32_t firmware_write_block_size = 4096;
// Config reports are 63 bytes, but a transport can ask for less (bluetooth HID leaves room for padding).
// Every config report is read with the same reqlen, so ConfigInfo can tell the tool the chunk size too.
static uint32_t config_chunk_size(uint16_t reqlen)
{
  return reqlen > 1 ? std::min<uint32_t>(reqlen - 1, 63) : 0;
}
static bool decode_firmware_update(const uint8_t *buffer, uint16_t bufsize, proto_FirmwareUpdate *update);
// Cycle changes from the host. They arrive in the USB / bluetooth callbacks, but stepping sends an event
// (which runs tud_task) and saves the aux config, so they wait for process() on the main loop. Shared, as
// either transport's config device may receive them
static std::vector<proto_Command> pending_cycle_commands;
uint8_t const desc_hid_report_config[] =
    {

        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),
        HID_USAGE(0x01),
        HID_COLLECTION(HID_COLLECTION_APPLICATION),
        TUD_HID_REPORT_DESC_GENERIC_INFEATURE(63, HID_REPORT_ID(ReportIdConfig)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdConfigInfo)),
        // the saved (rather than running) config info, answered by the same copy_config_info
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdConfigInfoSaved)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(1, HID_REPORT_ID(ReportIdLoaded)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdCommand)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(1, HID_REPORT_ID(ReportIdKeepalive)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(1, HID_REPORT_ID(ReportIdBootloader)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdGetActiveProfiles)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(63, HID_REPORT_ID(ReportIdUpdateFirmware)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(firmware_upload_report_size, HID_REPORT_ID(ReportIdUploadFirmware)),
        TUD_HID_REPORT_DESC_GENERIC_OUTPUT(firmware_upload_report_size, HID_REPORT_ID(ReportIdUploadFirmware)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(sizeof(version) + 1 + sizeof(build_number), HID_REPORT_ID(ReportIdGetVersion)),
        TUD_HID_REPORT_DESC_GENERIC_FEATURE(sizeof(type) + 1, HID_REPORT_ID(ReportIdGetType)),
        HID_COLLECTION_END};

HIDConfigDevice::HIDConfigDevice()
{
}
void HIDConfigDevice::initialize()
{
  m_epin = next_epin();
  m_epout = next_epout();
}
void HIDConfigDevice::process(bool full_poll, bool send_events)
{
  // Before the tool checks below: a hotkey tool only sends commands, never keepalives
  if (!pending_cycle_commands.empty())
  {
    auto commands = std::move(pending_cycle_commands);
    pending_cycle_commands.clear();
    for (const auto &command : commands)
    {
      auto device = DeviceManager::instance().get_root_device(command.command.setCycle.id);
      if (device)
      {
        device->handle_command(command);
      }
    }
  }
  if (clearedIn && clearedOut)
  {
    ConfigManager::instance().host_detected(ModeSwitch);
  }
  // if (millis() - lastKeepAlive > 10000) {
  //   if (debug_enabled) {
  //     deinitDebug();
  //     debug_enabled = false;
  //   }
  // }
  if (tool_closed())
  {
    // Keep the tool's selection: the keepalive can lapse while the tool is still open (its tab was
    // in the background, a config was being written), and it won't select the profile again after.
    // reset_keepalive() clears it when the tool really goes away.
    if (profile_selected)
    {
      tool_lapsed = true;
    }
    ProfileManager::instance().release_tool_profile();
    // A forced mode was held off while the tool was open, apply it now that the tool is gone
    auto &config_mgr = ConfigManager::instance();
    if (config_mgr.is_forced_mode_deferred() && !config_mgr.is_working() && !tool_may_be_open())
    {
      config_mgr.clear_deferred_forced_mode();
      reload();
    }
    return;
  }
  if (tool_lapsed)
  {
    // The tool missed whatever happened in the meantime, so start it over from the current state
    tool_lapsed = false;
    profile_changed = true;
    just_loaded = true;
  }
  bool profile_just_changed = profile_changed;
  if (profile_changed)
  {
    profile_changed = false;
    ProfileManager::instance().for_each_profile([](uint32_t profile_id, const auto &profile)
                                                {
      (void)profile_id;
      for (const auto &led : profile->leds)
      {
        led->off();
      } });
  }
  if (just_loaded)
  {
    DeviceManager::instance().update_all_devices(true, true);
    ProfileManager::instance().update_all_profile_devices(true, true);
    just_loaded = false;
  }
  if (profile_selected)
  {
    auto selected_ptr = ProfileManager::instance().load_tool_profile(selected_profile, selected_instance);
    if (!selected_ptr)
    {
      return;
    }
    auto selected = std::make_pair(selected_profile, selected_ptr);
    if (detect_done)
    {

      switch (m_detect_type)
      {
      case DetectDigital:
      {
        uint32_t now = millis();
        for (uint8_t i = 0; i < NUM_BANK0_GPIOS; i++)
        {
          uint64_t bit = 1ull << i;
          if (!(m_valid_pins & bit) || (m_detect_noisy & bit))
          {
            continue;
          }
          if (gpio_get(i) == last_digital_vals[i])
          {
            m_detect_changed &= ~bit;
            continue;
          }
          if (!(m_detect_changed & bit))
          {
            // Changes close together are bounce from one press, so only count ones further apart
            if (now - m_detect_changed_at[i] >= DETECT_BOUNCE_MS && ++m_detect_transitions[i] > DETECT_NOISY_TRANSITIONS)
            {
              // Keeps flicking without being held (a misfiring tilt switch, say), so it would win every detect
              printf("ignoring noisy pin: %d\r\n", i);
              m_detect_noisy |= bit;
            }
            m_detect_changed |= bit;
            m_detect_changed_at[i] = now;
            continue;
          }
          // Only take a pin once it has been held, so brief misfires don't get picked up
          if (now - m_detect_changed_at[i] >= DETECT_HOLD_MS)
          {
            printf("detected digital: %d %d %d\r\n", i, gpio_get(i), last_digital_vals[i]);
            detect_done = 0;
            proto_Event evt;
            evt.which_event = proto_Event_pin_tag;
            evt.event.pin.pin = i;
            send_event(evt, true);
            break;
          }
        }
        break;
      }
      case DetectAnalog:
        for (uint8_t i = 0; i < NUM_ADC_CHANNELS; i++)
        {
          if (m_valid_pins & (1ull << i))
          {
            adc_select_input(i);
            if (abs(last_adc_vals[i] - adc_read()) > 100)
            {
              // found!
              printf("detected adc: %d %d %d\r\n", last_adc_vals[i], adc_read(), i + ADC_BASE_PIN);
              detect_done = 0;
              proto_Event evt;
              evt.which_event = proto_Event_pin_tag;
              evt.event.pin.pin = i + ADC_BASE_PIN;
              send_event(evt, true);
              break;
            }
          }
        }
        break;
      }

      if (profile_just_changed || (detect_done && millis() > detect_done))
      {
        detect_done = 0;
      }
      if (!detect_done)
      {
        switch (m_detect_type)
        {
        case DetectDigital:
          for (uint8_t i = 0; i < NUM_BANK0_GPIOS; i++)
          {
            if (m_valid_pins & (1ull << i))
            {
              gpio_init(i);
              gpio_set_dir(i, false);
              gpio_set_pulls(i, false, false);
            }
          }
          break;
        case DetectAnalog:
          for (uint8_t i = 0; i < NUM_ADC_CHANNELS; i++)
          {
            if (m_valid_pins & (1ull << i))
            {
              adc_gpio_init(i + ADC_BASE_PIN);
              gpio_set_pulls(i + ADC_BASE_PIN, false, false);
            }
          }
          break;
        }
        for (auto &mapping : selected.second->mappings)
        {
          mapping->reload();
        }
        for (auto &led : selected.second->leds)
        {
          led->reload();
        }
      }
    }
    else
    {
      if (profile_just_changed)
      {
        DeviceManager::instance().update_all_devices(profile_just_changed, true);
        ProfileManager::instance().update_all_profile_devices(profile_just_changed, true);
      }
      ProfileManager::instance().update_profile_components(selected_profile, selected_instance, profile_just_changed, true);
      if (profile_just_changed)
      {
        m_input_detector.end();
      }
      proto_Event evt = proto_Event_init_zero;
      evt.which_event = proto_Event_inputDetection_tag;
      if (m_input_detector.poll(selected.second, evt.event.inputDetection))
      {
        printf("detected input: %d %ld\r\n", evt.event.inputDetection.axis, evt.event.inputDetection.value);
        send_event(evt, true);
      }
    }
  }
  process_events();
}

void HIDConfigDevice::process_events()
{
  // Keeps BTstack callbacks from queueing (and sending) events between the busy check and
  // the transfer below, which made TinyUSB assert on a busy endpoint and lost the events
  BtStackLock lock;
  // safety net in case event_count was ever pushed past the array's capacity
  if (list.event_count > TU_ARRAY_SIZE(list.event))
  {
    list.event_count = TU_ARRAY_SIZE(list.event);
  }
  if (list.event_count == 0 || events_busy())
  {
    return;
  }

  epin_buf[0] = ReportId::ReportIdConfig;
  // bluetooth can only notify as much as fits in its MTU
  uint16_t max_size = 63;
  if (m_transport == ConfigTransport::Bluetooth)
  {
    max_size = std::min<uint16_t>(max_size, bt_config_max_event_size());
  }

  // A full batch (up to max_count events) can overflow the 63 byte report once a
  // large event (e.g. UsbDeviceHotplugEvent's name) is in it - shrink the batch
  // (in place, no extra stack copy of the struct) until it actually fits instead
  // of silently dropping everything on overflow.
  pb_size_t total_count = list.event_count;
  pb_size_t sent_count = total_count;
  bool encoded = false;
  while (sent_count > 0)
  {
    list.event_count = sent_count;
    pb_ostream_t outputStream = pb_ostream_from_buffer(epin_buf + 1, max_size);
    if (pb_encode_delimited(&outputStream, proto_EventList_fields, &list))
    {
      if (m_transport == ConfigTransport::Bluetooth)
      {
        // zero the unused tail, so the report always matches its declared size
        memset(epin_buf + 1 + outputStream.bytes_written, 0, max_size - outputStream.bytes_written);
        if (!bt_config_send_event(epin_buf + 1, max_size))
        {
          // couldn't queue the notification, try again later
          list.event_count = total_count;
          return;
        }
      }
      else if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, 64, false))
      {
        // not sent, keep them for the next try
        list.event_count = total_count;
        return;
      }
      encoded = true;
      break;
    }
    sent_count--;
  }
  if (!encoded)
  {
    // Not even a single event fits - drop it rather than spin on it forever.
    sent_count = 1;
  }

  // Keep whatever didn't get sent this round queued for the next flush.
  for (pb_size_t i = sent_count; i < total_count; i++)
  {
    list.event[i - sent_count] = list.event[i];
  }
  list.event_count = total_count - sent_count;
}

bool HIDConfigDevice::events_busy()
{
  if (m_transport == ConfigTransport::Bluetooth)
  {
    return !bt_config_can_send_event();
  }
  return !tud_ready() || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin);
}

size_t HIDConfigDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
  return 0;
}

size_t HIDConfigDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
  uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_config), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
  assert(sizeof(desc) <= remaining);
  memcpy(dest, desc, sizeof(desc));
  return sizeof(desc);
}

size_t HIDConfigDevice::device_name(uint8_t idx, char *desc)
{
  return 0;
}

void HIDConfigDevice::device_descriptor(tusb_desc_device_t *desc)
{
}
const uint8_t *HIDConfigDevice::report_descriptor()
{
  return desc_hid_report_config;
}

uint16_t HIDConfigDevice::report_desc_len()
{
  return sizeof(desc_hid_report_config);
}

void HIDConfigDevice::handle_command(proto_Command command)
{
  switch (command.which_command)
  {
  case proto_Command_setProfile_tag:
  {
    printf("Set id %lu: %d, instance: %d\r\n", static_cast<unsigned long>(millis()), command.command.setProfile.profileId, command.command.setProfile.instanceId);
    profile_selected = true;
    profile_changed = true;
    selected_profile = command.command.setProfile.profileId;
    selected_instance = command.command.setProfile.has_instanceId ? command.command.setProfile.instanceId : 0;
    break;
  }
  case proto_Command_reboot_tag:
  {
    reload();
    return;
  }
  case proto_Command_save_tag:
  {
    printf("Save command received\r\n");
    save_config();
    return;
  }
  case proto_Command_disconnect_tag:
  {
    reset_keepalive();
    return;
  }
  case proto_Command_unpairBluetooth_tag:
  {
    bt_forget_pairing(command.command.unpairBluetooth.id);
    return;
  }
  case proto_Command_forgetBluetoothHosts_tag:
  {
    bt_forget_hosts();
    return;
  }
  case proto_Command_scan_tag:
  {
    DeviceManager::instance().for_each_active_device([&command](const auto &device)
    {
      device->handle_command(command);
    });
    break;
  }
  case proto_Command_crkdDrum_tag:
  {
    std::static_pointer_cast<CrkdDrumDevice>(DeviceManager::instance().get_root_device(command.command.crkdDrum.id))->drum.setParam(command.command.crkdDrum.type, command.command.crkdDrum.axisType, command.command.crkdDrum.val);
    break;
  }
  case proto_Command_testLed_tag:
  {
    uint32_t prof_id = command.command.testLed.has_profileId ? command.command.testLed.profileId : selected_profile;
    uint32_t duration = command.command.testLed.has_durationMs ? command.command.testLed.durationMs : 2000;
    auto prof = ProfileManager::instance().get_profile(prof_id, selected_instance);
    if (!prof)
    {
      prof = ProfileManager::instance().load_tool_profile(prof_id, selected_instance);
    }
    if (prof)
    {
      uint32_t led_id = command.command.testLed.has_id ? command.command.testLed.id : UINT32_MAX;
      if (led_id == UINT32_MAX)
      {
        for (const auto &led : prof->leds)
        {
          if (led)
          {
            led->test(duration);
          }
        }
      }
      else if (led_id < prof->leds.size() && prof->leds[led_id])
      {
        prof->leds[led_id]->test(duration);
      }
    }
    break;
  }
  case proto_Command_setCycle_tag:
  {
    // A handful is plenty between two loops, more is a host spamming it
    if (pending_cycle_commands.size() < 8)
    {
      pending_cycle_commands.push_back(command);
    }
    return;
  }
  case proto_Command_detectInput_tag:
  {
    // only detect for 10 seconds, like pins
    printf("detect input: %d %ld\r\n", command.command.detectInput.kind, command.command.detectInput.deviceid);
    m_input_detector.begin(command.command.detectInput, millis() + 10000);
    break;
  }
  case proto_Command_detectPin_tag:
  {
    // only detect for 10 seconds

    printf("detect: %d\r\n", command.command.detectPin.detectType);
    detect_done = millis() + 10000;
    m_valid_pins = 0;
    m_detect_changed = 0;
    m_detect_noisy = 0;
    memset(m_detect_changed_at, 0, sizeof(m_detect_changed_at));
    memset(m_detect_transitions, 0, sizeof(m_detect_transitions));
    m_detect_type = command.command.detectPin.detectType;
    switch (command.command.detectPin.detectType)
    {
    case DetectDigital:
      for (uint8_t i = 0; i < NUM_BANK0_GPIOS; i++)
      {
        if (i == 23)
        {
          continue;
        }
        bool found = false;
        DeviceManager::instance().for_each_active_device([&found, i](const auto &device)
                                                         {
          if (device->using_pin(i))
          {
            found = true;
          } });
        if (!found)
        {
          m_valid_pins |= 1ull << i;
          gpio_init(i);
          gpio_set_dir(i, false);
          gpio_set_pulls(i, true, false);
          sleep_us(1);
          last_digital_vals[i] = gpio_get(i);
        }
      }
      break;
    case DetectAnalog:
      for (uint8_t i = 0; i < NUM_ADC_CHANNELS; i++)
      {
        bool found = false;
        DeviceManager::instance().for_each_active_device([&found, i](const auto &device)
                                                         {
          if (device->using_pin(i + ADC_BASE_PIN))
          {
            found = true;
          } });
        if (!found)
        {
          m_valid_pins |= 1ull << i;
          adc_gpio_init(i + ADC_BASE_PIN);
          gpio_set_pulls(i + ADC_BASE_PIN, true, false);
          adc_select_input(i);
          sleep_us(10);
          last_adc_vals[i] = adc_read();
        }
      }
      break;
    }
  }
  break;
  }
}

void HIDConfigDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  set_report_from(ConfigTransport::Usb, report_id, report_type, buffer, bufsize);
}

void HIDConfigDevice::switch_transport(ConfigTransport transport)
{
  if (transport == m_transport)
  {
    return;
  }
  // events queued for a tool still connected over the old transport are of no use to this one,
  // but once a bluetooth tool has gone they are still current, such as the status changes its
  // disconnect caused
  if (m_transport != ConfigTransport::Bluetooth || bt_config_connected())
  {
    list.event_count = 0;
  }
  m_transport = transport;
}

bool HIDConfigDevice::in_use_elsewhere(ConfigTransport transport) const
{
  if (transport == m_transport)
  {
    return false;
  }
  // Both tools share the read and write position in the config, and each uploads its own copy of it,
  // so one would scramble the other's reads and undo its changes. The tool already open keeps the
  // device until it closes: its keepalive lapses (or it doesn't return after a reload) and it isn't
  // partway through an upload
  return ConfigManager::instance().is_working() || tool_may_be_open();
}

uint16_t HIDConfigDevice::handle_get_report_in_use(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  if (report_type != HID_REPORT_TYPE_FEATURE)
  {
    return 0;
  }
  switch (report_id)
  {
  // enough for the tool to say what the device is and why it can't open it
  case ReportId::ReportIdGetVersion:
  case ReportId::ReportIdGetType:
  case ReportId::ReportIdGetActiveProfiles:
    return handle_get_report(report_id, report_type, buffer, reqlen);
  case ReportId::ReportIdConfigInfo:
  case ReportId::ReportIdConfigInfoSaved:
    // without touching where the open tool is up to
    buffer[0] = report_id;
    return copy_config_info(buffer + 1, report_id == ReportId::ReportIdConfigInfo, false, config_chunk_size(reqlen), true) + 1;
  default:
    // no data, the same as reading past the end of the config
    buffer[0] = report_id;
    return 1;
  }
}

bool HIDConfigDevice::set_report_from(ConfigTransport transport, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  if (in_use_elsewhere(transport))
  {
    // Keepalives included, or the open tool would never be seen to close
    return false;
  }
  switch_transport(transport);
  return handle_set_report(report_id, report_type, buffer, bufsize);
}

uint16_t HIDConfigDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  return get_report_from(ConfigTransport::Usb, report_id, report_type, buffer, reqlen);
}

uint16_t HIDConfigDevice::get_report_from(ConfigTransport transport, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  if (in_use_elsewhere(transport))
  {
    return handle_get_report_in_use(report_id, report_type, buffer, reqlen);
  }
  switch_transport(transport);
  return handle_get_report(report_id, report_type, buffer, reqlen);
}

bool HIDConfigDevice::is_registered()
{
  bool found = false;
  ProfileManager::instance().for_each_instance([this, &found](const auto &instance)
                                               {
    if (instance.get() == this)
    {
      found = true;
    } });
  return found;
}

bool HIDConfigDevice::handle_set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  if (report_type == HID_REPORT_TYPE_FEATURE ||
      (report_type == HID_REPORT_TYPE_OUTPUT && report_id == ReportId::ReportIdUploadFirmware))
  {
    // skip over report id
    buffer++;
    bufsize--;
    switch (report_id)
    {
    case ReportId::ReportIdConfig:
      tool_seen = true;
      lastKeepAlive = millis();
      write_config(buffer, bufsize, start);
      start += bufsize;
      break;
    case ReportId::ReportIdUploadFirmware:
    {
      tool_seen = true;
      if (!fw_update_active)
      {
        printf("fw update data before metadata\r\n");
        break;
      }
      if (update_state.firmwareSize <= 0 ||
          update_state.offset < 0 ||
          update_state.chunkOffset < 0 ||
          update_state.chunkSize <= 0)
      {
        printf("invalid fw update state\r\n");
        break;
      }
      uint32_t firmware_size = static_cast<uint32_t>(update_state.firmwareSize);
      uint32_t firmware_offset = static_cast<uint32_t>(update_state.offset);
      uint32_t chunk_offset = static_cast<uint32_t>(update_state.chunkOffset);
      if (chunk_offset >= firmware_write_block_size)
      {
        printf("invalid fw update chunk offset\r\n");
        break;
      }
      uint32_t uploaded_before = firmware_offset + chunk_offset;
      if (uploaded_before >= firmware_size)
      {
        printf("fw update received data past end\r\n");
        break;
      }
      uint32_t remaining_firmware = firmware_size - uploaded_before;
      uint32_t remaining_block = firmware_write_block_size - chunk_offset;
      uint32_t to_copy = std::min<uint32_t>({
          bufsize,
          static_cast<uint32_t>(update_state.chunkSize),
          remaining_block,
          remaining_firmware,
      });
      memcpy(fw_update_tmp + chunk_offset, buffer, to_copy);
      chunk_offset += to_copy;
      update_state.chunkOffset = chunk_offset;
      if (chunk_offset == firmware_write_block_size ||
          (firmware_offset + chunk_offset) >= firmware_size)
      {
        uint32_t uploaded = firmware_offset + chunk_offset;
        if (m_target_secondary_pico)
        {
          uint32_t send_len = uploaded >= firmware_size
                                  ? ((chunk_offset + 255) / 256) * 256
                                  : chunk_offset;
          for (uint32_t i = 0; i < send_len; i += 256)
          {
            uint32_t len = std::min<uint32_t>(256, send_len - i);
            m_target_secondary_pico->send_ota_chunk(firmware_offset + i, fw_update_tmp + i, len);
          }
          if (uploaded >= firmware_size)
          {
            m_target_secondary_pico->send_ota_finish(firmware_size);
            m_target_secondary_pico = nullptr;
            fw_update_active = false;
          }
          else
          {
            update_state.offset = uploaded;
            update_state.chunkOffset = 0;
            memset(fw_update_tmp, 0, sizeof(fw_update_tmp));
          }
        }
        else
        {
          flash_write_begin();
          uint32_t write_size =
              uploaded >= firmware_size
                  ? ((chunk_offset + firmware_write_alignment - 1) / firmware_write_alignment) * firmware_write_alignment
                  : firmware_write_block_size;
          bool write_failed = pfb_write_to_flash_aligned_256_bytes(
              fw_update_tmp,
              firmware_offset,
              write_size);
          if (write_failed)
          {
            printf("failed to write update! %02x\r\n", firmware_offset);
          }
          if (!write_failed && uploaded >= firmware_size)
          {
            printf("fw uploaded! checking\r\n");
            if (pfb_firmware_sha256_check(firmware_size))
            {
              printf("sha failed!\r\n");
            }
            else
            {
              pfb_mark_download_slot_as_valid();
              pfb_perform_update();
            }
          }
          else if (!write_failed)
          {
            update_state.offset = uploaded;
            update_state.chunkOffset = 0;
            memset(fw_update_tmp, 0, sizeof(fw_update_tmp));
          }
          flash_write_end();
        }
      }
      break;
    }
    case ReportId::ReportIdUpdateFirmware:
    {
      // An update written with this firmware's layout would be swapped in by the bootloader from where its
      // own layout puts the download slot. Only a UF2 (bootloader included) can update it safely
      if (!pfb_bootloader_layout_matches())
      {
        printf("Bootloader layout doesn't match, refusing an OTA update\r\n");
        fw_update_active = false;
        break;
      }
      proto_FirmwareUpdate decoded;
      if (!decode_firmware_update(buffer, bufsize, &decoded))
      {
        printf("Didn't decode fw update?\r\n");
        fw_update_active = false;
        break;
      }
      // A streamed upload sends this between blocks, both to wait for the block to be written and to
      // check it all arrived: the offset has to be where the upload is up to, which is the start of the
      // next block once a block is written. Otherwise refuse it, so the configurator sends that block again
      // (starting with this report at the offset it is up to, which resets the block).
      if (fw_update_active && !m_target_secondary_pico && decoded.offset > 0 &&
          !(decoded.has_deviceId && decoded.deviceId > 0) && decoded.offset != update_state.offset)
      {
        printf("fw update at 0x%lx, expected 0x%lx\r\n", (unsigned long)decoded.offset, (unsigned long)update_state.offset);
        return false;
      }
      update_state = decoded;
      // printf("fw update offset: %02x\r\n", update_state.offset);
      tool_seen = true;
      fw_update_active = true;
      if (update_state.has_deviceId && update_state.deviceId > 0)
      {
        auto dev = DeviceManager::instance().get_root_device(update_state.deviceId);
        m_target_secondary_pico = std::static_pointer_cast<SecondaryPicoDevice>(dev);
      }
      else
      {
        m_target_secondary_pico = nullptr;
      }
      if (update_state.offset == 0)
      {
        update_state.chunkOffset = 0;
        memset(fw_update_tmp, 0, sizeof(fw_update_tmp));
        if (m_target_secondary_pico)
        {
          m_target_secondary_pico->send_ota_begin(update_state.firmwareSize);
        }
        else
        {
          flash_write_begin();
          pfb_initialize_download_slot();
          flash_write_end();
        }
      }
      break;
    }
    case ReportId::ReportIdConfigInfo:
      lastKeepAlive = millis();
      tool_seen = true;
      start = 0;
      write_config_info(buffer, bufsize);
      break;
    case ReportId::ReportIdLoaded:
      lastKeepAlive = millis();
      tool_seen = true;
      just_loaded = true;
      if (!debug_enabled)
      {
        initDebug();
        debug_enabled = true;
      }
      break;
    case ReportId::ReportIdKeepalive:
      if (!ConfigManager::instance().is_working())
      {
        // ignore keepalives until we are ready
        lastKeepAlive = millis();
        tool_seen = true;
      }
      break;
    case ReportId::ReportIdBootloader:
      reset_usb_boot(0, 0);
      break;
    case ReportId::ReportIdCommand:
    {
      tool_seen = true;
      proto_Command cmd;
      pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
      if (!pb_decode_delimited(&inputStream, proto_Command_fields, &cmd))
      {
        printf("Didn't decode cmd?\r\n");
        break;
      }
      handle_command(cmd);
      break;
    }
    }
  }
  return true;
}

bool encode_active_profiles(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
  bool ok = true;
  ProfileManager::instance().for_each_active_profile([stream, field, &ok](uint32_t profile_id, const auto &profile)
                                                     {
    (void)profile;
    if (!ok)
      return;
    if (!pb_encode_tag_for_field(stream, field))
    {
      ok = false;
      return;
    }

    if (!pb_encode_varint(stream, profile_id))
    {
      ok = false;
    } });
  return ok;
}

bool decode_firmware_update(const uint8_t *buffer, uint16_t bufsize, proto_FirmwareUpdate *update)
{
  for (uint16_t offset = 0; offset < 2 && offset < bufsize; offset++)
  {
    proto_FirmwareUpdate decoded = proto_FirmwareUpdate_init_zero;
    pb_istream_t inputStream = pb_istream_from_buffer(buffer + offset, bufsize - offset);
    if (pb_decode_delimited(&inputStream, proto_FirmwareUpdate_fields, &decoded))
    {
      *update = decoded;
      return true;
    }
  }
  return false;
}

uint16_t HIDConfigDevice::handle_get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  if (report_type != HID_REPORT_TYPE_FEATURE)
  {
    return 0;
  }

  switch (report_id)
  {
  case ReportId::ReportIdConfig:
  {
    buffer[0] = report_id;
    buffer++;
    uint32_t ret = copy_config(buffer, start, cached, config_chunk_size(reqlen));
    start += ret;
    return ret + 1;
  }
  case ReportId::ReportIdGetVersion:
  {
    // the id, the string, NUL and one zero byte, then the build number (little endian). Older
    // configurators read up to the NUL, so they still see the hash.
    buffer[0] = report_id;
    memcpy(buffer + 1, version, sizeof(version));
    buffer[1 + sizeof(version)] = 0;
    memcpy(buffer + 2 + sizeof(version), &build_number, sizeof(build_number));
    return sizeof(version) + 2 + sizeof(build_number);
  }
  case ReportId::ReportIdGetType:
  {
    // the id, then the declared sizeof(type) + 1 bytes
    buffer[0] = report_id;
    memcpy(buffer + 1, type, sizeof(type));
    buffer[1 + sizeof(type)] = 0;
    return sizeof(type) + 2;
  }
  case ReportId::ReportIdGetActiveProfiles:
  {
    buffer[0] = report_id;
    buffer++;
    auto stream = pb_ostream_from_buffer(buffer, reqlen - 1);
    proto_GetActiveProfiles resp = proto_GetActiveProfiles_init_zero;
    resp.profiles.funcs.encode = encode_active_profiles;
    auto &config_mgr = ConfigManager::instance();
    if (config_mgr.is_forced_mode_deferred())
    {
      resp.has_deferredMode = true;
      resp.deferredMode = config_mgr.get_deferred_mode();
    }
    if (!pb_encode_delimited(&stream, proto_GetActiveProfiles_fields, &resp))
      return 1;
    return std::min<uint16_t>(reqlen, 64);
  }
  case ReportId::ReportIdConfigInfo:
    cached = true;
    buffer[0] = report_id;
    buffer++;
    start = 0;
    return copy_config_info(buffer, cached, m_transport == ConfigTransport::Bluetooth, config_chunk_size(reqlen)) + 1;
  case ReportId::ReportIdConfigInfoSaved:
    cached = false;
    buffer[0] = report_id;
    buffer++;
    start = 0;
    return copy_config_info(buffer, cached, m_transport == ConfigTransport::Bluetooth, config_chunk_size(reqlen)) + 1;
  }
  return 0;
}

bool HIDConfigDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
  // The host reading an event is proof the tool is alive, even if its keepalive timer
  // is being throttled (e.g. a backgrounded browser tab)
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN && result == XFER_RESULT_SUCCESS && tool_seen)
  {
    lastKeepAlive = millis();
  }
  return HIDDevice::interrupt_xfer(ep_addr, result, xferred_bytes);
}

bool HIDConfigDevice::tool_closed()
{
  auto dev = HIDConfigDevice::instance;
  if (!dev || !dev->tool_seen || ConfigManager::instance().is_working())
  {
    return true;
  }
  return millis() - dev->lastKeepAlive > 1000;
}

bool HIDConfigDevice::tool_may_be_open()
{
  auto dev = HIDConfigDevice::instance;
  if (!tool_closed())
  {
    return true;
  }
  return dev && dev->tool_return_deadline && (int32_t)(dev->tool_return_deadline - millis()) > 0;
}

bool HIDConfigDevice::has_event_space()
{
  auto dev = HIDConfigDevice::instance;
  if (!dev || tool_closed())
  {
    return false;
  }
  dev->process_events();
  return dev->list.event_count < TU_ARRAY_SIZE(dev->list.event);
}

void HIDConfigDevice::flush_events(uint32_t timeout_us)
{
  auto dev = HIDConfigDevice::instance;
  if (!dev || is_secondary_pico_mode())
  {
    return;
  }
  const uint32_t started_us = micros();
  while ((dev->list.event_count || (dev->m_transport == ConfigTransport::Usb && tud_ready() && usbd_edpt_busy(TUD_OPT_RHPORT, dev->m_epin))) &&
         !tool_closed() && micros() - started_us < timeout_us)
  {
    tud_task();
    dev->process_events();
  }
}

bool HIDConfigDevice::send_event(proto_Event event, bool now)
{
  if (is_secondary_pico_mode())
  {
    return secondary_pico_enqueue_event(&event);
  }
  auto dev = HIDConfigDevice::instance;
  if (tool_closed())
  {
    return false;
  }
  // flush queue if event is important. Bounded by tool_closed(): if the host stops
  // reading (tool closed, USB suspended) this gives up after the keepalive timeout
  // instead of spinning forever, and the event is still queued below if there is room.
  // Not from an interrupt though (BTstack callbacks run in one on the Pico W), as that
  // would run tud_task() there; the main loop sends what gets queued.
  while (dev->list.event_count && now && !tool_closed() && !__get_current_exception())
  {
    dev->process_events();
    tud_task();
  }
  // BTstack callbacks queue events too, from an interrupt that could otherwise land
  // in the middle of the main loop changing the queue
  BtStackLock lock;
  // Check if the event is an axis event and update the existing event in the list if it exists.
  if (event.which_event == proto_Event_axis_tag) {
    for (size_t i = 0; i < dev->list.event_count; i++)
    {
      if (dev->list.event[i].which_event == proto_Event_axis_tag)
      {
        if (dev->list.event[i].event.axis.id == event.event.axis.id)
        {
          dev->list.event[i] = event;
          return true;
        }
      }
    }
  }
  // Check if the event is a button event and update the existing event in the list if it exists.
  if (event.which_event == proto_Event_button_tag) {
    for (size_t i = 0; i < dev->list.event_count; i++)
    {
      if (dev->list.event[i].which_event == proto_Event_button_tag)
      {
        if (dev->list.event[i].event.button.id == event.event.button.id)
        {
          dev->list.event[i] = event;
          return true;
        }
      }
    }
  }
  bool sent = false;
  if (dev->list.event_count < TU_ARRAY_SIZE(dev->list.event))
  {
    dev->list.event[dev->list.event_count++] = event;
    sent = true;
  }
  if (!sent)
  {
    return false;
  }
  return true;
}

void HIDConfigDevice::reset_keepalive()
{
  auto dev = HIDConfigDevice::instance;
  dev->lastKeepAlive = 0;
  dev->tool_seen = false;
  dev->tool_return_deadline = 0;
  dev->selected_profile = 0;
  dev->profile_selected = false;
  dev->profile_changed = false;
  dev->tool_lapsed = false;
}

void HIDConfigDevice::restart_keepalive()
{
  // The tool waits this long for the device to come back after a reload
  constexpr uint32_t TOOL_RETURN_GRACE_MS = 10000;
  const bool was_open = !tool_closed();
  reset_keepalive();
  if (was_open)
  {
    // 0 means no deadline
    instance->tool_return_deadline = (millis() + TOOL_RETURN_GRACE_MS) | 1;
  }
}

std::shared_ptr<HIDConfigDevice> HIDConfigDevice::instance = std::make_shared<HIDConfigDevice>();
const uint8_t *hid_config_report_descriptor(uint16_t *len)
{
  *len = sizeof(desc_hid_report_config);
  return desc_hid_report_config;
}

uint16_t hid_config_bt_get_report(uint8_t report_id, uint8_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  return HIDConfigDevice::instance->get_report_from(ConfigTransport::Bluetooth, report_id, static_cast<hid_report_type_t>(report_type), buffer, reqlen);
}

bool hid_config_bt_set_report(uint8_t report_id, uint8_t report_type, const uint8_t *buffer, uint16_t len)
{
  return HIDConfigDevice::instance->set_report_from(ConfigTransport::Bluetooth, report_id, static_cast<hid_report_type_t>(report_type), buffer, len);
}

void hid_config_bt_process(bool full_poll, bool send_events)
{
  auto dev = HIDConfigDevice::instance;
  if (!dev->is_registered())
  {
    dev->process(full_poll, send_events);
  }
}
