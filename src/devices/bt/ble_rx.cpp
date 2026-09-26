#include <btstack_tlv.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/le_device_db.h"
#include "btstack.h"
#include "btstack_config.h"
#include "gap.h"
#include "hidparser.h"
#include "hci_dump_embedded_stdout.h"
#include "devices/bt/bluetooth_stack.hpp"

#include "devices/bt/bt_host.hpp"
#include "devices/bt/bt_ble_host.hpp"
#include "emulation/usb/usb_devices.h"
#include "managers/device_manager.hpp"
#include "config/device_factory.hpp"
#include "devices/bluetooth.hpp"

// ---------------------------------------------------------------------------
// Per-Connection State
// ---------------------------------------------------------------------------

#define SIZE_OF_BD_ADDRESS 18
#define MAX_DEVICES_TO_SCAN 10

static hid_protocol_mode_t protocol_mode = HID_PROTOCOL_MODE_REPORT;
static uint8_t hid_descriptor_storage[500];

// Guitar Hero Live iOS BLE Guitar characteristic UUID: 533e1524-3abe-f33f-cd00-594e8b0a8ea3
static const uint8_t ghl_ios_char_uuid[16] = {
    0x53, 0x3E, 0x15, 0x24, 0x3A, 0xBE, 0xF3, 0x3F,
    0xCD, 0x00, 0x59, 0x4E, 0x8B, 0x0A, 0x8E, 0xA3};

// Valve Steam Controller BLE characteristic UUID: 100f6c34-1735-4313-b402-38567131e5f3
static const uint8_t steam_ble_char_uuid[16] = {
    0x10, 0x0F, 0x6C, 0x34, 0x17, 0x35, 0x43, 0x13,
    0xB4, 0x02, 0x38, 0x56, 0x71, 0x31, 0xE5, 0xF3};
#define MAX_BLE_CONNECTIONS 4
#ifndef MAX_BLE_RECONNECT_CANDIDATES
#define MAX_BLE_RECONNECT_CANDIDATES 8 // match/exceed NVM_NUM_DEVICE_DB_ENTRIES
#endif
struct BleConnectionContext
{
    bool in_use = false;
    bool pending_destroy = false;

    hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
    uint16_t hids_cid = 0;
    bd_addr_t addr = {};
    bd_addr_type_t addr_type = BD_ADDR_TYPE_LE_PUBLIC;
    uint16_t vid = 0;
    uint16_t pid = 0;
    uint16_t version = 0;
    uint16_t device_id = 0;
    bool is_ghl_guitar = false;
    bool found_ghl_char = false;
    bool is_steam_controller = false;
    bool found_steam_char = false;
    SubType known_subtype = SubType_Unknown;
    char paired_name[32] = {};
    gatt_client_characteristic_t ghl_characteristic = {};
    gatt_client_notification_t ghl_notification = {};
    gatt_client_characteristic_t steam_characteristic = {};
    gatt_client_notification_t steam_notification = {};

    bool pending_host_create = false;
    bool host_create_queued = false;
    uint16_t pending_hids_cid = 0;
    uint8_t desc_copy[512] = {};
    uint16_t desc_len = 0;

    std::shared_ptr<BluetoothHostInterface> host; // never touched by reset — see below
};

static BleConnectionContext s_context_pool[MAX_BLE_CONNECTIONS];

static BleConnectionContext *ble_context_alloc()
{
    for (auto &ctx : s_context_pool)
    {
        if (!ctx.in_use)
        {
            if (ctx.host)
            {
                printf("BUG: BLE context slot reused with host still set!\r\n");
            }

            ctx.pending_destroy = false;
            ctx.con_handle = HCI_CON_HANDLE_INVALID;
            ctx.hids_cid = 0;
            memset(ctx.addr, 0, sizeof(ctx.addr));
            ctx.addr_type = BD_ADDR_TYPE_LE_PUBLIC;
            ctx.vid = 0;
            ctx.pid = 0;
            ctx.version = 0;
            ctx.device_id = 0;
            ctx.is_ghl_guitar = false;
            ctx.found_ghl_char = false;
            ctx.is_steam_controller = false;
            ctx.found_steam_char = false;
            ctx.known_subtype = SubType_Unknown;
            ctx.paired_name[0] = '\0';
            memset(&ctx.ghl_characteristic, 0, sizeof(ctx.ghl_characteristic));
            memset(&ctx.ghl_notification, 0, sizeof(ctx.ghl_notification));
            memset(&ctx.steam_characteristic, 0, sizeof(ctx.steam_characteristic));
            memset(&ctx.steam_notification, 0, sizeof(ctx.steam_notification));

            ctx.pending_host_create = false;
            ctx.host_create_queued = false;
            ctx.pending_hids_cid = 0;
            ctx.desc_len = 0;

            ctx.in_use = true;
            return &ctx;
        }
    }
    printf("BLE connection pool exhausted\r\n");
    return nullptr;
}
static void ble_context_free(BleConnectionContext *ctx) { ctx->in_use = false; }

