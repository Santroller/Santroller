
#define MAX_ATTRIBUTE_VALUE_SIZE 300
#define INQUIRY_INTERVAL 5
#define PERIPHERAL_COD ((1 << 8) | (1 << 10))
#define SIZE_OF_BD_ADDRESS 18
#define MAX_DEVICES_TO_SCAN 10

#include <inttypes.h>
#include <stdio.h>
#include <memory>
#include <unordered_map>

#include "btstack.h"
#include "btstack_config.h"
#include "hidparser.h"
#include "devices/bt/bt_host.hpp"
#include "devices/bt/bt_classic_host.hpp"
#include "managers/device_manager.hpp"
#include "config/device_factory.hpp"
#include "devices/bluetooth.hpp"


// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------

// Forward declaration needed by hid_host_setup()
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static bool has_address = false;
static bd_addr_t remote_addr;

static btstack_packet_callback_registration_t hci_event_callback_registration;

// SDP
static uint8_t hid_descriptor_storage[500];

// App
typedef enum {
    APP_IDLE,
    APP_CONNECTED,
} app_state_t;

static app_state_t app_state = APP_IDLE;

#define MAX_DEVICES 20
enum DEVICE_STATE { REMOTE_NAME_REQUEST, REMOTE_NAME_INQUIRED, REMOTE_NAME_FETCHED };
struct device {
    bd_addr_t address;
    uint8_t pageScanRepetitionMode;
    uint16_t clockOffset;
    uint32_t cod;
    char name_buffer[240 + 2 + SIZE_OF_BD_ADDRESS];
    enum DEVICE_STATE state;
};
static struct device devices[MAX_DEVICES];
static int deviceCount = 0;

// --- Multiple-connection tracking -------------------------------------------
// Maps hid_host_cid → BluetoothHostInterface
static std::unordered_map<uint16_t, std::shared_ptr<BluetoothHostInterface>> bt_connections;

// Per-connection pending state (before the host object is created)
struct PendingConnection {
    uint16_t vid = 0;
    uint16_t pid = 0;
    uint16_t version = 0;
    bool     descriptor_available = false;
    bool     hid_descriptor_parsed = false;
    HID_ReportInfo_t *info = nullptr;
    bd_addr_t addr = {};
    // device_id assigned when SDP query completes (before HID connect)
    uint16_t device_id = 0;
};
static std::unordered_map<uint16_t, PendingConnection> pending_connections;

// Running counter for assigning unique BT device IDs
static uint16_t next_bt_device_id = 0x4000; // well above USB range

static uint16_t          hid_host_cid = 0;
static bool               hid_host_connection_pending = false;
static hid_protocol_mode_t hid_host_report_mode = HID_PROTOCOL_MODE_REPORT;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int getDeviceIndexForAddress(bd_addr_t addr)
{
    for (int j = 0; j < deviceCount; j++)
    {
        if (bd_addr_cmp(addr, devices[j].address) == 0)
            return j;
    }
    return -1;
}

static void hid_host_setup(void)
{
#ifdef ENABLE_BLE
    sm_init();
#endif
    hid_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    hid_host_register_packet_handler(packet_handler);

    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE |
                                         LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
    hci_set_master_slave_policy(HCI_ROLE_MASTER);

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    setvbuf(stdin, NULL, _IONBF, 0);
}

void btc_start_scan(uint32_t lap)
{
    printf("Starting inquiry scan (LAP 0x%06lx)..\r\n", (unsigned long)lap);
    deviceCount = 0;
    gap_inquiry_set_lap(lap);
    gap_inquiry_start(INQUIRY_INTERVAL);
}

void btc_stop_scan(void)
{
    printf("Stopping inquiry scan..\r\n");
    gap_inquiry_stop();
}

static int has_more_remote_name_requests(void)
{
    for (int i = 0; i < deviceCount; i++)
        if (devices[i].state == REMOTE_NAME_REQUEST) return 1;
    return 0;
}

static void do_next_remote_name_request(void)
{
    for (int i = 0; i < deviceCount; i++)
    {
        if (devices[i].state == REMOTE_NAME_REQUEST)
        {
            devices[i].state = REMOTE_NAME_INQUIRED;
            printf("Get remote name of %s...\r\n", bd_addr_to_str(devices[i].address));
            gap_remote_name_request(devices[i].address,
                                    devices[i].pageScanRepetitionMode,
                                    devices[i].clockOffset | 0x8000);
            return;
        }
    }
}

