#include "emulation/bt/bt_config_service.h"
#include "devices/bt/bluetooth_stack.hpp"
#include "emulation/bt/bt_profile.h"
#include "btstack.h"
#include <string.h>
#include <stdio.h>

// set to 1 to log every config request made over bluetooth
#define BT_CONFIG_DEBUG 0
#if BT_CONFIG_DEBUG
#define config_debug(...) printf(__VA_ARGS__)
#else
#define config_debug(...)
#endif

// btstack's hids_device ignores read offsets and drops long writes, and calls into the app from the
// bluetooth stack. Config reports are 63 bytes and the config code expects to run from the main loop,
// so this service handles its own ATT requests: reads and writes are queued, answered as pending, and
// completed once the main loop has passed them to HIDConfigDevice.
// The same reports are also served from a plain GATT service for Web Bluetooth, which blocks HID services.

#define CONFIG_SERVICE_START ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_02_START_HANDLE
#define CONFIG_SERVICE_END ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_02_END_HANDLE
#define WEB_SERVICE_START ATT_SERVICE_53414E54_524F_4C4C_4552_000000000000_START_HANDLE
#define WEB_SERVICE_END ATT_SERVICE_53414E54_524F_4C4C_4552_000000000000_END_HANDLE
#define WEB_REPORT(id) {0x##id, ATT_CHARACTERISTIC_53414E54_524F_4C4C_4552_0000000000##id##_01_VALUE_HANDLE}
#define WEB_EVENTS_CLIENT_CONFIGURATION ATT_CHARACTERISTIC_53414E54_524F_4C4C_4552_000000000022_01_CLIENT_CONFIGURATION_HANDLE
#define MAX_CONFIG_REPORTS 32
#define MAX_PENDING_WRITES 4
// largest config report, without the report id
#define MAX_REPORT_SIZE 64

// Web Bluetooth characteristics, by report id
static const struct
{
    uint8_t id;
    uint16_t value_handle;
} s_web_reports[] = {WEB_REPORT(22), WEB_REPORT(23), WEB_REPORT(24), WEB_REPORT(25), WEB_REPORT(26), WEB_REPORT(27),
                     WEB_REPORT(28), WEB_REPORT(29), WEB_REPORT(30), WEB_REPORT(31), WEB_REPORT(32)};

// Somewhere config events can be notified to
struct EventTarget
{
    uint16_t value_handle;
    uint16_t client_configuration_handle;
    uint16_t notifications;
};

enum ConfigServiceType
{
    ServiceHid,
    ServiceWeb,
};

struct ConfigReport
{
    uint16_t value_handle;
    uint8_t id;
    hid_report_type_t type;
    uint16_t size;
    ConfigServiceType service;
};

enum ReadState
{
    ReadIdle,
    // waiting for the main loop to fetch the report
    ReadQueued,
    // fetched, waiting for the ATT server to ask for it again
    ReadReady,
    // being sent, possibly over several blob reads
    ReadServing,
};

struct PendingWrite
{
    const ConfigReport *report;
    uint8_t data[MAX_REPORT_SIZE];
    uint16_t len;
    // false for executed long writes, which btstack can't delay the response for
    bool needs_ack;
};

static att_service_handler_t s_service_handler;
static att_service_handler_t s_web_service_handler;
static bool s_registered = false;
static ConfigReport s_reports[MAX_CONFIG_REPORTS];
static uint8_t s_report_count = 0;
static uint16_t s_report_map_handle = 0;
static uint16_t s_control_point_handle = 0;
// indexed by ConfigServiceType: the HID input report, and the Web Bluetooth events characteristic
static EventTarget s_event_targets[2];
// events go to whichever service the tool last made a request through
static ConfigServiceType s_active_service = ServiceHid;
static hci_con_handle_t s_con_handle = HCI_CON_HANDLE_INVALID;

static ReadState s_read_state = ReadIdle;
static const ConfigReport *s_read_report = nullptr;
static uint8_t s_read_buf[MAX_REPORT_SIZE];
static uint16_t s_read_len = 0;

static PendingWrite s_writes[MAX_PENDING_WRITES];
static uint8_t s_write_head = 0;
static uint8_t s_write_count = 0;
// a delayed write was handled, the ATT server will call back with it once more to send the response
static bool s_write_acked = false;
static uint16_t s_write_acked_handle = 0;

static const ConfigReport *s_prepared_report = nullptr;
static uint8_t s_prepared_buf[MAX_REPORT_SIZE];
static uint16_t s_prepared_len = 0;
static bool s_prepared_invalid = false;

