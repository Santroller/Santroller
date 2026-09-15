
#include <btstack_tlv.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <unordered_map>

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "btstack.h"
#include "btstack_config.h"
#include "gap.h"
#include "hidparser.h"
#include "hci_dump_embedded_stdout.h"

#include "devices/bt/bt_host.hpp"
#include "devices/bt/bt_ble_host.hpp"
#include "managers/device_manager.hpp"
#include "config/device_factory.hpp"

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

typedef struct {
    bd_addr_t addr;
    bd_addr_type_t addr_type;
} le_device_addr_t;

#define SIZE_OF_BD_ADDRESS 18
#define MAX_DEVICES_TO_SCAN 10

static enum {
    W4_WORKING,
    W4_HID_DEVICE_FOUND,
    W4_CONNECTED,
    W4_ENCRYPTED,
    W4_HID_CLIENT_CONNECTED,
    READY,
    W4_TIMEOUT_THEN_SCAN,
    W4_TIMEOUT_THEN_RECONNECT,
} app_state;

static le_device_addr_t remote_device;
static hci_con_handle_t connection_handle;
static uint16_t hids_cid;
static hid_protocol_mode_t protocol_mode = HID_PROTOCOL_MODE_REPORT;

static uint8_t hid_descriptor_storage[500];

// Guitar Hero Live iOS BLE Guitar characteristic UUID: 533e1524-3abe-f33f-cd00-594e8b0a8ea3
static const uint8_t ghl_ios_char_uuid[16] = {
    0x53, 0x3E, 0x15, 0x24, 0x3A, 0xBE, 0xF3, 0x3F,
    0xCD, 0x00, 0x59, 0x4E, 0x8B, 0x0A, 0x8E, 0xA3
};

// Per-connection pending PnP info (connection_handle → pending state)
struct BlePendingConnection {
    uint16_t vid = 0;
    uint16_t pid = 0;
    uint16_t version = 0;
    uint16_t device_id = 0;
    bool is_ghl_guitar = false;
    bool found_ghl_char = false;
    SubType known_subtype = SubType_Unknown;
    char paired_name[32] = {};
    gatt_client_characteristic_t ghl_characteristic = {};
    gatt_client_notification_t ghl_notification = {};
};
static std::unordered_map<hci_con_handle_t, BlePendingConnection> ble_pending;

// Active connections: hids_cid → host object
static std::unordered_map<uint16_t, std::shared_ptr<BluetoothHostInterface>> ble_connections;

static uint16_t next_ble_device_id = 0x5000;

// Scan result tracking
typedef struct {
    char addr[SIZE_OF_BD_ADDRESS];
    char name_buffer[100];
} scan_data_t;

static scan_data_t scan_devices[MAX_DEVICES_TO_SCAN];
static uint8_t devices_found;

// Timers
static btstack_timer_source_t connection_timer;
static btstack_timer_source_t reconnect_timer;
static btstack_timer_source_t scan_timer;

// BTstack event registrations
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool adv_event_contains_hid_service(const uint8_t *packet)
{
    const uint8_t *ad_data = gap_event_advertising_report_get_data(packet);
    uint8_t ad_len = gap_event_advertising_report_get_data_length(packet);
    return ad_data_contains_uuid16(ad_len, ad_data, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE);
}

void ble_stop_scan()
{
    gap_stop_scan();
}

static void bt_stop_scan_timer(btstack_timer_source_t *ts)
{
    UNUSED(ts);
    gap_stop_scan();
}

void ble_start_scan()
{
    printf("Scanning for LE HID devices...\r\n");
    devices_found = 0;
    gap_set_scan_parameters(1, 48, 48);
    gap_start_scan();
    btstack_run_loop_set_timer(&scan_timer, 10000);
    btstack_run_loop_set_timer_handler(&scan_timer, &bt_stop_scan_timer);
    btstack_run_loop_add_timer(&scan_timer);
}

