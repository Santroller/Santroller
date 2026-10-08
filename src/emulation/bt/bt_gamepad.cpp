#include "emulation/santroller_commands.hpp"
#include "devices/bt/bluetooth_status.hpp"
#include "managers/battery_manager.hpp"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"
#include <pico/unique_id.h>
#include "emulation/bt/bt_profile.h"
#include "emulation/bt/bt_descriptors.h"
#include "emulation/bt/bt_gamepad.h"
#include "emulation/bt/bt_config_service.h"
#include "managers/config_manager.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "btstack.h"
#include "utils.h"
#include "enums.pb.h"
#define SIZE_OF_BD_ADDRESS 18
// static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint8_t battery = 100;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
// a host connected to us, from the moment it connects (con_handle is only set once it subscribes to reports)
static hci_con_handle_t peripheral_handle = HCI_CON_HANDLE_INVALID;
static uint8_t protocol_mode = 1;
static BTGamepadDevice *s_instance = nullptr;
static hids_device_report_t report_storage[6];
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void get_report_callback(hci_con_handle_t con_hdl, hid_report_type_t report_type, uint16_t report_id, uint16_t max_report_size, uint8_t * out_report);
// Appearance HID - Keyboard (Category 15, Sub-Category 1)
#define APPEARANCE_KEYBOARD 0xC1
// Appearance HID - Gamepad (Category 15, Sub-Category 4)
#define APPEARANCE_GAMEPAD 0xC4
// TODO: we could just build this on the fly, grabbing the name from the profile
static const uint8_t adv_data_keyboard[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02,
    BLUETOOTH_DATA_TYPE_FLAGS,
    0x06,
    // Name
    0x0d,
    BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'S',
    'a',
    'n',
    't',
    'r',
    'o',
    'l',
    'l',
    'e',
    'r',
    'B',
    'T',
    // 16-bit Service UUIDs
    0x03,
    BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    0x03,
    BLUETOOTH_DATA_TYPE_APPEARANCE,
    // Appearance HID - Keyboard (Category 15, Sub-Category 1)
    0xC1,
    0x03,
};
static const uint8_t adv_data_gamepad[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02,
    BLUETOOTH_DATA_TYPE_FLAGS,
    0x06,
    // Name
    0x0d,
    BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'S',
    'a',
    'n',
    't',
    'r',
    'o',
    'l',
    'l',
    'e',
    'r',
    'B',
    'T',
    // 16-bit Service UUIDs
    0x03,
    BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    0x03,
    BLUETOOTH_DATA_TYPE_APPEARANCE,
    // Appearance HID - Gamepad (Category 15, Sub-Category 4)
    0xC4,
    0x03,
};
bool check_bluetooth_ready()
{
    return con_handle != HCI_CON_HANDLE_INVALID;
}
int get_bt_address(uint8_t *addr)
{
    BtStackLock lock;
    bd_addr_t local_addr;
    gap_local_bd_addr(local_addr);
    memcpy(addr, bd_addr_to_str(local_addr), SIZE_OF_BD_ADDRESS);
    return SIZE_OF_BD_ADDRESS;
}
void send_report(uint8_t size, uint8_t *report)
{
    BtStackLock lock;
    if (con_handle != HCI_CON_HANDLE_INVALID)
    {
        if (size > 0 && report[0] == ReportIdSantrollerCapabilities)
        {
            hids_device_send_input_report_for_id(con_handle, ReportIdSantrollerCapabilities, report + 1, size - 1);
        }
        else
        {
            hids_device_send_input_report(con_handle, report + 1, size - 1);
        }
        // hids_device_request_can_send_now_event(con_handle);
    }
}
static void get_report_callback(hci_con_handle_t con_hdl, hid_report_type_t report_type, uint16_t report_id, uint16_t max_report_size, uint8_t * out_report)
{
    UNUSED(con_hdl);
    UNUSED(report_type);
    if (report_id == ReportIdSantrollerCapabilities && s_instance)
    {
        if (max_report_size >= 2)
        {
            out_report[0] = s_instance->subtype;
            out_report[1] = s_instance->capabilities;
        }
    }
}
const uint8_t adv_data_len = sizeof(adv_data_gamepad);