static EventTarget *event_target_for_client_configuration(uint16_t handle)
{
    for (auto &target : s_event_targets)
    {
        if (target.client_configuration_handle && target.client_configuration_handle == handle)
        {
            return &target;
        }
    }
    return nullptr;
}

static const ConfigReport *report_for_handle(uint16_t handle)
{
    for (uint8_t i = 0; i < s_report_count; i++)
    {
        if (s_reports[i].value_handle == handle)
        {
            return &s_reports[i];
        }
    }
    return nullptr;
}

static bool queue_write(const ConfigReport *report, const uint8_t *data, uint16_t len, bool needs_ack)
{
    if (s_write_count >= MAX_PENDING_WRITES)
    {
        return false;
    }
    PendingWrite &write = s_writes[(s_write_head + s_write_count) % MAX_PENDING_WRITES];
    write.report = report;
    write.len = btstack_min(len, sizeof(write.data));
    memcpy(write.data, data, write.len);
    write.needs_ack = needs_ack;
    s_write_count++;
    return true;
}

static void clear_prepared_write()
{
    s_prepared_report = nullptr;
    s_prepared_len = 0;
    s_prepared_invalid = false;
}

static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    if (att_handle == ATT_READ_RESPONSE_PENDING)
    {
        return 0;
    }
    s_con_handle = con_handle;
    if (att_handle == s_report_map_handle)
    {
        uint16_t len;
        const uint8_t *desc = hid_config_report_descriptor(&len);
        return att_read_callback_handle_blob(desc, len, offset, buffer, buffer_size);
    }
    EventTarget *target = event_target_for_client_configuration(att_handle);
    if (target)
    {
        return att_read_callback_handle_little_endian_16(target->notifications, offset, buffer, buffer_size);
    }
    const ConfigReport *report = report_for_handle(att_handle);
    if (!report)
    {
        return 0;
    }
    s_active_service = report->service;
    if (report->type != HID_REPORT_TYPE_FEATURE)
    {
        // input and output reports have nothing to read, same as over USB
        static const uint8_t empty[MAX_REPORT_SIZE] = {};
        return att_read_callback_handle_blob(empty, report->size, offset, buffer, buffer_size);
    }
    bool cached = s_read_report == report && (s_read_state == ReadReady || s_read_state == ReadServing);
    if (!buffer)
    {
        // length request, made at the start of every read
        if (offset == 0 && !(cached && s_read_state == ReadReady))
        {
            // new read, let the main loop fetch it
            config_debug("BT config: read 0x%02x\r\n", report->id);
            s_read_report = report;
            s_read_state = ReadQueued;
            return ATT_READ_RESPONSE_PENDING;
        }
        if (!cached)
        {
            return 0;
        }
        s_read_state = ReadServing;
        return s_read_len;
    }
    if (!cached)
    {
        return 0;
    }
    return att_read_callback_handle_blob(s_read_buf, s_read_len, offset, buffer, buffer_size);
}

static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    // Long writes. btstack sends validate / execute / cancel to every service, so only act if they were ours
    switch (transaction_mode)
    {
    case ATT_TRANSACTION_MODE_VALIDATE:
        return s_prepared_report && s_prepared_invalid ? ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH : 0;
    case ATT_TRANSACTION_MODE_EXECUTE:
        if (s_prepared_report && !s_prepared_invalid)
        {
            queue_write(s_prepared_report, s_prepared_buf, s_prepared_len, false);
        }
        clear_prepared_write();
        return 0;
    case ATT_TRANSACTION_MODE_CANCEL:
        clear_prepared_write();
        return 0;
    default:
        break;
    }
    s_con_handle = con_handle;
    if (att_handle == s_control_point_handle)
    {
        // suspend / exit suspend, nothing to do
        return 0;
    }
    EventTarget *target = event_target_for_client_configuration(att_handle);
    if (target)
    {
        if (buffer_size >= 2)
        {
            target->notifications = little_endian_read_16(buffer, 0);
            printf("BT: config events %s (%s)\r\n", target->notifications ? "enabled" : "disabled", target == &s_event_targets[ServiceWeb] ? "web bluetooth" : "hid");
        }
        return 0;
    }
    const ConfigReport *report = report_for_handle(att_handle);
    if (!report || report->type == HID_REPORT_TYPE_INPUT)
    {
        printf("BT: config write to unexpected handle 0x%04x\r\n", att_handle);
        return ATT_ERROR_WRITE_NOT_PERMITTED;
    }
    s_active_service = report->service;
    if (transaction_mode == ATT_TRANSACTION_MODE_ACTIVE)
    {
        // a prepared write, collect it until it is executed
        if (s_prepared_report && s_prepared_report != report)
        {
            s_prepared_invalid = true;
        }
        s_prepared_report = report;
        if (offset + buffer_size > sizeof(s_prepared_buf))
        {
            s_prepared_invalid = true;
            return ATT_ERROR_INVALID_OFFSET;
        }
        memcpy(s_prepared_buf + offset, buffer, buffer_size);
        s_prepared_len = btstack_max(s_prepared_len, offset + buffer_size);
        return 0;
    }
    if (s_write_acked && s_write_acked_handle == att_handle)
    {
        // the ATT server asking again for the result of a write we already handled
        s_write_acked = false;
        return 0;
    }
    config_debug("BT config: write 0x%02x len %u\r\n", report->id, buffer_size);
    if (!queue_write(report, buffer, buffer_size, true))
    {
        printf("BT: config write queue full\r\n");
        return ATT_ERROR_INSUFFICIENT_RESOURCES;
    }
    return ATT_ERROR_WRITE_RESPONSE_PENDING;
}