static void hog_connect(void);

static void hog_reconnect_timeout(btstack_timer_source_t *ts)
{
    UNUSED(ts);
    hog_connect();
}

static void hog_connection_timeout(btstack_timer_source_t *ts)
{
    UNUSED(ts);
    printf("Timeout - abort connection\r\n");
    gap_connect_cancel();
    btstack_run_loop_set_timer(&reconnect_timer, 100);
    btstack_run_loop_set_timer_handler(&reconnect_timer, &hog_reconnect_timeout);
    btstack_run_loop_add_timer(&reconnect_timer);
}

static void hog_connect(void)
{
    btstack_run_loop_set_timer(&connection_timer, 10000);
    btstack_run_loop_set_timer_handler(&connection_timer, &hog_connection_timeout);
    btstack_run_loop_add_timer(&connection_timer);
    app_state = W4_CONNECTED;
    gap_connect(remote_device.addr, remote_device.addr_type);
}

static void hog_start_reconnect_timer()
{
    btstack_run_loop_set_timer(&connection_timer, 100);
    btstack_run_loop_set_timer_handler(&connection_timer, &hog_reconnect_timeout);
    btstack_run_loop_add_timer(&connection_timer);
}

static void hog_start_connect(void)
{
    // If we have a stored address, reconnect; otherwise wait for scan
}