void set_battery_state(uint8_t state)
{
    BtStackLock lock;
    battery = state;
    battery_service_server_set_battery_value(state);
}
BTGamepadDevice::BTGamepadDevice()
{
}
BTGamepadDevice::~BTGamepadDevice()
{
    deinitialize();
}

void BTGamepadDevice::deinitialize()
{
    BtStackLock lock;
    if (!m_initialized)
    {
        return;
    }
    if (con_handle != HCI_CON_HANDLE_INVALID)
    {
        gap_disconnect(con_handle);
        con_handle = HCI_CON_HANDLE_INVALID;
    }
    s_instance = nullptr;
    hids_device_register_get_report_callback(nullptr);
    hids_device_register_packet_handler(nullptr);
    gap_advertisements_enable(0);
    gap_advertisements_set_data(0, nullptr);
    hci_remove_event_handler(&hci_event_callback_registration);
    sm_remove_event_handler(&sm_event_callback_registration);
    m_initialized = false;
    printf("btgamepaddevice deinit\r\n");
}

void BTGamepadDevice::initialize()
{
    BtStackLock lock;
    if (m_initialized)
    {
        return;
    }
    if (!ConfigManager::instance().has_bluetooth())
    {
        printf("BT: gamepad not started, bluetooth isn't enabled in the config\r\n");
        return;
    }
    printf("btgamepaddevice init\r\n");
    if (!BluetoothStack::instance().begin())
    {
        printf("BT: gamepad not started, the bluetooth stack failed to start\r\n");
        return;
    }
    s_instance = this;
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING);

    // setup ATT server
    att_server_init(profile_data, NULL, NULL);
    att_server_register_packet_handler(packet_handler);

    // setup battery service
    battery_service_server_init(battery);

    // setup device information service
    device_information_service_server_init();
    device_information_service_server_set_pnp_id(DEVICE_ID_VENDOR_ID_SOURCE_USB, 0x1209, 0x2882, 0);
    char id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2];
    pico_get_unique_board_id_string(id, sizeof(id));
    device_information_service_server_set_serial_number(id);
    memset(report_storage, 0, sizeof(report_storage));
    uint16_t num_reports = sizeof(report_storage) / sizeof(report_storage[0]);
    switch (subtype)
    {
    case SubType_KeyboardMouse:
        hids_device_init_with_storage(0, desc_hid_report_keyboard, desc_hid_report_keyboard_len, num_reports, report_storage);
        break;
    case SubType_Dancepad:
        hids_device_init_with_storage(0, desc_hid_report_buttons, sizeof(desc_hid_report_buttons), num_reports, report_storage);
        break;
    default:
        hids_device_init_with_storage(0, desc_hid_report_hat, sizeof(desc_hid_report_hat), num_reports, report_storage);
        break;
    }
    hids_device_register_get_report_callback(get_report_callback);

    // lets the configurator talk to us over bluetooth
    bt_config_service_init();

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);

    switch (subtype)
    {
    case SubType_KeyboardMouse:
        gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data_keyboard);
        break;
    default:
        gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data_gamepad);
        break;
    }
    gap_advertisements_enable(1);
    {
        bd_addr_t local_addr;
        gap_local_bd_addr(local_addr);
        printf("BT: advertising as SantrollerBT from %s, subtype %d\r\n", bd_addr_to_str(local_addr), subtype);
    }

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for SM events
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // register for HIDS
    hids_device_register_packet_handler(packet_handler);

    memset(&m_initial_report, 0, sizeof(m_initial_report));
    switch (subtype)
    {
    case RockBandDrums:
    {
        XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)&m_initial_report;
        report->redVelocity = -1;
        report->blueVelocity = -1;
        report->greenVelocity = 0;
        report->yellowVelocity = 0;
        break;
    }
    case GuitarHeroGuitar:
    {
        XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case LiveGuitar:
    {
        XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case GuitarHeroDrums:
    {
        XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)&m_initial_report;
        report->leftThumbClick = true;
        break;
    }
    default:
        break;
    }
    XInputGamepad_Data_t *gamepad = (XInputGamepad_Data_t *)m_initial_report;
    gamepad->rsize = sizeof(XInputGamepad_Data_t);
    m_reports.reset();
    m_initialized = true;
}