void bt_config_service_init()
{
    if (s_registered)
    {
        return;
    }
    uint16_t len;
    const uint8_t *desc = hid_config_report_descriptor(&len);
    uint16_t start = CONFIG_SERVICE_START;
    uint16_t end = CONFIG_SERVICE_END;
    s_report_map_handle = gatt_server_get_value_handle_for_characteristic_with_uuid16(start, end, ORG_BLUETOOTH_CHARACTERISTIC_REPORT_MAP);
    s_control_point_handle = gatt_server_get_value_handle_for_characteristic_with_uuid16(start, end, ORG_BLUETOOTH_CHARACTERISTIC_HID_CONTROL_POINT);

    // find every report characteristic, the same way hids_device does
    s_report_count = 0;
    uint16_t chr_handle = start;
    while (chr_handle < end && s_report_count < MAX_CONFIG_REPORTS)
    {
        uint16_t value_handle = gatt_server_get_value_handle_for_characteristic_with_uuid16(chr_handle, end, ORG_BLUETOOTH_CHARACTERISTIC_REPORT);
        if (!value_handle)
        {
            break;
        }
        uint16_t reference_handle = gatt_server_get_descriptor_handle_for_characteristic_with_uuid16(chr_handle, end, ORG_BLUETOOTH_CHARACTERISTIC_REPORT, ORG_BLUETOOTH_DESCRIPTOR_REPORT_REFERENCE);
        if (!reference_handle)
        {
            break;
        }
        uint16_t reference_len;
        const uint8_t *reference = gatt_server_get_const_value_for_handle(reference_handle, &reference_len);
        if (!reference || reference_len != 2)
        {
            break;
        }
        ConfigReport &report = s_reports[s_report_count++];
        report.value_handle = value_handle;
        report.id = reference[0];
        report.type = (hid_report_type_t)reference[1];
        report.size = btstack_min(btstack_hid_get_report_size_for_id(report.id, report.type, desc, len), MAX_REPORT_SIZE);
        report.service = ServiceHid;
        if (report.type == HID_REPORT_TYPE_INPUT)
        {
            s_event_targets[ServiceHid].value_handle = value_handle;
            s_event_targets[ServiceHid].client_configuration_handle = gatt_server_get_client_configuration_handle_for_characteristic_with_uuid16(chr_handle, end, ORG_BLUETOOTH_CHARACTERISTIC_REPORT);
        }
        chr_handle = reference_handle + 1;
    }

    // Web Bluetooth reads and writes are always feature reports
    for (const auto &web : s_web_reports)
    {
        if (s_report_count >= MAX_CONFIG_REPORTS)
        {
            break;
        }
        ConfigReport &report = s_reports[s_report_count++];
        report.value_handle = web.value_handle;
        report.id = web.id;
        report.type = HID_REPORT_TYPE_FEATURE;
        report.size = btstack_min(btstack_hid_get_report_size_for_id(report.id, report.type, desc, len), MAX_REPORT_SIZE);
        report.service = ServiceWeb;
        if (web.id == 0x22)
        {
            s_event_targets[ServiceWeb].value_handle = web.value_handle;
            s_event_targets[ServiceWeb].client_configuration_handle = WEB_EVENTS_CLIENT_CONFIGURATION;
        }
    }

    s_service_handler.start_handle = start;
    s_service_handler.end_handle = end;
    s_service_handler.read_callback = &att_read_callback;
    s_service_handler.write_callback = &att_write_callback;
    att_server_register_service_handler(&s_service_handler);
    s_web_service_handler.start_handle = WEB_SERVICE_START;
    s_web_service_handler.end_handle = WEB_SERVICE_END;
    s_web_service_handler.read_callback = &att_read_callback;
    s_web_service_handler.write_callback = &att_write_callback;
    att_server_register_service_handler(&s_web_service_handler);
    s_registered = true;
    printf("BT: config service 0x%04x-0x%04x, web bluetooth 0x%04x-0x%04x, report map 0x%04x, %u reports, events %s\r\n",
           start, end, WEB_SERVICE_START, WEB_SERVICE_END, s_report_map_handle, s_report_count, s_event_targets[ServiceHid].value_handle ? "ok" : "missing");
}