static void handle_outgoing_connection_error(void)
{
    printf("Error occurred, disconnect and start over\r\n");
    gap_disconnect(connection_handle);
    hog_start_reconnect_timer();
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
        auto it = ble_connections.find(handle);
        if (it != ble_connections.end())
        {
            it->second->handle_report(
                gatt_event_notification_get_value(packet),
                gatt_event_notification_get_value_length(packet));
        }
        return;
    }
    else if (packet_type_code == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT)
    {
        auto &pending = ble_pending[connection_handle];
        gatt_client_characteristic_t characteristic;
        gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
        pending.ghl_characteristic = characteristic;
        pending.found_ghl_char = true;
        printf("GHL iOS Guitar characteristic found, value_handle=0x%04x\r\n", characteristic.value_handle);
        return;
    }
    else if (packet_type_code == GATT_EVENT_QUERY_COMPLETE)
    {
        auto &pending = ble_pending[connection_handle];
        if (pending.found_ghl_char)
        {
            printf("GHL iOS Guitar query complete, subscribing to notifications\r\n");
            gatt_client_listen_for_characteristic_value_updates(
                &pending.ghl_notification,
                handle_gatt_client_event,
                connection_handle,
                &pending.ghl_characteristic);

            gatt_client_write_client_characteristic_configuration(
                handle_gatt_client_event,
                connection_handle,
                &pending.ghl_characteristic,
                GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);

            uint16_t device_id = pending.device_id ? pending.device_id : next_ble_device_id++;
            auto host = std::make_shared<BleGhlIosHost>(device_id);
            memcpy(host->m_addr, remote_device.addr, 6);
            host->m_addr_type = remote_device.addr_type;
            host->m_cid       = connection_handle;
            strncpy(host->m_name, "Ble Guitar", sizeof(host->m_name) - 1);

            ble_connections[connection_handle] = host;
            host->on_connected();
            bt_host_add_interface(host);
            bt_host_save_pairing(host, true);
            app_state = READY;
        }
        else
        {
            printf("GHL iOS characteristic not found\r\n");
            handle_outgoing_connection_error();
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
        if (status == ERROR_CODE_SUCCESS)
        {
            printf("HID service connected, cid=0x%04x, instances=%d\r\n", cid,
                   gattservice_subevent_hid_service_connected_get_num_instances(packet));

            hids_host_get_hid_information(cid, 0);
            app_state = READY;
        }
        else
        {
            printf("HID service connection failed (err 0x%02x) - trying GHL iOS characteristic query\r\n", status);
            auto &pending = ble_pending[connection_handle];
            pending.is_ghl_guitar = true;
            gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                handle_gatt_client_event,
                connection_handle,
                0x0001,
                0xffff,
                ghl_ios_char_uuid);
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
        auto &pending = ble_pending[connection_handle];
        pending.vid     = gattservice_subevent_device_information_pnp_id_get_vendor_id(packet);
        pending.pid     = gattservice_subevent_device_information_pnp_id_get_product_id(packet);
        pending.version = gattservice_subevent_device_information_pnp_id_get_product_version(packet);
        printf("BLE PnP: VID=0x%04x PID=0x%04x\r\n", pending.vid, pending.pid);
        break;
    }

    case GATTSERVICE_SUBEVENT_DEVICE_INFORMATION_DONE:
        printf("Device info done\r\n");
        hids_host_connect(connection_handle, handle_gatt_client_event, protocol_mode, &hids_cid);
        break;

    case GATTSERVICE_SUBEVENT_HID_INFORMATION:
    {
        // Once we have HID info, build the host object using PnP VID/PID
        auto &pending = ble_pending[connection_handle];
        if (ble_connections.find(hids_cid) == ble_connections.end())
        {
            // Parse the HID descriptor for 3rd-party detection
            HID_ReportInfo_t *info = nullptr;
            const uint8_t *desc = hids_host_descriptor_storage_get_descriptor_data(hids_cid, 0);
            uint16_t desc_len   = hids_host_descriptor_storage_get_descriptor_len(hids_cid, 0);
            if (desc && desc_len > 0)
                USB_ProcessHIDReport(desc, desc_len, &info);

            uint16_t device_id = pending.device_id ? pending.device_id : next_ble_device_id++;
            auto host = ble_create_host(pending.vid, pending.pid, pending.version, device_id, info, desc, desc_len, pending.known_subtype);

            memcpy(host->m_addr, remote_device.addr, 6);
            host->m_addr_type = remote_device.addr_type;
            host->m_cid       = hids_cid;
            if (pending.paired_name[0] && !host->m_name[0])
                strncpy(host->m_name, pending.paired_name, sizeof(host->m_name) - 1);

            ble_connections[hids_cid] = host;
            host->on_connected();
            bt_host_add_interface(host);
            bt_host_save_pairing(host, true);
        }
        break;
    }

    case GATTSERVICE_SUBEVENT_HID_REPORT:
    {
        uint16_t cid = gattservice_subevent_hid_report_get_hids_cid(packet);
        auto it = ble_connections.find(cid);
        if (it != ble_connections.end())
        {
            it->second->handle_report(
                gattservice_subevent_hid_report_get_report(packet),
                gattservice_subevent_hid_report_get_report_len(packet));
            if (!it->second->is_registered() && it->second->is_ready())
            {
                bt_host_promote_if_ready(it->second);
                bt_host_save_pairing(it->second, true);
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
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING)
                break;
            btstack_assert(app_state == W4_WORKING);
            hog_start_connect();
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
        {
            bd_addr_t address;
            gap_event_advertising_report_get_address(packet, address);
            const uint8_t *adv_data = gap_event_advertising_report_get_data(packet);
            uint8_t adv_size = gap_event_advertising_report_get_data_length(packet);
            ad_context_t context;
            char *address_string = bd_addr_to_str(address);
            bool found = false;
            int current_device = devices_found;

            // Check if already in list
            for (int i = 0; i < devices_found; i++)
            {
                if (memcmp(address_string, scan_devices[i].addr, SIZE_OF_BD_ADDRESS) == 0)
                {
                    found = true;
                    current_device = i;
                }
            }

            for (ad_iterator_init(&context, adv_size, adv_data);
                 ad_iterator_has_more(&context); ad_iterator_next(&context))
            {
                uint8_t data_type = ad_iterator_get_data_type(&context);
                uint8_t data_len  = ad_iterator_get_data_len(&context);
                const uint8_t *data = ad_iterator_get_data(&context);
                uint8_t i;

                switch (data_type)
                {
                case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
                    for (i = 0u; (i + 2u) <= data_len; i += 2u)
                    {
                        uint16_t uuid = (uint16_t)little_endian_read_16(data, (int)i);
                        if (uuid == ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE && !found && devices_found < MAX_DEVICES_TO_SCAN)
                        {
                            memcpy(scan_devices[devices_found].addr, address_string, SIZE_OF_BD_ADDRESS);
                            snprintf(scan_devices[devices_found].name_buffer,
                                     sizeof(scan_devices[devices_found].name_buffer),
                                     " (%s)", address_string);
                            found = true;
                            current_device = devices_found++;
                            printf("found BLE HID device: %s\n", scan_devices[current_device].name_buffer);
                        }
                    }
                    break;
                case BLUETOOTH_DATA_TYPE_APPEARANCE:
                    if (little_endian_read_16(data, 0) == 0x03C4 && !found && devices_found < MAX_DEVICES_TO_SCAN)
                    {
                        memcpy(scan_devices[devices_found].addr, address_string, SIZE_OF_BD_ADDRESS);
                        snprintf(scan_devices[devices_found].name_buffer,
                                 sizeof(scan_devices[devices_found].name_buffer),
                                 " (%s)", address_string);
                        found = true;
                        current_device = devices_found++;
                    }
                    break;
                case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
                case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
                {
                    char temp_name[100];
                    int copy = data_len < (int)sizeof(temp_name) - 1 ? data_len : (int)sizeof(temp_name) - 1;
                    memcpy(temp_name, data, copy);
                    temp_name[copy] = '\0';

                    if (strstr(temp_name, "Ble Guitar") != nullptr && !found && devices_found < MAX_DEVICES_TO_SCAN)
                    {
                        memcpy(scan_devices[devices_found].addr, address_string, SIZE_OF_BD_ADDRESS);
                        strncpy(scan_devices[devices_found].name_buffer, temp_name, sizeof(scan_devices[devices_found].name_buffer) - 1);
                        found = true;
                        current_device = devices_found++;
                        printf("found BLE Guitar device: %s\n", scan_devices[current_device].name_buffer);
                    }
                    else if (found && current_device < MAX_DEVICES_TO_SCAN)
                    {
                        strncpy(scan_devices[current_device].name_buffer, temp_name, sizeof(scan_devices[current_device].name_buffer) - 1);
                        printf("BLE device name '%s'\r\n", scan_devices[current_device].name_buffer);
                    }
                    break;
                }
                default:
                    break;
                }
            }
            break;
        }

        case HCI_EVENT_DISCONNECTION_COMPLETE:
        {
            if (app_state != READY) break;

            hci_con_handle_t handle = hci_event_disconnection_complete_get_connection_handle(packet);
            auto it = ble_connections.find(handle);
            if (it != ble_connections.end())
            {
                it->second->on_disconnected();
                ble_connections.erase(it);
            }
            if (hids_cid != 0)
            {
                auto it_hids = ble_connections.find(hids_cid);
                if (it_hids != ble_connections.end())
                {
                    it_hids->second->on_disconnected();
                    ble_connections.erase(it_hids);
                }
                hids_host_disconnect(hids_cid);
                hids_cid = 0;
            }
            ble_pending.erase(handle);
            connection_handle = HCI_CON_HANDLE_INVALID;
            app_state = W4_TIMEOUT_THEN_RECONNECT;
            hog_start_reconnect_timer();
            break;
        }

        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) != GAP_SUBEVENT_LE_CONNECTION_COMPLETE)
                break;
            if (app_state != W4_CONNECTED) return;
            btstack_run_loop_remove_timer(&connection_timer);
            connection_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
            {
                auto &pending = ble_pending[connection_handle];
                pending.device_id = next_ble_device_id++;

                DeviceFactory::BluetoothPairingStateData paired_state = {};
                if (DeviceFactory::find_bluetooth_pairing_state_by_mac(remote_device.addr, paired_state) && paired_state.ble)
                {
                    printf("BLE device is already paired: name='%s' type=%d vid=0x%04x pid=0x%04x subtype=%d\r\n",
                           paired_state.name, (int)paired_state.controller_type, paired_state.vid, paired_state.pid, (int)paired_state.subtype);
                    pending.vid = paired_state.vid;
                    pending.pid = paired_state.pid;
                    pending.known_subtype = paired_state.subtype;
                    strncpy(pending.paired_name, paired_state.name, sizeof(pending.paired_name) - 1);
                    if (paired_state.controller_type == BtControllerType_BtControllerTypeGhlIos)
                    {
                        pending.is_ghl_guitar = true;
                    }
                }

                // Check if the connecting device was scanned as GHL guitar
                for (int i = 0; i < devices_found; i++)
                {
                    char *address_string = bd_addr_to_str(remote_device.addr);
                    if (memcmp(address_string, scan_devices[i].addr, SIZE_OF_BD_ADDRESS) == 0)
                    {
                        if (strstr(scan_devices[i].name_buffer, "Ble Guitar") != nullptr)
                        {
                            pending.is_ghl_guitar = true;
                            printf("Connecting device identified as GHL BLE Guitar\r\n");
                            break;
                        }
                    }
                }
            }
            app_state = W4_ENCRYPTED;
            sm_request_pairing(connection_handle);
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

    if (packet_type != HCI_EVENT_PACKET) return;

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
        uint8_t status = sm_event_pairing_complete_get_status(packet);
        auto &pending = ble_pending[connection_handle];
        if (status == ERROR_CODE_SUCCESS || pending.is_ghl_guitar)
        {
            printf("Pairing complete (or GHL guitar without pairing), success=0x%02x\r\n", status);
            app_state = W4_HID_CLIENT_CONNECTED;
            if (pending.is_ghl_guitar)
            {
                printf("Connecting to iOS GHL guitar GATT characteristic...\r\n");
                gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                    handle_gatt_client_event,
                    connection_handle,
                    0x0001,
                    0xffff,
                    ghl_ios_char_uuid);
            }
            else if (pending.vid != 0)
            {
                printf("Paired BLE device: skipping DIS query, connecting HIDS directly\r\n");
                hids_host_connect(connection_handle, handle_gatt_client_event, protocol_mode, &hids_cid);
            }
            else
            {
                device_information_service_client_query(connection_handle, handle_gatt_client_event);
            }
        }
        else
        {
            printf("Pairing failed, status = 0x%02x\r\n", status);
            hog_start_reconnect_timer();
        }
        break;
    }
    case SM_EVENT_REENCRYPTION_COMPLETE:
    {
        printf("Re-encryption complete\n");
        app_state = W4_HID_CLIENT_CONNECTED;
        auto &pending = ble_pending[connection_handle];
        if (pending.is_ghl_guitar)
        {
            gatt_client_discover_characteristics_for_handle_range_by_uuid128(
                handle_gatt_client_event,
                connection_handle,
                0x0001,
                0xffff,
                ghl_ios_char_uuid);
        }
        else if (pending.vid != 0)
        {
            printf("Paired BLE device: skipping DIS query, connecting HIDS directly\r\n");
            hids_host_connect(connection_handle, handle_gatt_client_event, protocol_mode, &hids_cid);
        }
        else
        {
            device_information_service_client_query(connection_handle, handle_gatt_client_event);
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

    sm_set_io_capabilities(IO_CAPABILITY_DISPLAY_ONLY);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    gatt_client_init();

    hids_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    device_information_service_client_init();

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);

    app_state = W4_WORKING;
    return 0;
}