static void continue_remote_names(void)
{
    if (has_more_remote_name_requests())
        do_next_remote_name_request();
}

// ---------------------------------------------------------------------------
// SDP query result — extracts VID/PID for the pending connection
// ---------------------------------------------------------------------------
static uint8_t attribute_value[1000];

static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel,
                                           uint8_t *packet, uint16_t size)
{
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    // We use the currently-pending hid_host_cid as the key
    PendingConnection &pending = pending_connections[hid_host_cid];

    switch (hci_event_packet_get_type(packet))
    {
    case SDP_EVENT_QUERY_ATTRIBUTE_VALUE:
        attribute_value[sdp_event_query_attribute_byte_get_data_offset(packet)] =
            sdp_event_query_attribute_byte_get_data(packet);

        if ((uint16_t)(sdp_event_query_attribute_byte_get_data_offset(packet) + 1) ==
            sdp_event_query_attribute_byte_get_attribute_length(packet))
        {
            unsigned int pos = de_get_header_size(attribute_value);
            uint16_t attr_id = sdp_event_query_attribute_byte_get_attribute_id(packet);
            if (attr_id == BLUETOOTH_ATTRIBUTE_VENDOR_ID)
                pending.vid     = big_endian_read_16(attribute_value, pos);
            if (attr_id == BLUETOOTH_ATTRIBUTE_PRODUCT_ID)
                pending.pid     = big_endian_read_16(attribute_value, pos);
            if (attr_id == BLUETOOTH_ATTRIBUTE_VERSION)
                pending.version = big_endian_read_16(attribute_value, pos);
        }
        break;

    case SDP_EVENT_QUERY_COMPLETE:
        if (sdp_event_query_complete_get_status(packet))
        {
            printf("SDP query failed 0x%02x, connecting anyway\r\n", sdp_event_query_complete_get_status(packet));
        }
        else
        {
            printf("SDP: VID=0x%04x PID=0x%04x\r\n", pending.vid, pending.pid);
        }
        pending.device_id = next_bt_device_id++;

        hid_host_connection_pending = true;
        {
            uint8_t status = hid_host_connect(remote_addr, hid_host_report_mode, &hid_host_cid);
            if (status != ERROR_CODE_SUCCESS)
            {
                hid_host_connection_pending = false;
                printf("HID host connect failed, status 0x%02x.\r\n", status);
            }
        }
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Packet handler
// ---------------------------------------------------------------------------
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    uint8_t event;
    bd_addr_t event_addr;
    uint8_t status;
    bd_addr_t addr;
    int index;

    switch (packet_type)
    {
    case HCI_EVENT_PACKET:
        event = hci_event_packet_get_type(packet);

        switch (event)
        {
        case BTSTACK_EVENT_STATE:
        {
            uint8_t st = btstack_event_state_get_state(packet);
            printf("Classic BTstack state: %d (WORKING=%d)\r\n", st, HCI_STATE_WORKING);
            if (st == HCI_STATE_WORKING)
            {
                if (has_address)
                {
                    sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr,
                                            BLUETOOTH_SERVICE_CLASS_PNP_INFORMATION);
                }
            }
            break;
        }

        case GAP_EVENT_INQUIRY_RESULT:
            if (deviceCount >= MAX_DEVICES) break;
            gap_event_inquiry_result_get_bd_addr(packet, addr);
            index = getDeviceIndexForAddress(addr);
            if (index >= 0) break;
            {
                uint32_t cod = gap_event_inquiry_result_get_class_of_device(packet);
                // Major device class 0x0500 is Peripheral (or PERIPHERAL_COD)
                if ((cod & 0x1F00) != 0x0500 && (cod & PERIPHERAL_COD) != PERIPHERAL_COD) break;

                printf("Inquiry found peripheral: %s, COD: 0x%06lx\r\n", bd_addr_to_str(addr), (unsigned long)cod);
                memcpy(devices[deviceCount].address, addr, 6);
                devices[deviceCount].pageScanRepetitionMode =
                    gap_event_inquiry_result_get_page_scan_repetition_mode(packet);
                devices[deviceCount].clockOffset =
                    gap_event_inquiry_result_get_clock_offset(packet);
                devices[deviceCount].cod = cod;

                if (gap_event_inquiry_result_get_name_available(packet))
                {
                    int name_len = gap_event_inquiry_result_get_name_len(packet);
                    memcpy(devices[deviceCount].name_buffer,
                           gap_event_inquiry_result_get_name(packet), name_len);
                    devices[deviceCount].name_buffer[name_len] = ' ';
                    devices[deviceCount].name_buffer[name_len + 1] = '(';
                    memcpy(devices[deviceCount].name_buffer + name_len + 2,
                           bd_addr_to_str(addr), SIZE_OF_BD_ADDRESS);
                    devices[deviceCount].name_buffer[name_len + SIZE_OF_BD_ADDRESS + 1] = ')';
                    devices[deviceCount].name_buffer[name_len + SIZE_OF_BD_ADDRESS + 2] = 0;
                    printf("Found device '%s'\r\n", devices[deviceCount].name_buffer);
                    devices[deviceCount].state = REMOTE_NAME_FETCHED;
                }
                else
                {
                    devices[deviceCount].state = REMOTE_NAME_REQUEST;
                }
                deviceCount++;
            }
            break;

        case GAP_EVENT_INQUIRY_COMPLETE:
            printf("Inquiry complete, found %d devices\r\n", deviceCount);
            for (int i = 0; i < deviceCount; i++)
                if (devices[i].state == REMOTE_NAME_INQUIRED)
                    devices[i].state = REMOTE_NAME_REQUEST;
            if (has_more_remote_name_requests())
            {
                continue_remote_names();
            }
            else if (deviceCount > 0)
            {
                // Connect to the first discovered device
                memcpy(remote_addr, devices[0].address, sizeof(bd_addr_t));
                has_address = true;
                printf("Connecting to classic device %s (%s)...\r\n", devices[0].name_buffer, bd_addr_to_str(remote_addr));
                sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr,
                                        BLUETOOTH_SERVICE_CLASS_PNP_INFORMATION);
                bt_discovery_on_device_found();
            }
            else
            {
                bt_classic_on_inquiry_complete_empty();
            }
            break;

        case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
            hci_event_remote_name_request_complete_get_bd_addr(packet, addr);
            index = getDeviceIndexForAddress(addr);
            if (index >= 0)
            {
                if (packet[2] == 0)
                {
                    const char *name = hci_event_remote_name_request_complete_get_remote_name(packet);
                    int name_len = strlen(name);
                    memcpy(devices[index].name_buffer, name, name_len);
                    devices[index].name_buffer[name_len] = 0;
                    printf("Found device '%s'\r\n", devices[index].name_buffer);
                    devices[index].state = REMOTE_NAME_FETCHED;

                    // Immediately initiate connection to this discovered device
                    memcpy(remote_addr, devices[index].address, sizeof(bd_addr_t));
                    has_address = true;
                    printf("Connecting to classic device %s (%s)...\r\n", devices[index].name_buffer, bd_addr_to_str(remote_addr));
                    sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr,
                                            BLUETOOTH_SERVICE_CLASS_PNP_INFORMATION);
                    bt_discovery_on_device_found();
                    break;
                }
                else
                {
                    printf("Failed to get name: page timeout, connecting by address\r\n");
                    memcpy(remote_addr, devices[index].address, sizeof(bd_addr_t));
                    has_address = true;
                    sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr,
                                            BLUETOOTH_SERVICE_CLASS_PNP_INFORMATION);
                    bt_discovery_on_device_found();
                    break;
                }
            }
            continue_remote_names();
            break;

        case HCI_EVENT_PIN_CODE_REQUEST:
            if (!hid_host_connection_pending && hid_host_cid == 0) break;
            hci_event_pin_code_request_get_bd_addr(packet, event_addr);
            index = getDeviceIndexForAddress(event_addr);
            if ((index >= 0 && strstr(devices[index].name_buffer, "RVL") != nullptr) ||
                (pending_connections[hid_host_cid].vid == 0x057E))
            {
                printf("Pin code request for Nintendo Wii device - using reversed host BD_ADDR\r\n");
                bd_addr_t local_addr;
                static bd_addr_t pin_code;
                gap_local_bd_addr(local_addr);
                reverse_bd_addr(local_addr, pin_code);
                gap_pin_code_response_binary(event_addr, pin_code, sizeof(pin_code));
            }
            else
            {
                printf("Pin code request - using '0000'\r\n");
                gap_pin_code_response(event_addr, "0000");
            }
            break;

        case HCI_EVENT_USER_CONFIRMATION_REQUEST:
            printf("SSP User Confirmation Auto accept\r\n");
            break;

        case HCI_EVENT_HID_META:
            switch (hci_event_hid_meta_get_subevent_code(packet))
            {
            case HID_SUBEVENT_INCOMING_CONNECTION:
                // Accept incoming connections from any peripheral
                hid_host_accept_connection(
                    hid_subevent_incoming_connection_get_hid_cid(packet),
                    hid_host_report_mode);
                break;

            case HID_SUBEVENT_CONNECTION_OPENED:
            {
                uint16_t cid = hid_subevent_connection_opened_get_hid_cid(packet);
                status = hid_subevent_connection_opened_get_status(packet);
                if (status != ERROR_CODE_SUCCESS)
                {
                    printf("Connection failed, status 0x%02x\r\n", status);
                    pending_connections.erase(cid);
                    hid_host_connection_pending = false;
                    hid_host_cid = 0;
                    return;
                }
                hid_host_connection_pending = false;
                app_state = APP_CONNECTED;
                printf("HID Host connected, cid=0x%04x\r\n", cid);

                bd_addr_t connected_addr = {};
                hid_subevent_connection_opened_get_bd_addr(packet, connected_addr);

                // Move VID/PID from the pending map to a real host object
                PendingConnection &pending = pending_connections[cid];

                // Check if this device is already paired in config
                DeviceFactory::BluetoothPairingStateData paired_state = {};
                bool is_paired = DeviceFactory::find_bluetooth_pairing_state_by_mac(connected_addr, paired_state) && !paired_state.ble;

                uint16_t vid = pending.vid;
                uint16_t pid = pending.pid;
                SubType known_subtype = SubType_Unknown;
                bool known_ready = false;

                if (is_paired)
                {
                    printf("Device is already paired: VID=0x%04x PID=0x%04x SubType=%d\r\n",
                           paired_state.vid, paired_state.pid, (int)paired_state.subtype);
                    if (!vid && paired_state.vid)
                        vid = paired_state.vid;
                    if (!pid && paired_state.pid)
                        pid = paired_state.pid;
                    known_subtype = paired_state.subtype;
                    known_ready = true;
                }

                // Parse HID descriptor if available in storage (SDP populated this during hid_host_connect)
                const uint8_t *desc = hid_descriptor_storage_get_descriptor_data(cid);
                uint16_t desc_len   = hid_descriptor_storage_get_descriptor_len(cid);
                HID_ReportInfo_t *info = nullptr;
                if (desc && desc_len > 0)
                {
                    USB_ProcessHIDReport(desc, desc_len, &info);
                }
                else if (pending.info)
                {
                    info = pending.info;
                    pending.info = nullptr;
                }

                // Fallback detection from device name if SDP didn't populate VID/PID
                index = getDeviceIndexForAddress(connected_addr);
                if ((!vid || !pid) && index >= 0)
                {
                    if (strstr(devices[index].name_buffer, "RVL-CNT-01-UC") != nullptr)
                    {
                        vid = 0x057E;
                        pid = 0x0330;
                    }
                    else if (strstr(devices[index].name_buffer, "RVL") != nullptr)
                    {
                        vid = 0x057E;
                        pid = 0x0306;
                    }
                }

                auto host = bt_classic_create_host(vid, pid,
                                                   pending.version,
                                                   pending.device_id ? pending.device_id
                                                                      : next_bt_device_id++,
                                                   info,
                                                   known_subtype,
                                                   known_ready);

                memcpy(host->m_addr, connected_addr, 6);
                host->m_addr_type = BD_ADDR_TYPE_ACL;
                host->m_cid = cid;

                // Copy name from the scan result or paired state if we have it
                index = getDeviceIndexForAddress(connected_addr);
                if (index >= 0 && devices[index].name_buffer[0])
                    strncpy(host->m_name, devices[index].name_buffer, sizeof(host->m_name) - 1);
                else if (is_paired && paired_state.name[0])
                    strncpy(host->m_name, paired_state.name, sizeof(host->m_name) - 1);

                bt_connections[cid] = host;
                host->on_connected();
                bt_host_add_interface(host);
                if (host->is_ready())
                {
                    bt_host_save_pairing(host, false);
                }

                pending_connections.erase(cid);
                break;
            }

            case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:
            {
                uint16_t cid = hid_subevent_descriptor_available_get_hid_cid(packet);
                status = hid_subevent_descriptor_available_get_status(packet);
                if (status == ERROR_CODE_SUCCESS)
                {
                    printf("HID Descriptor available for cid=0x%04x\r\n", cid);
                }
                else
                {
                    printf("HID Descriptor unavailable: 0x%02x\r\n", status);
                }
                break;
            }

            case HID_SUBEVENT_REPORT:
            {
                uint16_t cid     = hid_subevent_report_get_hid_cid(packet);
                auto it = bt_connections.find(cid);
                if (it != bt_connections.end())
                {
                    const uint8_t *report = hid_subevent_report_get_report(packet);
                    uint16_t report_len   = hid_subevent_report_get_report_len(packet);
                    if (report_len > 0 && report[0] == 0xa1)
                    {
                        report++;
                        report_len--;
                    }
                    it->second->handle_report(report, report_len);
                }
                else
                {
                    printf_hexdump(hid_subevent_report_get_report(packet),
                                   hid_subevent_report_get_report_len(packet));
                }
                break;
            }

            case HID_SUBEVENT_CONNECTION_CLOSED:
            {
                uint16_t cid = hid_subevent_connection_closed_get_hid_cid(packet);
                auto it = bt_connections.find(cid);
                if (it != bt_connections.end())
                {
                    it->second->on_disconnected();
                    bt_connections.erase(it);
                }
                pending_connections.erase(cid);
                printf("HID Host disconnected, cid=0x%04x\r\n", cid);

                // Reconnect if this was our hard-coded remote address
                if (has_address)
                {
                    sdp_client_query_uuid16(&handle_sdp_client_query_result, remote_addr,
                                             BLUETOOTH_SERVICE_CLASS_PNP_INFORMATION);
                }
                break;
            }

            case HID_SUBEVENT_GET_REPORT_RESPONSE:
            {
                uint16_t cid = hid_subevent_get_report_response_get_hid_cid(packet);
                status = hid_subevent_get_report_response_get_handshake_status(packet);
                auto it = bt_connections.find(cid);
                if (it != bt_connections.end() && it->second)
                {
                    if (status != HID_HANDSHAKE_PARAM_TYPE_SUCCESSFUL)
                    {
                        printf("Error get report for cid 0x%04x, status 0x%02x\r\n", cid, status);
                        it->second->handle_feature_report_failed();
                    }
                    else
                    {
                        uint16_t len = hid_subevent_get_report_response_get_report_len(packet);
                        const uint8_t *report = hid_subevent_get_report_response_get_report(packet);
                        it->second->handle_feature_report(report, len);
                    }
                    bt_host_promote_if_ready(it->second);
                    if (it->second->is_ready())
                    {
                        bt_host_save_pairing(it->second, false);
                    }
                }
                break;
            }

            case HID_SUBEVENT_SET_PROTOCOL_RESPONSE:
                status = hid_subevent_set_protocol_response_get_handshake_status(packet);
                if (status != HID_HANDSHAKE_PARAM_TYPE_SUCCESSFUL)
                    printf("Error set protocol, status 0x%02x\r\n", status);
                break;

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
// Entry point
// ---------------------------------------------------------------------------
int btstack_classic_main(bool enable_hid_host)
{
    printf("btclassic init\r\n");
    if (enable_hid_host)
        hid_host_setup();
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);

#ifdef BT_ADDR
#ifdef CONFIGURABLE_BLOBS
    has_address = bt_addr[0];
#else
    has_address = true;
#endif
    if (has_address)
        sscanf_bd_addr(remote_addr_string, remote_addr);
#endif
    return 0;
}

void btstack_classic_set_accept_incoming(bool accept)
{
    hid_host_set_accept_incoming(accept);
}