static BleConnectionContext *ble_context_by_handle(hci_con_handle_t handle)
{
    for (auto &ctx : s_context_pool)
        if (ctx.in_use && ctx.con_handle == handle)
            return &ctx;
    return nullptr;
}

static BleConnectionContext *ble_context_by_cid(uint16_t cid)
{
    for (auto &ctx : s_context_pool)
        if (ctx.in_use && ctx.hids_cid == cid)
            return &ctx;
    return nullptr;
}

// Scan result tracking
typedef struct
{
    bd_addr_t addr;
    bd_addr_type_t addr_type;
    char name_buffer[100];
} scan_data_t;

static scan_data_t scan_devices[MAX_DEVICES_TO_SCAN];
static uint8_t devices_found = 0;

// Central coordinator state
static bool s_ble_scanning = false;
static bool s_whitelist_active = false;
static bool s_direct_connect_pending = false;

static btstack_timer_source_t s_scan_timer;
static btstack_timer_source_t s_direct_connect_timer;

// BTstack event registrations
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bd_addr_type_t ble_resolve_addr_type(const bd_addr_t mac)
{
    int max_entries = le_device_db_max_count();
    for (int i = 0; i < max_entries; i++)
    {
        int db_addr_type = 0;
        bd_addr_t db_addr = {};
        le_device_db_info(i, &db_addr_type, db_addr, nullptr);
        if (bd_addr_cmp(db_addr, mac) == 0)
        {
            return (bd_addr_type_t)db_addr_type;
        }
    }
    // In BLE, static random addresses have the two MSBs set: 0b11xxxxxx (0xC0)
    if ((mac[0] & 0xC0) == 0xC0)
    {
        return BD_ADDR_TYPE_LE_RANDOM;
    }
    return BD_ADDR_TYPE_LE_PUBLIC;
}

static bool ble_is_mac_connected(const bd_addr_t mac)
{
    for (const auto &ctx : s_context_pool)
    {
        if (ctx.in_use && bd_addr_cmp(ctx.addr, mac) == 0)
        {
            return true;
        }
    }
    return false;
}

static void ble_sync_reconnect(void);

static void ble_direct_connect_timeout(btstack_timer_source_t *ts)
{
    UNUSED(ts);
    printf("BLE direct connect timed out\r\n");
    if (s_direct_connect_pending)
    {
        s_direct_connect_pending = false;
        gap_connect_cancel();
        ble_sync_reconnect();
    }
}

static void bt_stop_scan_timer(btstack_timer_source_t *ts)
{
    UNUSED(ts);
    printf("BLE scan timed out\r\n");
    gap_stop_scan();
    s_ble_scanning = false;
    ble_sync_reconnect();
}

void ble_stop_scan()
{
    btstack_run_loop_remove_timer(&s_scan_timer);
    gap_stop_scan();
    s_ble_scanning = false;
    ble_sync_reconnect();
}

void ble_start_scan()
{
    printf("Scanning for LE HID devices...\r\n");
    devices_found = 0;
    if (s_direct_connect_pending)
    {
        s_direct_connect_pending = false;
        btstack_run_loop_remove_timer(&s_direct_connect_timer);
        gap_connect_cancel();
    }
    if (s_whitelist_active)
    {
        s_whitelist_active = false;
        gap_connect_cancel();
    }
    s_ble_scanning = true;
    gap_set_scan_parameters(1, 48, 48);
    gap_start_scan();
    btstack_run_loop_set_timer(&s_scan_timer, 5000);
    btstack_run_loop_set_timer_handler(&s_scan_timer, &bt_stop_scan_timer);
    btstack_run_loop_add_timer(&s_scan_timer);
}

bool ble_is_connecting()
{
    if (s_ble_scanning || s_direct_connect_pending)
        return true;
    for (const auto &ctx : s_context_pool)
    {
        if (ctx.in_use && !ctx.host)
            return true;
    }
    return false;
}

bool ble_has_connected_device()
{
    for (const auto &ctx : s_context_pool)
    {
        if (ctx.in_use && ctx.host && ctx.host->is_ready())
            return true;
    }
    return false;
}

