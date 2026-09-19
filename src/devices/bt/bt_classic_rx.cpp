
#define MAX_ATTRIBUTE_VALUE_SIZE 300
#define INQUIRY_INTERVAL 5
#define PERIPHERAL_COD ((1 << 8) | (1 << 10))
#define SIZE_OF_BD_ADDRESS 18
#define MAX_DEVICES_TO_SCAN 10

#include <inttypes.h>
#include <stdio.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "btstack.h"
#include "btstack_config.h"
#include "hidparser.h"
#include "devices/bt/bt_host.hpp"
#include "devices/bt/bt_classic_host.hpp"
#include "managers/device_manager.hpp"
#include "config/device_factory.hpp"
#include "devices/bluetooth.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "emulation/usb/usb_devices.h"

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------

// Forward declaration needed by hid_host_setup()
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static btstack_packet_callback_registration_t hci_event_callback_registration;

// SDP
static uint8_t hid_descriptor_storage[2048];

#define MAX_DEVICES 20
enum DEVICE_STATE
{
    REMOTE_NAME_REQUEST,
    REMOTE_NAME_INQUIRED,
    REMOTE_NAME_FETCHED
};
struct device
{
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
struct PendingConnection
{
    uint16_t vid = 0;
    uint16_t pid = 0;
    uint16_t version = 0;
    bool descriptor_available = false;
    bool hid_descriptor_parsed = false;
    HID_ReportInfo_t *info = nullptr;
    bd_addr_t addr = {};
    // device_id assigned when SDP query completes (before HID connect)
    uint16_t device_id = 0;
};
static std::unordered_map<uint16_t, PendingConnection> pending_connections;

// PNP SDP query in progress (before HID CID is allocated)
struct PnpSdpState
{
    bd_addr_t addr = {};
    bool in_progress = false;
    uint16_t vid = 0;
    uint16_t pid = 0;
    uint16_t version = 0;
};
static PnpSdpState s_pnp_sdp;

static hid_protocol_mode_t hid_host_report_mode = HID_PROTOCOL_MODE_REPORT;

static btstack_timer_source_t s_classic_reconnect_timer;
static bool s_classic_reconnect_timer_active = false;
static bool s_classic_connect_in_progress = false;
static size_t s_reconnect_candidate_idx = 0;

// Forward declaration needed by connect_to_discovered_device()
static void handle_sdp_client_query_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static bool is_already_connected_or_pending(const bd_addr_t addr)
{
    for (const auto &pair : bt_connections)
    {
        if (pair.second && bd_addr_cmp(pair.second->m_addr, addr) == 0)
            return true;
    }
    for (const auto &pair : pending_connections)
    {
        if (bd_addr_cmp(pair.second.addr, addr) == 0)
            return true;
    }
    if (s_pnp_sdp.in_progress && bd_addr_cmp(s_pnp_sdp.addr, addr) == 0)
        return true;
    return false;
}

struct ClassicCandidate
{
    bd_addr_t addr = {};
    char name[64] = {};
    uint16_t vid = 0;
    uint16_t pid = 0;
    bool has_link_key = false;
};

static std::vector<ClassicCandidate> btc_get_unconnected_paired_devices()
{
    std::vector<ClassicCandidate> candidates;

    // 1. Collect from DeviceFactory
    DeviceFactory::foreach_bluetooth_pairing_state([&candidates](int32_t id, const DeviceFactory::BluetoothPairingStateData &state)
                                                   {
        // Skip Wii remote emulation ID (when Pico acts as a Wiimote to a console)
        if (static_cast<uint32_t>(id) == 0xFFFFFFFEu) return;

        if (!state.ble && !btstack_is_null_bd_addr(state.mac))
        {
            // Seed link key into BTstack link key DB if available and non-null
            if (state.has_link_key && !btstack_is_null(state.link_key, 16))
            {
                link_key_t fetched_key;
                link_key_type_t key_type = INVALID_LINK_KEY;
                if (!gap_get_link_key_for_bd_addr(const_cast<uint8_t *>(state.mac), fetched_key, &key_type))
                {
                    gap_store_link_key_for_bd_addr(const_cast<uint8_t *>(state.mac),
                                                   const_cast<uint8_t *>(state.link_key),
                                                   COMBINATION_KEY);
                    printf("Classic BT: Seeded stored link key into BTstack for %s\r\n", bd_addr_to_str(const_cast<uint8_t *>(state.mac)));
                }
            }

            // Check if already connected
            for (const auto &pair : bt_connections)
            {
                if (pair.second && bd_addr_cmp(pair.second->m_addr, state.mac) == 0)
                    return;
            }

            bool exists = false;
            for (const auto &c : candidates)
            {
                if (bd_addr_cmp(c.addr, state.mac) == 0)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
            {
                ClassicCandidate cand = {};
                memcpy(cand.addr, state.mac, 6);
                strncpy(cand.name, state.name, sizeof(cand.name) - 1);
                cand.vid = state.vid;
                cand.pid = state.pid;
                cand.has_link_key = state.has_link_key;

                // Fallback detection from device name if VID/PID are not stored
                if (!cand.vid && !cand.pid)
                {
                    if (strstr(cand.name, "RVL-CNT-01-UC") != nullptr)
                    {
                        cand.vid = 0x057E;
                        cand.pid = 0x0330;
                    }
                    else if (strstr(cand.name, "RVL") != nullptr)
                    {
                        cand.vid = 0x057E;
                        cand.pid = 0x0306;
                    }
                }

                candidates.push_back(cand);
            }
        } });

    return candidates;
}

static void classic_reconnect_timer_handler(btstack_timer_source_t *ts);

static void btc_schedule_reconnect(uint32_t delay_ms)
{
    if (s_classic_reconnect_timer_active)
    {
        btstack_run_loop_remove_timer(&s_classic_reconnect_timer);
        s_classic_reconnect_timer_active = false;
    }
    btstack_run_loop_set_timer(&s_classic_reconnect_timer, delay_ms);
    btstack_run_loop_set_timer_handler(&s_classic_reconnect_timer, classic_reconnect_timer_handler);
    btstack_run_loop_add_timer(&s_classic_reconnect_timer);
    s_classic_reconnect_timer_active = true;
}

static void btc_sync_reconnect(void)
{
    if (s_classic_connect_in_progress)
    {
        return;
    }

    auto candidates = btc_get_unconnected_paired_devices();
    if (candidates.empty())
    {
        if (s_classic_reconnect_timer_active)
        {
            btstack_run_loop_remove_timer(&s_classic_reconnect_timer);
            s_classic_reconnect_timer_active = false;
        }
        return;
    }

    if (s_reconnect_candidate_idx >= candidates.size())
    {
        s_reconnect_candidate_idx = 0;
    }

    const auto &target = candidates[s_reconnect_candidate_idx];
    s_reconnect_candidate_idx = (s_reconnect_candidate_idx + 1) % candidates.size();

    if (is_already_connected_or_pending(target.addr))
    {
        btc_schedule_reconnect(2000);
        return;
    }

    uint16_t cid = 0;
    uint8_t status = hid_host_connect(const_cast<uint8_t *>(target.addr), hid_host_report_mode, &cid);
    if (status == ERROR_CODE_SUCCESS)
    {
        s_classic_connect_in_progress = true;
        PendingConnection &pending = pending_connections[cid];
        pending.vid = target.vid;
        pending.pid = target.pid;
        memcpy(pending.addr, target.addr, 6);
        pending.device_id = BluetoothStack::instance().device_id();
    }
    else
    {
        printf("Classic BT: hid_host_connect failed, status 0x%02x\r\n", status);
        btc_schedule_reconnect(3000);
    }
}

static void classic_reconnect_timer_handler(btstack_timer_source_t *ts)
{
    UNUSED(ts);
    s_classic_reconnect_timer_active = false;
    btc_sync_reconnect();
}

static void connect_to_discovered_device(const bd_addr_t addr, const char *name)
{
    if (is_already_connected_or_pending(addr))
    {
        return;
    }

    printf("Connecting to classic device %s (%s)...\r\n", name ? name : "", bd_addr_to_str(addr));

    if (!s_pnp_sdp.in_progress)
    {
        memcpy(s_pnp_sdp.addr, addr, sizeof(bd_addr_t));
        s_pnp_sdp.in_progress = true;
        s_pnp_sdp.vid = 0;
        s_pnp_sdp.pid = 0;
        s_pnp_sdp.version = 0;

        uint8_t err = sdp_client_query_uuid16(&handle_sdp_client_query_result, const_cast<uint8_t *>(addr),
                                              BLUETOOTH_SERVICE_CLASS_PNP_INFORMATION);
        if (err == ERROR_CODE_SUCCESS)
        {
            bt_discovery_on_device_found();
            return;
        }
        printf("SDP query failed to start (0x%02x), connecting directly\r\n", err);
        s_pnp_sdp.in_progress = false;
    }

    // Connect directly if SDP query could not be started or is already busy
    uint16_t cid = 0;
    uint8_t status = hid_host_connect(const_cast<uint8_t *>(addr), hid_host_report_mode, &cid);
    if (status == ERROR_CODE_SUCCESS)
    {
        PendingConnection &pending = pending_connections[cid];
        memcpy(pending.addr, addr, 6);
        pending.device_id = BluetoothStack::instance().device_id();
        bt_discovery_on_device_found();
    }
    else
    {
        printf("HID host connect failed, status 0x%02x\r\n", status);
    }
}

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

static bool is_wii_device(const bd_addr_t addr)
{
    int index = getDeviceIndexForAddress(const_cast<uint8_t *>(addr));
    if (index >= 0 && strstr(devices[index].name_buffer, "RVL") != nullptr)
    {
        return true;
    }
    if (s_pnp_sdp.in_progress && bd_addr_cmp(s_pnp_sdp.addr, addr) == 0 && s_pnp_sdp.vid == 0x057E)
    {
        return true;
    }
    for (const auto &pair : pending_connections)
    {
        if (bd_addr_cmp(pair.second.addr, addr) == 0 && pair.second.vid == 0x057E)
        {
            return true;
        }
    }
    DeviceFactory::BluetoothPairingStateData paired_state = {};
    if (DeviceFactory::find_bluetooth_pairing_state_by_mac(addr, paired_state) && !paired_state.ble)
    {
        if (paired_state.vid == 0x057E || strstr(paired_state.name, "RVL") != nullptr)
        {
            return true;
        }
    }
    return false;
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
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);

    hid_host_set_accept_incoming(true);

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    setvbuf(stdin, NULL, _IONBF, 0);
}

void btc_start_scan(uint32_t lap)
{
    printf("Starting inquiry scan (LAP 0x%06lx)..\r\n", (unsigned long)lap);
    if (s_classic_reconnect_timer_active)
    {
        btstack_run_loop_remove_timer(&s_classic_reconnect_timer);
        s_classic_reconnect_timer_active = false;
    }
    deviceCount = 0;
    gap_inquiry_set_lap(lap);
    gap_inquiry_start(INQUIRY_INTERVAL);
}

void btc_stop_scan(void)
{
    printf("Stopping inquiry scan..\r\n");
    gap_inquiry_stop();
    btc_schedule_reconnect(2000);
}

static int has_more_remote_name_requests(void)
{
    for (int i = 0; i < deviceCount; i++)
        if (devices[i].state == REMOTE_NAME_REQUEST)
            return 1;
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
                s_pnp_sdp.vid = big_endian_read_16(attribute_value, pos);
            if (attr_id == BLUETOOTH_ATTRIBUTE_PRODUCT_ID)
                s_pnp_sdp.pid = big_endian_read_16(attribute_value, pos);
            if (attr_id == BLUETOOTH_ATTRIBUTE_VERSION)
                s_pnp_sdp.version = big_endian_read_16(attribute_value, pos);
        }
        break;