void BTGamepadDevice::process_keyboard_mouse(bool full_poll, bool send_events)
{
    for (const auto &profile : profiles)
    {
        profile->reset_drum_state();
        profile->keyboard_state.clear_all();
        profile->mouse_state.clear_all();
        profile->consumer_state.clear_all();
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_hid(m_epin_buffer);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    m_reports.collect(profiles);
    BtStackLock lock;
    if (con_handle == HCI_CON_HANDLE_INVALID)
    {
        return;
    }
    // Reports that can't go out yet (no ACL buffers) stay pending and are retried next loop
    KeyboardReport keyboard;
    if (m_reports.keyboard_pending(keyboard))
    {
        uint8_t status = protocol_mode
                             ? hids_device_send_input_report_for_id(con_handle, KEYBOARD_REPORT_ID, (const uint8_t *)&keyboard, sizeof(keyboard))
                             : hids_device_send_boot_keyboard_input_report(con_handle, (const uint8_t *)&keyboard, KEYBOARD_BOOT_REPORT_SIZE);
        if (status == ERROR_CODE_SUCCESS)
        {
            m_reports.keyboard_sent(keyboard);
        }
        return;
    }
    // boot protocol only has a keyboard and mouse, so media keys need report protocol
    ConsumerReport consumer;
    if (protocol_mode && m_reports.consumer_pending(consumer))
    {
        if (hids_device_send_input_report_for_id(con_handle, CONSUMER_REPORT_ID, (const uint8_t *)&consumer, sizeof(consumer)) == ERROR_CODE_SUCCESS)
        {
            m_reports.consumer_sent(consumer);
        }
        return;
    }
    MouseReport mouse;
    if (protocol_mode && m_reports.mouse_pending(mouse) &&
        hids_device_send_input_report_for_id(con_handle, MOUSE_REPORT_ID, (const uint8_t *)&mouse, sizeof(mouse)) == ERROR_CODE_SUCCESS)
    {
        m_reports.mouse_sent(mouse);
    }
}

bool bt_gamepad_connected()
{
    return con_handle != HCI_CON_HANDLE_INVALID;
}

bool bt_gamepad_advertising()
{
    // BTstack advertises while it has room for another peripheral connection, which is only ever one here
    return s_instance && peripheral_handle == HCI_CON_HANDLE_INVALID && BluetoothStack::instance().is_powered() &&
           hci_get_state() == HCI_STATE_WORKING;
}

void BTGamepadDevice::process(bool full_poll, bool send_events)
{
    if (m_initialized)
    {
        bt_config_service_process(full_poll, send_events);
    }
    // the battery service starts with the current level, so only changes after that need sending
    uint8_t level = BatteryManager::instance().level();
    if (m_initialized && level != battery)
    {
        set_battery_state(level);
    }
    if (con_handle == HCI_CON_HANDLE_INVALID)
    {
        // Nothing to send to, but the inputs still need reading so presses count as activity,
        // which is what turns bluetooth back on after it times out
        for (const auto &profile : profiles)
        {
            for (const auto &mapping : profile->mappings)
            {
                mapping->update(full_poll, send_events);
            }
            for (const auto &led : profile->leds)
            {
                led->update(full_poll, send_events);
            }
        }
        return;
    }
    if (subtype == SubType_KeyboardMouse)
    {
        if (con_handle != HCI_CON_HANDLE_INVALID)
        {
            process_keyboard_mouse(full_poll, send_events);
        }
        return;
    }
    if (con_handle != HCI_CON_HANDLE_INVALID)
    {
        PCGamepadDpad_Data_t *report = (PCGamepadDpad_Data_t *)m_epin_buffer;
        memcpy(m_epin_buffer, m_initial_report, sizeof(m_epin_buffer));
        report->rid = ReportIdGamepad;
        report->rsize = sizeof(PCGamepadDpad_Data_t);

        for (const auto &profile : profiles)
        {
            profile->reset_drum_state();
            for (const auto &mapping : profile->mappings)
            {
                mapping->update(full_poll, send_events);
                mapping->update_hid(m_epin_buffer);
            }
            for (const auto &led : profile->leds)
            {
                led->update(full_poll, send_events);
            }
        }
        if (invert_y_axis_hid && subtype == Gamepad)
        {
            report->leftStickY = static_cast<int16_t>(~static_cast<uint16_t>(report->leftStickY));
            report->rightStickY = static_cast<int16_t>(~static_cast<uint16_t>(report->rightStickY));
        }
        // dance pads need to report the dpad as buttons, so skip the conversion to hat
        if (subtype != Dancepad)
        {
            // convert bitmask dpad to actual hid dpad
            report->dpad = GamepadButtonMapping::dpad_bindings[report->dpad];
        }
        if (subtype == GuitarHeroGuitar)
        {
            // convert bitmask slider to actual hid slider
            XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)report;
            reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
        }
        if (memcmp(m_last_report, m_epin_buffer, sizeof(XInputGamepad_Data_t)) != 0)
        {

            send_report(sizeof(XInputGamepad_Data_t), m_epin_buffer);
            memcpy(m_last_report, m_epin_buffer, sizeof(XInputGamepad_Data_t));
        }
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET)
        return;
    switch (hci_event_packet_get_type(packet))
    {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
        {
            bd_addr_t local_addr;
            gap_local_bd_addr(local_addr);
            printf("BT: stack up, address %s\r\n", bd_addr_to_str(local_addr));
        }
        break;
    case HCI_EVENT_META_GAP:
        if (hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE &&
            gap_subevent_le_connection_complete_get_status(packet) == ERROR_CODE_SUCCESS &&
            gap_subevent_le_connection_complete_get_role(packet) != HCI_ROLE_MASTER)
        {
            peripheral_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (hci_event_disconnection_complete_get_connection_handle(packet) == peripheral_handle)
        {
            peripheral_handle = HCI_CON_HANDLE_INVALID;
        }
        con_handle = HCI_CON_HANDLE_INVALID;
        bt_config_service_disconnected();
        // 0x05 auth failure, 0x08 supervision timeout, 0x13 remote closed, 0x16 we closed, 0x3d MIC failure (bad keys)
        printf("BT: disconnected handle 0x%04x reason 0x%02x\r\n", hci_event_disconnection_complete_get_connection_handle(packet), hci_event_disconnection_complete_get_reason(packet));
        break;
    case HCI_EVENT_ENCRYPTION_CHANGE:
        printf("BT: encryption change handle 0x%04x status 0x%02x enabled %u\r\n", hci_event_encryption_change_get_connection_handle(packet), hci_event_encryption_change_get_status(packet), hci_event_encryption_change_get_encryption_enabled(packet));
        break;
    case SM_EVENT_PAIRING_STARTED:
    {
        bd_addr_t addr;
        sm_event_pairing_started_get_address(packet, addr);
        printf("BT: pairing started with %s\r\n", bd_addr_to_str(addr));
        break;
    }
    case SM_EVENT_REENCRYPTION_STARTED:
    {
        bd_addr_t addr;
        sm_event_reencryption_started_get_address(packet, addr);
        printf("BT: re-encryption started with %s\r\n", bd_addr_to_str(addr));
        break;
    }
    case SM_EVENT_REENCRYPTION_COMPLETE:
        // a failure here usually means one side lost the bond, remove the device on the host and pair again
        printf("BT: re-encryption complete status 0x%02x\r\n", sm_event_reencryption_complete_get_status(packet));
        break;
    case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
    {
        bd_addr_t addr;
        sm_event_identity_resolving_succeeded_get_address(packet, addr);
        printf("BT: resolved bonded identity %s\r\n", bd_addr_to_str(addr));
        break;
    }
    case SM_EVENT_IDENTITY_RESOLVING_FAILED:
    {
        bd_addr_t addr;
        sm_event_identity_resolving_failed_get_address(packet, addr);
        printf("BT: %s is not bonded\r\n", bd_addr_to_str(addr));
        break;
    }
    case SM_EVENT_IDENTITY_CREATED:
    {
        bd_addr_t addr;
        sm_event_identity_created_get_address(packet, addr);
        printf("BT: bond stored for %s\r\n", bd_addr_to_str(addr));
        break;
    }
    case SM_EVENT_AUTHORIZATION_RESULT:
        printf("BT: authorization result %u\r\n", sm_event_authorization_result_get_authorization_result(packet));
        break;
    case ATT_EVENT_CONNECTED:
        printf("BT: ATT connected handle 0x%04x\r\n", att_event_connected_get_handle(packet));
        break;
    case ATT_EVENT_DISCONNECTED:
        printf("BT: ATT disconnected handle 0x%04x\r\n", att_event_disconnected_get_handle(packet));
        break;
    case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
        printf("BT: MTU %u\r\n", att_event_mtu_exchange_complete_get_MTU(packet));
        break;
    case SM_EVENT_JUST_WORKS_REQUEST:
        printf("Just Works requested\r\n");
        sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
        break;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        printf("Confirming numeric comparison: %" PRIu32 "\r\n", sm_event_numeric_comparison_request_get_passkey(packet));
        sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
        break;
    case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
        printf("Display Passkey: %" PRIu32 "\r\n", sm_event_passkey_display_number_get_passkey(packet));
        break;
    case SM_EVENT_PAIRING_COMPLETE:
        switch (sm_event_pairing_complete_get_status(packet))
        {
        case ERROR_CODE_SUCCESS:
            printf("Pairing complete, success\r\n");
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            printf("Pairing failed, timeout\r\n");
            break;
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
            printf("Pairing failed, disconnected\r\n");
            break;
        case ERROR_CODE_AUTHENTICATION_FAILURE:
            // reason is an SM_REASON_* code, e.g. 0x05 pairing not supported, 0x08 unspecified, 0x0b DHKey check failed
            printf("Pairing failed, authentication failure with reason = 0x%02x\r\n", sm_event_pairing_complete_get_reason(packet));
            break;
        default:
            printf("Pairing failed, status 0x%02x reason 0x%02x\r\n", sm_event_pairing_complete_get_status(packet), sm_event_pairing_complete_get_reason(packet));
            break;
        }
        break;
    case HCI_EVENT_LE_META:
        switch (hci_event_le_meta_get_subevent_code(packet))
        {
        case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1:
        {
            // used instead of LE_CONNECTION_COMPLETE when address resolution is enabled
            bd_addr_t addr;
            hci_subevent_le_enhanced_connection_complete_v1_get_peer_addresss(packet, addr);
            printf("BT: LE connection status 0x%02x handle 0x%04x role %u peer %s (type %u)\r\n",
                   hci_subevent_le_enhanced_connection_complete_v1_get_status(packet),
                   hci_subevent_le_enhanced_connection_complete_v1_get_connection_handle(packet),
                   hci_subevent_le_enhanced_connection_complete_v1_get_role(packet),
                   bd_addr_to_str(addr),
                   hci_subevent_le_enhanced_connection_complete_v1_get_peer_address_type(packet));
            break;
        }
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
        {
            bd_addr_t addr;
            hci_subevent_le_connection_complete_get_peer_address(packet, addr);
            printf("BT: LE connection status 0x%02x handle 0x%04x role %u peer %s\r\n",
                   hci_subevent_le_connection_complete_get_status(packet),
                   hci_subevent_le_connection_complete_get_connection_handle(packet),
                   hci_subevent_le_connection_complete_get_role(packet),
                   bd_addr_to_str(addr));
            // print connection parameters (without using float operations)
            uint16_t conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
            printf("LE Connection Complete:\r\n");
            printf("- Connection Interval: %u.%02u ms\r\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
            printf("- Connection Latency: %u\r\n", hci_subevent_le_connection_complete_get_conn_latency(packet));
            break;
        }
        case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
        {
            // print connection parameters (without using float operations)
            uint16_t conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
            printf("LE Connection Update:\r\n");
            printf("- Connection Interval: %u.%02u ms\r\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
            printf("- Connection Latency: %u\r\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
            break;
        }
        default:
            break;
        }
        break;
    case HCI_EVENT_HIDS_META:
        switch (hci_event_hids_meta_get_subevent_code(packet))
        {
        case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
            con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
            gap_request_connection_parameter_update(con_handle, 6, 7, 0, 100);
            printf("Input Report Characteristic Subscribed %u\r\n", hids_subevent_input_report_enable_get_enable(packet));
            break;
        case HIDS_SUBEVENT_OUTPUT_REPORT_ENABLE:
            con_handle = hids_subevent_output_report_enable_get_con_handle(packet);
            printf("Output Report Characteristic Subscribed %u\r\n", hids_subevent_output_report_enable_get_enable(packet));
            break;
        case HIDS_SUBEVENT_FEATURE_REPORT_ENABLE:
            con_handle = hids_subevent_feature_report_enable_get_con_handle(packet);
            printf("Feature Report Characteristic Subscribed %u\r\n", hids_subevent_feature_report_enable_get_enable(packet));
            break;
        case HIDS_SUBEVENT_BOOT_KEYBOARD_INPUT_REPORT_ENABLE:
            con_handle = hids_subevent_boot_keyboard_input_report_enable_get_con_handle(packet);
            printf("Boot Keyboard Characteristic Subscribed %u\r\n", hids_subevent_boot_keyboard_input_report_enable_get_enable(packet));
            break;
        case HIDS_SUBEVENT_PROTOCOL_MODE:
            protocol_mode = hids_subevent_protocol_mode_get_protocol_mode(packet);
            printf("Protocol Mode: %s mode\r\n", hids_subevent_protocol_mode_get_protocol_mode(packet) ? "Report" : "Boot");
            break;
        case HIDS_SUBEVENT_SET_REPORT:
        {
            uint8_t report_id = hids_subevent_set_report_get_report_id(packet);
            uint8_t report_type = hids_subevent_set_report_get_report_type(packet);
            uint8_t report_len = hids_subevent_set_report_get_report_length(packet);
            const uint8_t *report_data = hids_subevent_set_report_get_report_data(packet);

            printf("set report id=%d, type=%d, len=%d\r\n", report_id, report_type, report_len);

            if (report_type == HID_REPORT_TYPE_OUTPUT)
            {
                if (s_instance && s_instance->subtype == SubType_KeyboardMouse)
                {
                    // the lock lights, with or without the report id in front
                    if (report_len >= 2 && report_data[0] == KEYBOARD_REPORT_ID)
                        s_instance->set_keyboard_leds(report_data[1]);
                    else if (report_len >= 1)
                        s_instance->set_keyboard_leds(report_data[0]);
                    break;
                }
                if (report_id == ReportIdGamepad && s_instance &&
                    santroller_handle_output_command(*s_instance, report_data, report_len))
                {
                    // rumble / player LED / RGB / stage kit command
                }
                else if (report_id == ReportIdGamepad && s_instance)
                {
                    if (report_len >= 5)
                    {
                        uint8_t left = report_data[4] ? report_data[4] : report_data[1];
                        uint8_t right = report_data[2];
                        s_instance->set_rumble(left, right);
                    }
                    else if (report_len >= 3)
                    {
                        s_instance->set_rumble(report_data[1], report_data[2]);
                    }
                }
                if (report_id == ReportIdSantrollerCapabilities || (report_len > 0 && report_data[0] == ReportIdSantrollerCapabilities))
                {
                    if (s_instance)
                    {
                        uint8_t epin_buf[3];
                        epin_buf[0] = ReportIdSantrollerCapabilities;
                        epin_buf[1] = s_instance->subtype;
                        epin_buf[2] = s_instance->capabilities;
                        send_report(sizeof(epin_buf), epin_buf);
                    }
                }
            }
            break;
        }
        case HIDS_SUBEVENT_CAN_SEND_NOW:
        {
            printf("can send now\r\n");
        }
        default:
            break;
        }
        break;

    default:
        break;
    }
}