static void ble_sync_reconnect(void)
{
    if (s_ble_scanning || s_direct_connect_pending)
    {
        return;
    }

    struct Candidate
    {
        bd_addr_t addr;
        bd_addr_type_t addr_type;
    };
    static Candidate candidates[MAX_BLE_RECONNECT_CANDIDATES];
    uint8_t candidate_count = 0;

    auto candidate_exists = [&](const bd_addr_t addr)
    {
        for (uint8_t i = 0; i < candidate_count; i++)
            if (bd_addr_cmp(candidates[i].addr, addr) == 0)
                return true;
        return false;
    };

    auto try_add_candidate = [&](const bd_addr_t addr, bd_addr_type_t addr_type)
    {
        if (candidate_count >= MAX_BLE_RECONNECT_CANDIDATES)
            return;
        if (candidate_exists(addr))
            return;
        bd_addr_copy(candidates[candidate_count].addr, addr);
        candidates[candidate_count].addr_type = addr_type;
        candidate_count++;
    };

    // 1. Check paired BLE devices from DeviceFactory
    DeviceFactory::foreach_bluetooth_pairing_state([&](int32_t id, const DeviceFactory::BluetoothPairingStateData &state)
                                                   {
        UNUSED(id);
        if (state.ble && !btstack_is_null_bd_addr(state.mac) && !ble_is_mac_connected(state.mac))
        {
            try_add_candidate(state.mac, ble_resolve_addr_type(state.mac));
        } });

    // 2. Check le_device_db as fallback
    int max_entries = le_device_db_max_count();
    for (int i = 0; i < max_entries; i++)
    {
        int db_addr_type = 0;
        bd_addr_t db_addr = {};
        le_device_db_info(i, &db_addr_type, db_addr, nullptr);
        if (!btstack_is_null_bd_addr(db_addr) && !ble_is_mac_connected(db_addr))
        {
            try_add_candidate(db_addr, (bd_addr_type_t)db_addr_type);
        }
    }

    if (candidate_count == 0)
    {
        if (s_whitelist_active)
        {
            gap_connect_cancel();
            s_whitelist_active = false;
        }
        return;
    }

    gap_whitelist_clear();
    for (uint8_t i = 0; i < candidate_count; i++)
    {
        const auto &c = candidates[i];
        printf("BLE auto-reconnect candidate: %s (type %d)\r\n", bd_addr_to_str(c.addr), c.addr_type);
        gap_whitelist_add(c.addr_type, c.addr);
    }

    if (!s_whitelist_active)
    {
        uint8_t status = gap_connect_with_whitelist();
        printf("gap_connect_with_whitelist status: 0x%02x\r\n", status);
        if (status == ERROR_CODE_SUCCESS)
        {
            s_whitelist_active = true;
        }
    }
}

static void handle_outgoing_connection_error(hci_con_handle_t con_handle)
{
    printf("Error occurred for connection 0x%04x, disconnecting\r\n", con_handle);
    gap_disconnect(con_handle);
}

static void ble_register_hids_host(BleConnectionContext *ctx, uint16_t cid)
{
    if (!ctx || ctx->host)
        return;

    HID_ReportInfo_t *info = nullptr;
    if (ctx->desc_len > 0)
        USB_ProcessHIDReport(ctx->desc_copy, ctx->desc_len, &info);

    uint16_t device_id = BluetoothStack::instance().device_id();
    auto host = ble_create_host(ctx->vid, ctx->pid, ctx->version, device_id, info, ctx->desc_copy, ctx->desc_len, ctx->known_subtype);
    host->set_ble(true);

    memcpy(host->m_addr, ctx->addr, 6);
    host->m_addr_type = ctx->addr_type;
    host->m_cid = cid;
    if (ctx->paired_name[0] && !host->m_name[0])
        strncpy(host->m_name, ctx->paired_name, sizeof(host->m_name) - 1);

    ctx->host = host;
    host->on_connected();
    bt_host_add_interface(host);
    bt_host_save_pairing(host, true);
    printf("BLE host registered and interface added: VID=0x%04x PID=0x%04x name='%s'\r\n",
           ctx->vid, ctx->pid, host->m_name);
}