    case SDP_EVENT_QUERY_COMPLETE:
        if (sdp_event_query_complete_get_status(packet))
        {
            printf("SDP query failed 0x%02x, connecting anyway\r\n", sdp_event_query_complete_get_status(packet));
        }
        else
        {
            printf("SDP: VID=0x%04x PID=0x%04x\r\n", s_pnp_sdp.vid, s_pnp_sdp.pid);
        }
        {
            bd_addr_t target_addr;
            memcpy(target_addr, s_pnp_sdp.addr, sizeof(bd_addr_t));
            uint16_t vid = s_pnp_sdp.vid;
            uint16_t pid = s_pnp_sdp.pid;
            uint16_t version = s_pnp_sdp.version;
            s_pnp_sdp.in_progress = false;

            uint16_t allocated_cid = 0;
            uint8_t status = hid_host_connect(target_addr, hid_host_report_mode, &allocated_cid);
            if (status == ERROR_CODE_SUCCESS)
            {
                PendingConnection &pending = pending_connections[allocated_cid];
                pending.vid = vid;
                pending.pid = pid;
                pending.version = version;
                memcpy(pending.addr, target_addr, 6);
                pending.device_id = BluetoothStack::instance().device_id();
            }
            else
            {
                printf("HID host connect failed, status 0x%02x.\r\n", status);
            }
        }
        break;