void bt_config_service_disconnected()
{
    s_con_handle = HCI_CON_HANDLE_INVALID;
    for (auto &target : s_event_targets)
    {
        target.notifications = 0;
    }
    s_active_service = ServiceHid;
    s_read_state = ReadIdle;
    s_read_report = nullptr;
    s_write_count = 0;
    s_write_acked = false;
    clear_prepared_write();
}

void bt_config_service_process(bool full_poll, bool send_events)
{
    if (!s_registered)
    {
        return;
    }
    // writes first, so a read always sees the result of the writes before it
    while (true)
    {
        PendingWrite write;
        {
            BtStackLock lock;
            if (!s_write_count)
            {
                break;
            }
            write = s_writes[s_write_head];
            s_write_head = (s_write_head + 1) % MAX_PENDING_WRITES;
            s_write_count--;
        }
        uint8_t buf[MAX_REPORT_SIZE + 1];
        buf[0] = write.report->id;
        memcpy(buf + 1, write.data, write.len);
        hid_config_bt_set_report(write.report->id, write.report->type, buf, write.len + 1);
        if (write.needs_ack)
        {
            BtStackLock lock;
            s_write_acked = true;
            s_write_acked_handle = write.report->value_handle;
            if (att_server_response_ready(s_con_handle) != ERROR_CODE_SUCCESS)
            {
                s_write_acked = false;
            }
        }
    }

    const ConfigReport *read_report = nullptr;
    {
        BtStackLock lock;
        if (s_read_state == ReadQueued)
        {
            read_report = s_read_report;
        }
    }
    if (read_report)
    {
        uint8_t buf[MAX_REPORT_SIZE + 1] = {};
        uint16_t len = hid_config_bt_get_report(read_report->id, read_report->type, buf, read_report->size + 1);
        config_debug("BT config: read 0x%02x (%s) -> %u bytes: %02x %02x %02x %02x\r\n", read_report->id,
                     read_report->service == ServiceWeb ? "web" : "hid", len, buf[1], buf[2], buf[3], buf[4]);
        BtStackLock lock;
        // drop the report id. Only send what was actually returned, the same as USB does: padding it out
        // to the declared size breaks the configurator decoding reports like ConfigInfo, which are plain
        // protobuf (an odd number of trailing zeros reads as a truncated field)
        s_read_len = len > 1 ? btstack_min(len - 1, sizeof(s_read_buf)) : 0;
        memcpy(s_read_buf, buf + 1, s_read_len);
        // the request may have been dropped by a disconnect while it was fetched
        if (s_read_state == ReadQueued && s_read_report == read_report)
        {
            s_read_state = ReadReady;
            if (att_server_response_ready(s_con_handle) != ERROR_CODE_SUCCESS)
            {
                s_read_state = ReadIdle;
            }
        }
    }

    hid_config_bt_process(full_poll, send_events);
}

bool bt_config_can_send_event()
{
    BtStackLock lock;
    const EventTarget &target = s_event_targets[s_active_service];
    return target.value_handle && s_con_handle != HCI_CON_HANDLE_INVALID &&
           (target.notifications & GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION) &&
           att_server_can_send_packet_now(s_con_handle);
}

uint16_t bt_config_max_event_size()
{
    BtStackLock lock;
    if (s_con_handle == HCI_CON_HANDLE_INVALID)
    {
        return 0;
    }
    uint16_t mtu = att_server_get_mtu(s_con_handle);
    return mtu > 3 ? mtu - 3 : 0;
}

bool bt_config_send_event(const uint8_t *data, uint16_t len)
{
    BtStackLock lock;
    const EventTarget &target = s_event_targets[s_active_service];
    if (!target.value_handle || s_con_handle == HCI_CON_HANDLE_INVALID)
    {
        return false;
    }
    return att_server_notify(s_con_handle, target.value_handle, data, len) == ERROR_CODE_SUCCESS;
}