// ---------------------------------------------------------------------------
// GATT client events — called for HID service + device information
// ---------------------------------------------------------------------------

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size)
{
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint8_t status = 0;
    uint8_t packet_type_code = hci_event_packet_get_type(packet);

    if (packet_type_code == GATT_EVENT_NOTIFICATION)
    {
        hci_con_handle_t handle = gatt_event_notification_get_handle(packet);
        auto ctx = ble_context_by_handle(handle);
        if (ctx && ctx->host)
        {
            ctx->host->handle_report(
                gatt_event_notification_get_value(packet),
                gatt_event_notification_get_value_length(packet));
        }
        return;
    }
    else if (packet_type_code == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT)
    {
        hci_con_handle_t handle = gatt_event_characteristic_query_result_get_handle(packet);
        auto ctx = ble_context_by_handle(handle);
        if (ctx)
        {
            if (ctx->is_steam_controller)
            {
                gatt_event_characteristic_query_result_get_characteristic(packet, &ctx->steam_characteristic);
                ctx->found_steam_char = true;
                printf("Steam Controller characteristic found for handle 0x%04x, value_handle=0x%04x\r\n",
                       handle, ctx->steam_characteristic.value_handle);
            }
            else
            {
                gatt_event_characteristic_query_result_get_characteristic(packet, &ctx->ghl_characteristic);
                ctx->found_ghl_char = true;
                printf("GHL iOS Guitar characteristic found for handle 0x%04x, value_handle=0x%04x\r\n",
                       handle, ctx->ghl_characteristic.value_handle);
            }
        }
        return;
    }
    else if (packet_type_code == GATT_EVENT_QUERY_COMPLETE)
    {
        hci_con_handle_t handle = gatt_event_query_complete_get_handle(packet);
        auto ctx = ble_context_by_handle(handle);
        if (!ctx)
            return;

        if (ctx->found_steam_char)
        {
            printf("Steam Controller query complete, subscribing to notifications for handle 0x%04x\r\n", handle);
            gatt_client_listen_for_characteristic_value_updates(
                &ctx->steam_notification,
                handle_gatt_client_event,
                handle,
                &ctx->steam_characteristic);

            gatt_client_write_client_characteristic_configuration(
                handle_gatt_client_event,
                handle,
                &ctx->steam_characteristic,
                GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);

            uint16_t device_id = BluetoothStack::instance().device_id();
            auto host = std::make_shared<BleSteamHost>(device_id);
            memcpy(host->m_addr, ctx->addr, 6);
            host->m_addr_type = ctx->addr_type;
            host->m_cid = handle;
            host->m_con_handle = handle;
            host->m_char_handle = ctx->steam_characteristic.value_handle;
            host->m_vid = VALVE_USB_VID;
            host->m_pid = VALVE_STEAM_CONTROLLER_BLE_PID;
            strncpy(host->m_name, "Steam Controller", sizeof(host->m_name) - 1);

            ctx->host = host;
            host->on_connected();
            bt_host_add_interface(host);
            bt_host_save_pairing(host, true);
        }
        else if (ctx->found_ghl_char)
        {
            printf("GHL iOS query complete, subscribing to notifications for handle 0x%04x\r\n", handle);
            gatt_client_listen_for_characteristic_value_updates(
                &ctx->ghl_notification,
                handle_gatt_client_event,
                handle,
                &ctx->ghl_characteristic);

            gatt_client_write_client_characteristic_configuration(
                handle_gatt_client_event,
                handle,
                &ctx->ghl_characteristic,
                GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);

            uint16_t device_id = BluetoothStack::instance().device_id();
            auto host = std::make_shared<BleGhlIosHost>(device_id);
            memcpy(host->m_addr, ctx->addr, 6);
            host->m_addr_type = ctx->addr_type;
            host->m_cid = handle;
            strncpy(host->m_name, "GHL iOS Guitar", sizeof(host->m_name) - 1);

            ctx->host = host;
            host->on_connected();
            bt_host_add_interface(host);
            bt_host_save_pairing(host, true);
        }
        else
        {
            handle_outgoing_connection_error(handle);
        }
        return;
    }

    if (packet_type_code != HCI_EVENT_GATTSERVICE_META)
        return;

    switch (hci_event_gattservice_meta_get_subevent_code(packet))
    {
    case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED:
    {
        status = gattservice_subevent_hid_service_connected_get_status(packet);
        uint16_t cid = gattservice_subevent_hid_service_connected_get_hids_cid(packet);
        auto ctx = ble_context_by_cid(cid);
        if (!ctx)
            break;

        if (status == ERROR_CODE_SUCCESS)
        {
            printf("HID service connected, cid=0x%04x, instances=%d\r\n", cid,
                   gattservice_subevent_hid_service_connected_get_num_instances(packet));

            if (!ctx->host && !ctx->host_create_queued)
            {
                const uint8_t *desc = hids_host_descriptor_storage_get_descriptor_data(cid, 0);
                uint16_t len = hids_host_descriptor_storage_get_descriptor_len(cid, 0);
                if (desc && len > 0 && len <= sizeof(ctx->desc_copy))
                {
                    memcpy(ctx->desc_copy, desc, len);
                    ctx->desc_len = len;
                }
                else
                {
                    ctx->desc_len = 0; // oversized or missing — create with no descriptor rather than truncate silently
                }
                ctx->pending_hids_cid = cid;
                ctx->pending_host_create = true;
                ctx->host_create_queued = true;
            }
            hids_host_get_hid_information(cid, 0);
        }
        else
        {
            printf("HID service connection failed (err 0x%02x) - trying direct characteristic query\r\n", status);
            if (ctx->is_steam_controller)
            {
                gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                    handle_gatt_client_event,
                    ctx->con_handle,
                    0x0001,
                    0xffff,
                    steam_ble_char_uuid);
            }
            else
            {
                ctx->is_ghl_guitar = true;
                gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                    handle_gatt_client_event,
                    ctx->con_handle,
                    0x0001,
                    0xffff,
                    ghl_ios_char_uuid);
            }
        }
        break;
    }

    case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_PNP_ID:
    {
        status = gattservice_subevent_device_information_pnp_id_get_att_status(packet);
        if (status != ATT_ERROR_SUCCESS)
        {
            printf("PNP ID read failed, ATT Error 0x%02x\n", status);
            break;
        }
        hci_con_handle_t handle = gattservice_subevent_device_information_pnp_id_get_con_handle(packet);
        auto ctx = ble_context_by_handle(handle);
        if (ctx)
        {
            ctx->vid = gattservice_subevent_device_information_pnp_id_get_vendor_id(packet);
            ctx->pid = gattservice_subevent_device_information_pnp_id_get_product_id(packet);
            ctx->version = gattservice_subevent_device_information_pnp_id_get_product_version(packet);
            printf("BLE PnP (handle 0x%04x): VID=0x%04x PID=0x%04x\r\n", handle, ctx->vid, ctx->pid);
        }
        break;
    }

    case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_DONE:
    {
        hci_con_handle_t handle = gattservice_subevent_device_information_done_get_con_handle(packet);
        auto ctx = ble_context_by_handle(handle);
        if (ctx)
        {
            printf("Device info done for handle 0x%04x\r\n", handle);
            uint16_t hids_cid = 0;
            uint8_t err = hids_host_connect(handle, handle_gatt_client_event, protocol_mode, &hids_cid);
            if (err != ERROR_CODE_SUCCESS)
            {
                printf("hids_host_connect failed, err=0x%02x\r\n", err);
            }
            else
            {
                ctx->hids_cid = hids_cid;
            }
        }
        break;
    }

    case GATTSERVICE_SUBEVENT_HID_INFORMATION:
    {
        uint16_t cid = gattservice_subevent_hid_information_get_hids_cid(packet);
        auto ctx = ble_context_by_cid(cid);
        if (ctx && !ctx->host && !ctx->host_create_queued)
        {
            const uint8_t *desc = hids_host_descriptor_storage_get_descriptor_data(cid, 0);
            uint16_t len = hids_host_descriptor_storage_get_descriptor_len(cid, 0);
            if (desc && len > 0 && len <= sizeof(ctx->desc_copy))
            {
                memcpy(ctx->desc_copy, desc, len);
                ctx->desc_len = len;
            }
            else
            {
                ctx->desc_len = 0; // oversized or missing — create with no descriptor rather than truncate silently
            }
            ctx->pending_hids_cid = cid;
            ctx->pending_host_create = true;
            ctx->host_create_queued = true;
        }
        break;
    }

    case GATTSERVICE_SUBEVENT_HID_REPORT:
    {
        uint16_t cid = gattservice_subevent_hid_report_get_hids_cid(packet);
        auto ctx = ble_context_by_cid(cid);
        if (ctx && ctx->host)
        {
            auto host = ctx->host;
            host->handle_report(
                gattservice_subevent_hid_report_get_report(packet),
                gattservice_subevent_hid_report_get_report_len(packet));
            if (!host->is_registered() && host->is_ready())
            {
                bt_host_promote_if_ready(host);
                bt_host_save_pairing(host, true);
            }
        }
        break;
    }

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// HCI / GAP packet handler
// ---------------------------------------------------------------------------

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);
    uint8_t event;

    switch (packet_type)
    {
    case HCI_EVENT_PACKET:
        event = hci_event_packet_get_type(packet);
        switch (event)
        {
        case BTSTACK_EVENT_STATE:
        {
            uint8_t state = btstack_event_state_get_state(packet);
            printf("BLE BTstack state: %d (WORKING=%d)\r\n", state, HCI_STATE_WORKING);
            if (state != HCI_STATE_WORKING)
                break;
            ble_sync_reconnect();
            break;
        }

        case GAP_EVENT_ADVERTISING_REPORT:
        {
            if (!s_ble_scanning)
                break;

            bd_addr_t address;
            gap_event_advertising_report_get_address(packet, address);
            bd_addr_type_t addr_type = (bd_addr_type_t)gap_event_advertising_report_get_address_type(packet);
            const uint8_t *adv_data = gap_event_advertising_report_get_data(packet);
            uint8_t adv_size = gap_event_advertising_report_get_data_length(packet);
            ad_context_t context;

            bool is_hid = false;
            char dev_name[100] = {};

            for (ad_iterator_init(&context, adv_size, adv_data);
                 ad_iterator_has_more(&context); ad_iterator_next(&context))
            {
                uint8_t data_type = ad_iterator_get_data_type(&context);
                uint8_t data_len = ad_iterator_get_data_len(&context);
                const uint8_t *data = ad_iterator_get_data(&context);

                switch (data_type)
                {
                case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                    for (uint8_t i = 0u; (i + 2u) <= data_len; i += 2u)
                    {
                        uint16_t uuid = (uint16_t)little_endian_read_16(data, (int)i);
                        if (uuid == ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE)
                        {
                            is_hid = true;
                        }
                    }
                    break;
                case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
                case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
                    for (uint8_t i = 0u; (i + 16u) <= data_len; i += 16u)
                    {
                        // Check for Steam Controller BLE service (little-endian wire format)
                        static const uint8_t steam_srv_le[16] = {
                            0xF3, 0xE5, 0x31, 0x71, 0x56, 0x38, 0x02, 0xB4,
                            0x13, 0x43, 0x35, 0x17, 0x32, 0x6C, 0x0F, 0x10};
                        if (memcmp(data + i, steam_srv_le, 16) == 0)
                        {
                            is_hid = true;
                        }
                    }
                    break;
                case BLUETOOTH_DATA_TYPE_APPEARANCE:
                {
                    uint16_t appearance = little_endian_read_16(data, 0);
                    if (appearance == 0x03C4 /* Gamepad */ || appearance == 0x03C5 /* Joystick */ || appearance == 0x03C6 /* Simulation */)
                    {
                        is_hid = true;
                    }
                    break;
                }
                case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
                case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
                {
                    int copy = data_len < (int)sizeof(dev_name) - 1 ? data_len : (int)sizeof(dev_name) - 1;
                    memcpy(dev_name, data, copy);
                    dev_name[copy] = '\0';
                    if (strstr(dev_name, "Ble Guitar") != nullptr || strstr(dev_name, "8BitDo") != nullptr ||
                        strstr(dev_name, "Wireless") != nullptr || strstr(dev_name, "Gamepad") != nullptr ||
                        strstr(dev_name, "Controller") != nullptr || strstr(dev_name, "Steam") != nullptr ||
                        strstr(dev_name, "Switch") != nullptr || strstr(dev_name, "Joy-Con") != nullptr)
                    {
                        is_hid = true;
                    }
                    break;
                }
                default:
                    break;
                }
            }

            if (is_hid)
            {
                printf("Found BLE HID device: '%s' [%s] type %d\r\n", dev_name, bd_addr_to_str(address), addr_type);
                gap_stop_scan();
                btstack_run_loop_remove_timer(&s_scan_timer);
                s_ble_scanning = false;
                bt_discovery_on_device_found();

                devices_found = 1;
                memcpy(scan_devices[0].addr, address, sizeof(bd_addr_t));
                scan_devices[0].addr_type = addr_type;
                strncpy(scan_devices[0].name_buffer, dev_name[0] ? dev_name : "BLE HID Device", sizeof(scan_devices[0].name_buffer) - 1);

                printf("Connecting to BLE device %s (type %d)...\r\n", bd_addr_to_str(address), addr_type);
                s_direct_connect_pending = true;
                btstack_run_loop_set_timer(&s_direct_connect_timer, 10000);
                btstack_run_loop_set_timer_handler(&s_direct_connect_timer, ble_direct_connect_timeout);
                btstack_run_loop_add_timer(&s_direct_connect_timer);
                gap_connect(address, addr_type);
            }
            break;
        }

        case HCI_EVENT_DISCONNECTION_COMPLETE:
        {
            hci_con_handle_t handle = hci_event_disconnection_complete_get_connection_handle(packet);
            auto ctx = ble_context_by_handle(handle);
            if (ctx)
            {
                printf("BLE device disconnected (handle 0x%04x, addr %s)\r\n", handle, bd_addr_to_str(ctx->addr));
                if (ctx->hids_cid != 0)
                {
                    hids_host_disconnect(ctx->hids_cid);
                }
                ctx->pending_destroy = true;
                ble_sync_reconnect();
            }
            break;
        }

        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) != GAP_SUBEVENT_LE_CONNECTION_COMPLETE)
                break;

            if (s_direct_connect_pending)
            {
                s_direct_connect_pending = false;
                btstack_run_loop_remove_timer(&s_direct_connect_timer);
            }
            if (s_whitelist_active)
            {
                s_whitelist_active = false;
            }

            if (gap_subevent_le_connection_complete_get_status(packet) != ERROR_CODE_SUCCESS)
            {
                printf("LE connection failed (status 0x%02x)\r\n",
                       gap_subevent_le_connection_complete_get_status(packet));
                ble_sync_reconnect();
                return;
            }

            {
                hci_con_handle_t handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                bd_addr_t peer_addr;
                gap_subevent_le_connection_complete_get_peer_address(packet, peer_addr);
                bd_addr_type_t peer_addr_type = (bd_addr_type_t)gap_subevent_le_connection_complete_get_peer_address_type(packet);

                auto ctx = ble_context_alloc();
                ctx->con_handle = handle;
                bd_addr_copy(ctx->addr, peer_addr);
                ctx->addr_type = peer_addr_type;
                ctx->device_id = BluetoothStack::instance().device_id();

                DeviceFactory::BluetoothPairingStateData paired_state = {};
                if (DeviceFactory::find_bluetooth_pairing_state_by_mac(peer_addr, paired_state) && paired_state.ble)
                {
                    printf("BLE device is already paired: name='%s' type=%d vid=0x%04x pid=0x%04x subtype=%d\r\n",
                           paired_state.name, (int)paired_state.controller_type, paired_state.vid, paired_state.pid, (int)paired_state.subtype);
                    ctx->vid = paired_state.vid;
                    ctx->pid = paired_state.pid;
                    ctx->known_subtype = paired_state.subtype;
                    strncpy(ctx->paired_name, paired_state.name, sizeof(ctx->paired_name) - 1);
                    if (paired_state.controller_type == BtControllerType_BtControllerTypeGhlIos)
                    {
                        ctx->is_ghl_guitar = true;
                    }
                }

                // Check if the connecting device was scanned as GHL guitar
                for (int i = 0; i < devices_found; i++)
                {
                    if (bd_addr_cmp(peer_addr, scan_devices[i].addr) == 0)
                    {
                        memcpy(ctx->paired_name, scan_devices[i].name_buffer, sizeof(ctx->paired_name) - 1);
                        ctx->paired_name[sizeof(ctx->paired_name) - 1] = '\0';
                        if (strstr(scan_devices[i].name_buffer, "Ble Guitar") != nullptr)
                        {
                            ctx->is_ghl_guitar = true;
                            printf("Connecting device identified as GHL BLE Guitar\r\n");
                            break;
                        }
                        else if (strstr(scan_devices[i].name_buffer, "Steam") != nullptr)
                        {
                            ctx->is_steam_controller = true;
                            printf("Connecting device identified as Steam Controller\r\n");
                            break;
                        }
                    }
                }

                printf("BLE connection complete (handle 0x%04x) for %s (type %d)\r\n",
                       handle, bd_addr_to_str(peer_addr), peer_addr_type);
                sm_request_pairing(handle);
                ble_sync_reconnect();
            }
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet))
            {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
            {
                uint16_t conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
                printf("LE Connection Complete: interval %u.%02u ms\r\n",
                       conn_interval * 125 / 100, 25 * (conn_interval & 3));
                break;
            }
            default:
                break;
            }
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// SM (security manager) packet handler
// ---------------------------------------------------------------------------