    default:
        break;
    }
}

static bool upgrade_to_ps4(uint16_t cid, std::shared_ptr<BluetoothHostInterface> old_host)
{
    printf("Upgrading BT Classic host on cid=0x%04x to BtDs4Host\r\n", cid);
    uint16_t vid = old_host->m_vid ? old_host->m_vid : SONY_VID;
    uint16_t pid = old_host->m_pid ? old_host->m_pid : SONY_DS4_PID_1;
    bool is_inst = (vid == MADCATZ_VID && (pid == PS4_STRAT_PID || pid == PS4_MADCATZ_DRUM_PID)) ||
                   (vid == PDP_VID && pid == PS4_JAG_PID);
    SubType sub = SubType_Gamepad;
    if (is_inst)
    {
        sub = (vid == MADCATZ_VID && pid == PS4_MADCATZ_DRUM_PID) ? RockBandDrums : RockBandGuitar;
    }
    else if (old_host->subtype() != SubType_Gamepad && old_host->subtype() != SubType_Unknown)
    {
        sub = old_host->subtype();
    }

    auto new_host = std::make_shared<BtDs4Host>(
        BluetoothStack::instance().device_id(),
        sub,
        is_inst,  // third_party
        !is_inst, // sensors
        !is_inst, // lightbar
        !is_inst, // vibration
        !is_inst, // touchpad
        vid,
        pid);
    memcpy(new_host->m_addr, old_host->m_addr, 6);
    new_host->m_addr_type = old_host->m_addr_type;
    new_host->m_cid = cid;
    if (old_host->m_name[0])
    {
        strncpy(new_host->m_name, old_host->m_name, sizeof(new_host->m_name) - 1);
    }

    bt_host_remove_interface(old_host.get());
    bt_connections[cid] = new_host;
    new_host->on_connected();
    bt_host_add_interface(new_host);
    return true;
}

static bool upgrade_to_ps5(uint16_t cid, std::shared_ptr<BluetoothHostInterface> old_host)
{
    printf("Upgrading BT Classic host on cid=0x%04x to BtDs5Host\r\n", cid);
    uint16_t vid = old_host->m_vid ? old_host->m_vid : SONY_VID;
    uint16_t pid = old_host->m_pid ? old_host->m_pid : SONY_DS5_PID;
    SubType sub = (old_host->subtype() != SubType_Gamepad && old_host->subtype() != SubType_Unknown)
                      ? old_host->subtype()
                      : SubType_Gamepad;

    auto new_host = std::make_shared<BtDs5Host>(
        BluetoothStack::instance().device_id(),
        sub,
        false, // third_party
        true,  // sensors
        true,  // lightbar
        true,  // vibration
        true,  // touchpad
        vid,
        pid);
    memcpy(new_host->m_addr, old_host->m_addr, 6);
    new_host->m_addr_type = old_host->m_addr_type;
    new_host->m_cid = cid;
    if (old_host->m_name[0])
    {
        strncpy(new_host->m_name, old_host->m_name, sizeof(new_host->m_name) - 1);
    }

    bt_host_remove_interface(old_host.get());
    bt_connections[cid] = new_host;
    new_host->on_connected();
    bt_host_add_interface(new_host);
    return true;
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
                gap_connectable_control(1);
                btc_sync_reconnect();
            }
            break;
        }

        case GAP_EVENT_INQUIRY_RESULT:
            if (deviceCount >= MAX_DEVICES)
                break;
            gap_event_inquiry_result_get_bd_addr(packet, addr);
            index = getDeviceIndexForAddress(addr);
            if (index >= 0)
                break;
            {
                uint32_t cod = gap_event_inquiry_result_get_class_of_device(packet);
                // Major device class 0x0500 is Peripheral (or PERIPHERAL_COD)
                if ((cod & 0x1F00) != 0x0500 && (cod & PERIPHERAL_COD) != PERIPHERAL_COD)
                    break;

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
                // Connect to the first discovered un-connected device
                bool connected_any = false;
                for (int i = 0; i < deviceCount; i++)
                {
                    if (!is_already_connected_or_pending(devices[i].address))
                    {
                        connect_to_discovered_device(devices[i].address, devices[i].name_buffer);
                        connected_any = true;
                        break;
                    }
                }
                if (!connected_any)
                {
                    bt_classic_on_inquiry_complete_empty();
                }
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

                    if (!is_already_connected_or_pending(devices[index].address))
                    {
                        connect_to_discovered_device(devices[index].address, devices[index].name_buffer);
                        break;
                    }
                }
                else
                {
                    printf("Failed to get name: page timeout\r\n");
                    if (!is_already_connected_or_pending(devices[index].address))
                    {
                        connect_to_discovered_device(devices[index].address, devices[index].name_buffer);
                        break;
                    }
                }
            }
            continue_remote_names();
            break;

        case HCI_EVENT_CONNECTION_COMPLETE:
        {
            status = hci_event_connection_complete_get_status(packet);
            hci_event_connection_complete_get_bd_addr(packet, event_addr);
            if (status == ERROR_CODE_SUCCESS)
            {
                hci_con_handle_t handle = hci_event_connection_complete_get_connection_handle(packet);
                printf("Classic ACL connection complete: %s, handle=0x%04x\r\n", bd_addr_to_str(event_addr), handle);
                if (is_wii_device(event_addr))
                {
                    printf("Requesting security LEVEL_2 for Wii device %s\r\n", bd_addr_to_str(event_addr));
                    gap_request_security_level(handle, LEVEL_2);
                }
            }
            else if (status != ERROR_CODE_PAGE_TIMEOUT)
            {
                printf("Classic ACL connection failed: %s, status=0x%02x\r\n", bd_addr_to_str(event_addr), status);
            }
            break;
        }

        case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT:
        {
            status = hci_event_authentication_complete_get_status(packet);
            hci_con_handle_t handle = hci_event_authentication_complete_get_connection_handle(packet);
            printf("Classic authentication complete: handle=0x%04x, status=0x%02x\r\n", handle, status);
            if (status == ERROR_CODE_AUTHENTICATION_FAILURE || status == ERROR_CODE_PIN_OR_KEY_MISSING)
            {
                hci_connection_t *conn = hci_connection_for_handle(handle);
                if (conn)
                {
                    printf("Authentication failed (bad pin/key), dropping link key for %s\r\n", bd_addr_to_str(conn->address));
                    gap_drop_link_key_for_bd_addr(conn->address);
                    int32_t pairing_id = DeviceFactory::find_bluetooth_pairing_id_by_mac(conn->address);
                    if (pairing_id >= 0)
                    {
                        DeviceFactory::set_bluetooth_pairing_link_key(pairing_id, nullptr);
                    }
                }
            }
            break;
        }

        case GAP_EVENT_SECURITY_LEVEL:
        {
            hci_con_handle_t handle = gap_event_security_level_get_handle(packet);
            gap_security_level_t sec_level = (gap_security_level_t)gap_event_security_level_get_security_level(packet);
            status = gap_event_security_level_get_status(packet);
            printf("GAP security level: handle=0x%04x, level=%d, status=0x%02x\r\n", handle, (int)sec_level, status);
            break;
        }

        case HCI_EVENT_PIN_CODE_REQUEST:
        {
            hci_event_pin_code_request_get_bd_addr(packet, event_addr);
            if (is_wii_device(event_addr))
            {
                printf("Pin code request for Nintendo Wii device %s - using reversed host BD_ADDR\r\n", bd_addr_to_str(event_addr));
                static bd_addr_t pin_code;
                bd_addr_t local_addr;
                gap_local_bd_addr(local_addr);
                reverse_bd_addr(local_addr, pin_code);
                gap_pin_code_response_binary(event_addr, pin_code, sizeof(pin_code));
            }
            else
            {
                printf("Pin code request for %s - using '0000'\r\n", bd_addr_to_str(event_addr));
                gap_pin_code_response(event_addr, "0000");
            }
            break;
        }

        case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        {
            hci_event_user_confirmation_request_get_bd_addr(packet, event_addr);
            printf("SSP User Confirmation Auto accept for %s\r\n", bd_addr_to_str(event_addr));
            gap_ssp_confirmation_response(event_addr);
            break;
        }

        case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
        {
            status = hci_event_simple_pairing_complete_get_status(packet);
            hci_event_simple_pairing_complete_get_bd_addr(packet, event_addr);
            printf("Classic SSP pairing complete: %s, status=0x%02x\r\n", bd_addr_to_str(event_addr), status);
            break;
        }

        case HCI_EVENT_HID_META:
            switch (hci_event_hid_meta_get_subevent_code(packet))
            {
            case HID_SUBEVENT_INCOMING_CONNECTION:
            {
                uint16_t cid = hid_subevent_incoming_connection_get_hid_cid(packet);
                bd_addr_t in_addr = {};
                hid_subevent_incoming_connection_get_address(packet, in_addr);
                printf("Classic BT: Incoming HID connection from %s, cid=0x%04x\r\n", bd_addr_to_str(in_addr), cid);
                hid_host_accept_connection(cid, hid_host_report_mode);
                break;
            }

            case HID_SUBEVENT_CONNECTION_OPENED:
            {
                uint16_t cid = hid_subevent_connection_opened_get_hid_cid(packet);
                status = hid_subevent_connection_opened_get_status(packet);
                s_classic_connect_in_progress = false;
                if (status != ERROR_CODE_SUCCESS)
                {
                    bd_addr_t fail_addr = {};
                    hid_subevent_connection_opened_get_bd_addr(packet, fail_addr);
                    if (status != ERROR_CODE_PAGE_TIMEOUT)
                    {
                        printf("Connection failed for %s, status 0x%02x\r\n", bd_addr_to_str(fail_addr), status);
                    }
                    if ((status == 0x67 || status == 0x66) && is_wii_device(fail_addr))
                    {
                        printf("L2CAP security/resource refusal, dropping stored link key for Wii %s\r\n", bd_addr_to_str(fail_addr));
                        gap_drop_link_key_for_bd_addr(fail_addr);
                        int32_t pairing_id = DeviceFactory::find_bluetooth_pairing_id_by_mac(fail_addr);
                        if (pairing_id >= 0)
                        {
                            DeviceFactory::set_bluetooth_pairing_link_key(pairing_id, nullptr);
                        }
                    }
                    pending_connections.erase(cid);
                    btc_schedule_reconnect(2000);
                    return;
                }
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
                BtControllerType known_controller_type = BtControllerType_BtControllerTypeGeneric;

                if (is_paired)
                {
                    printf("Device is already paired: VID=0x%04x PID=0x%04x SubType=%d Type=%d\r\n",
                           paired_state.vid, paired_state.pid, (int)paired_state.subtype, (int)paired_state.controller_type);
                    if (!vid && paired_state.vid)
                        vid = paired_state.vid;
                    if (!pid && paired_state.pid)
                        pid = paired_state.pid;
                    known_subtype = paired_state.subtype;
                    known_ready = true;
                    known_controller_type = paired_state.controller_type;
                }

                // Parse HID descriptor if available in storage (SDP populated this during hid_host_connect)
                const uint8_t *desc = hid_descriptor_storage_get_descriptor_data(cid);
                uint16_t desc_len = hid_descriptor_storage_get_descriptor_len(cid);
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

                const char *dev_name = "";
                index = getDeviceIndexForAddress(connected_addr);
                if (index >= 0 && devices[index].name_buffer[0])
                    dev_name = devices[index].name_buffer;
                else if (is_paired && paired_state.name[0])
                    dev_name = paired_state.name;

                // Fallback detection from device name if SDP didn't populate VID/PID
                if (!vid || !pid)
                {
                    if (strstr(dev_name, "RVL-CNT-01-UC") != nullptr)
                    {
                        vid = NINTENDO_VID;
                        pid = WII_U_PRO_PID;
                    }
                    else if (strstr(dev_name, "RVL") != nullptr)
                    {
                        vid = NINTENDO_VID;
                        pid = WII_REMOTE_PID;
                    }
                    else if (is_switch_name(dev_name))
                    {
                        vid = NINTENDO_VID;
                        pid = switch_pid_from_name(dev_name);
                    }
                    else if (strstr(dev_name, "PLAYSTATION(R)3") != nullptr)
                    {
                        vid = SONY_VID;
                        pid = SONY_DS3_PID;
                    }
                    else if (strstr(dev_name, "Navigation Controller") != nullptr)
                    {
                        vid = SONY_VID;
                        pid = SONY_PS3_NAV_PID;
                    }
                    else if (strstr(dev_name, "Xbox Wireless Controller") != nullptr)
                    {
                        vid = XBOX_VID;
                        pid = XBOX_BT_PID;
                    }
                    else if (strstr(dev_name, "DualSense") != nullptr)
                    {
                        vid = SONY_VID;
                        pid = SONY_DS5_PID;
                    }
                    else if (strstr(dev_name, "DualShock 4") != nullptr ||
                             strstr(dev_name, "DUALSHOCK 4") != nullptr ||
                             strstr(dev_name, "Wireless Controller") != nullptr)
                    {
                        vid = SONY_VID;
                        pid = SONY_DS4_PID_1;
                    }
                }

                auto host = bt_classic_create_host(vid, pid,
                                                   pending.version,
                                                   BluetoothStack::instance().device_id(),
                                                   info,
                                                   known_subtype,
                                                   known_ready,
                                                   dev_name,
                                                   known_controller_type);

                memcpy(host->m_addr, connected_addr, 6);
                host->m_addr_type = BD_ADDR_TYPE_ACL;
                host->m_cid = cid;

                // Copy name from the scan result or paired state if we have it
                if (dev_name[0])
                    strncpy(host->m_name, dev_name, sizeof(host->m_name) - 1);

                bt_connections[cid] = host;
                host->on_connected();
                bt_host_add_interface(host);

                pending_connections.erase(cid);
                btc_schedule_reconnect(1000);
                break;
            }

            case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:
            {
                uint16_t cid = hid_subevent_descriptor_available_get_hid_cid(packet);
                status = hid_subevent_descriptor_available_get_status(packet);
                if (status == ERROR_CODE_SUCCESS)
                {
                    printf("HID Descriptor available for cid=0x%04x\r\n", cid);
                    const uint8_t *desc = hid_descriptor_storage_get_descriptor_data(cid);
                    uint16_t desc_len = hid_descriptor_storage_get_descriptor_len(cid);
                    if (desc && desc_len > 0)
                    {
                        HID_ReportInfo_t *info = nullptr;
                        USB_ProcessHIDReport(desc, desc_len, &info);
                        if (info)
                        {
                            auto it = bt_connections.find(cid);
                            if (it != bt_connections.end() && it->second)
                            {
                                if (it->second->controller_type() == BtControllerType_BtControllerTypeGeneric)
                                {
                                    if (info->foundPS4Usage)
                                    {
                                        upgrade_to_ps4(cid, it->second);
                                        USB_FreeReportInfo(info);
                                    }
                                    else if (info->foundPS5Usage)
                                    {
                                        upgrade_to_ps5(cid, it->second);
                                        USB_FreeReportInfo(info);
                                    }
                                    else
                                    {
                                        static_cast<BtGenericHost *>(it->second.get())->set_report_info(info);
                                        bt_host_promote_if_ready(it->second);
                                    }
                                }
                                else
                                {
                                    USB_FreeReportInfo(info);
                                }
                            }
                            else
                            {
                                USB_FreeReportInfo(info);
                            }
                        }
                    }
                }
                else
                {
                    printf("HID Descriptor unavailable: 0x%02x\r\n", status);
                }
                break;
            }

            case HID_SUBEVENT_REPORT:
            {
                uint16_t cid = hid_subevent_report_get_hid_cid(packet);
                auto it = bt_connections.find(cid);
                if (it != bt_connections.end())
                {
                    const uint8_t *report = hid_subevent_report_get_report(packet);
                    uint16_t report_len = hid_subevent_report_get_report_len(packet);
                    if (report_len > 0 && report[0] == 0xa1)
                    {
                        report++;
                        report_len--;
                    }
                    if (it->second->controller_type() == BtControllerType_BtControllerTypeGeneric)
                    {
                        if (report_len >= 48 && report[0] == 0x11)
                        {
                            upgrade_to_ps4(cid, it->second);
                            it = bt_connections.find(cid);
                        }
                        else if (report_len >= 48 && report[0] == 0x31)
                        {
                            upgrade_to_ps5(cid, it->second);
                            it = bt_connections.find(cid);
                        }
                        else if (report_len == 64 && report[0] == 0x01 && (report[5] & 0x0F) <= 8)
                        {
                            const char *name = it->second->m_name;
                            if (strstr(name, "DualSense") || strstr(name, "PS5"))
                            {
                                upgrade_to_ps5(cid, it->second);
                                it = bt_connections.find(cid);
                            }
                            else if (strstr(name, "Wireless Controller") || strstr(name, "DualShock") ||
                                     strstr(name, "DUALSHOCK") || strstr(name, "PS4") ||
                                     it->second->m_vid == SONY_VID)
                            {
                                upgrade_to_ps4(cid, it->second);
                                it = bt_connections.find(cid);
                            }
                        }
                    }
                    if (it != bt_connections.end() && it->second)
                    {
                        it->second->request_capabilities();
                        it->second->handle_report(report, report_len);
                    }
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
                btc_schedule_reconnect(1000);
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
            {
                uint16_t cid = hid_subevent_set_protocol_response_get_hid_cid(packet);
                status = hid_subevent_set_protocol_response_get_handshake_status(packet);
                printf("HID Set Protocol Response for cid=0x%04x, status=0x%02x\r\n", cid, status);
                if (status != HID_HANDSHAKE_PARAM_TYPE_SUCCESSFUL)
                    printf("Error set protocol, status 0x%02x\r\n", status);
                auto it = bt_connections.find(cid);
                if (it != bt_connections.end() && it->second)
                {
                    it->second->request_capabilities();
                }
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
// Entry point
// ---------------------------------------------------------------------------
int btstack_classic_main(bool enable_hid_host)
{
    printf("btclassic init\r\n");
    if (enable_hid_host)
        hid_host_setup();
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    return 0;
}

void btstack_classic_set_accept_incoming(bool accept)
{
    hid_host_set_accept_incoming(accept);
}