static void sm_packet_handler(uint8_t packet_type, uint16_t channel,
                              uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET)
        return;

    switch (hci_event_packet_get_type(packet))
    {
    case SM_EVENT_JUST_WORKS_REQUEST:
        printf("Just works requested\r\n");
        sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
        break;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        printf("Confirming numeric comparison: %" PRIu32 "\r\n",
               sm_event_numeric_comparison_request_get_passkey(packet));
        sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
        break;
    case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
        printf("Display Passkey: %" PRIu32 "\r\n",
               sm_event_passkey_display_number_get_passkey(packet));
        break;
    case SM_EVENT_PAIRING_COMPLETE:
    {
        hci_con_handle_t handle = sm_event_pairing_complete_get_handle(packet);
        auto ctx = ble_context_by_handle(handle);
        if (!ctx)
            break;

        uint8_t status = sm_event_pairing_complete_get_status(packet);
        uint8_t reason = sm_event_pairing_complete_get_reason(packet);
        if (status == ERROR_CODE_SUCCESS || ctx->is_ghl_guitar || ctx->is_steam_controller || reason == SM_REASON_PAIRING_NOT_SUPPORTED)
        {
            printf("Pairing complete for handle 0x%04x (status=0x%02x, reason=0x%02x)\r\n", handle, status, reason);
            if (ctx->is_steam_controller)
            {
                printf("Connecting to Steam Controller GATT characteristic...\r\n");
                gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                    handle_gatt_client_event,
                    handle,
                    0x0001,
                    0xffff,
                    steam_ble_char_uuid);
            }
            else if (ctx->is_ghl_guitar)
            {
                printf("Connecting to iOS GHL guitar GATT characteristic...\r\n");
                gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                    handle_gatt_client_event,
                    handle,
                    0x0001,
                    0xffff,
                    ghl_ios_char_uuid);
            }
            else if (ctx->vid != 0)
            {
                printf("Paired BLE device: skipping DIS query, connecting HIDS directly\r\n");
                uint16_t hids_cid = 0;
                uint8_t err = hids_host_connect(handle, handle_gatt_client_event, protocol_mode, &hids_cid);
                if (err != ERROR_CODE_SUCCESS)
                {
                    printf("hids_host_connect failed, err=0x%02x\r\n", err);
                }
                else
                {
                    ctx->hids_cid = hids_cid;
                }
            }
            else
            {
                device_information_service_client_query(handle, handle_gatt_client_event);
            }
        }
        else
        {
            printf("Pairing failed for handle 0x%04x, status = 0x%02x, reason = 0x%02x\r\n", handle, status, reason);
            gap_disconnect(handle);
        }
        break;
    }
    case SM_EVENT_REENCRYPTION_COMPLETE:
    {
        hci_con_handle_t handle = sm_event_reencryption_complete_get_handle(packet);
        auto ctx = ble_context_by_handle(handle);
        if (!ctx)
            break;

        uint8_t status = sm_event_reencryption_complete_get_status(packet);
        if (status != ERROR_CODE_SUCCESS)
        {
            printf("Re-encryption failed for handle 0x%04x (status 0x%02x)\r\n", handle, status);
            if (status == ERROR_CODE_PIN_OR_KEY_MISSING)
            {
                printf("Bonding info missing on remote device, requesting new pairing...\r\n");
                gap_delete_bonding(ctx->addr_type, ctx->addr);
                sm_request_pairing(handle);
            }
            else
            {
                gap_disconnect(handle);
            }
            break;
        }

        printf("Re-encryption complete for handle 0x%04x\r\n", handle);
        if (ctx->is_steam_controller)
        {
            printf("Connecting to Steam Controller GATT characteristic...\r\n");
            gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                handle_gatt_client_event,
                handle,
                0x0001,
                0xffff,
                steam_ble_char_uuid);
        }
        else if (ctx->is_ghl_guitar)
        {
            gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                handle_gatt_client_event,
                handle,
                0x0001,
                0xffff,
                ghl_ios_char_uuid);
        }
        else if (ctx->vid != 0)
        {
            printf("Paired BLE device: skipping DIS query, connecting HIDS directly\r\n");
            uint16_t hids_cid = 0;
            uint8_t err = hids_host_connect(handle, handle_gatt_client_event, protocol_mode, &hids_cid);
            if (err != ERROR_CODE_SUCCESS)
            {
                printf("hids_host_connect failed, err=0x%02x\r\n", err);
            }
            else
            {
                ctx->hids_cid = hids_cid;
            }
        }
        else
        {
            device_information_service_client_query(handle, handle_gatt_client_event);
        }
        break;
    }
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int ble_main(void)
{
    printf("BLE init\r\n");

    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    sm_set_secure_connections_only_mode(false);
    gatt_client_init();

    hids_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    device_information_service_client_init();

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    return 0;
}
void ble_tick()
{
    for (auto &ctx : s_context_pool)
    {
        if (ctx.in_use && ctx.pending_host_create)
        {
            ctx.pending_host_create = false;
            ble_register_hids_host(&ctx, ctx.pending_hids_cid);
        }
        if (ctx.in_use && ctx.pending_destroy)
        {
            if (ctx.host)
            {
                ctx.host->on_disconnected();
                ctx.host.reset();
            }
            ctx.in_use = false;
        }
    }
}