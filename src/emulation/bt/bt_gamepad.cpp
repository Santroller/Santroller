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
#include "emulation/bt/bt_gatt_changes.h"

// bt_gatt_changes relies on the Generic Attribute service being at the same handles in every database
static_assert(ATT_SERVICE_GATT_SERVICE_START_HANDLE == BT_GATT_SERVICE_START_HANDLE, "GATT service moved");
static_assert(ATT_SERVICE_GATT_SERVICE_END_HANDLE == BT_GATT_SERVICE_END_HANDLE, "GATT service moved");
static_assert(ATT_CHARACTERISTIC_GATT_SERVICE_CHANGED_01_VALUE_HANDLE == BT_GATT_SERVICE_CHANGED_VALUE_HANDLE, "Service Changed moved");
static_assert(ATT_CHARACTERISTIC_GATT_SERVICE_CHANGED_01_CLIENT_CONFIGURATION_HANDLE == BT_GATT_SERVICE_CHANGED_CLIENT_CONFIGURATION_HANDLE, "Service Changed moved");
static_assert(ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE == BT_GAP_DEVICE_NAME_VALUE_HANDLE, "GAP device name moved");
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
// A capabilities request is waiting for HIDS_SUBEVENT_CAN_SEND_NOW
static bool s_capabilities_pending = false;
static BTGamepadDevice *s_instance = nullptr;
static hids_device_report_t report_storage[6];
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void get_report_callback(hci_con_handle_t con_hdl, hid_report_type_t report_type, uint16_t report_id, uint16_t max_report_size, uint8_t * out_report);
// Appearance HID - Keyboard (Category 15, Sub-Category 1)
#define APPEARANCE_KEYBOARD 0x03C1
// Appearance HID - Gamepad (Category 15, Sub-Category 4)
#define APPEARANCE_GAMEPAD 0x03C4
// Appearance HID - Generic (Category 15)
#define APPEARANCE_GENERIC_HID 0x03C0
// What hosts list us as: the name of the profile using bluetooth, otherwise the device name, or SantrollerBT when
// there isn't one
static char s_name[sizeof(Profile::name)] = "SantrollerBT";
// gap_advertisements_set_data and gap_scan_response_set_data keep the pointers, so these must outlive advertising
static uint8_t s_adv_data[LE_ADVERTISING_DATA_SIZE];
static uint8_t s_scan_response[LE_ADVERTISING_DATA_SIZE];
// What set_advertisement last advertised as, so the name can be changed without restarting
static uint16_t s_appearance = APPEARANCE_GENERIC_HID;

static uint8_t add_ad_field(uint8_t *dest, uint8_t pos, uint8_t type, const void *data, uint8_t len)
{
    dest[pos++] = len + 1;
    dest[pos++] = type;
    memcpy(dest + pos, data, len);
    return pos + len;
}

// Advertises as discoverable HID with the given appearance, plus the config service so the configurator can find us
// whatever we're named. Browsers only filter on the advertisement, not the scan response, and that leaves no room for
// the name, so it goes in the scan response, which hosts ask for before listing us (ble_rx waits for it too).
static void set_advertisement(uint16_t appearance)
{
    s_appearance = appearance;
    static const uint8_t flags = 0x06; // general discoverable, BR/EDR not supported
    static const uint8_t uuids[] = {ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8};
    const uint8_t appearance_le[] = {(uint8_t)(appearance & 0xff), (uint8_t)(appearance >> 8)};
    uint8_t adv_len = 0;
    adv_len = add_ad_field(s_adv_data, adv_len, BLUETOOTH_DATA_TYPE_FLAGS, &flags, sizeof(flags));
    adv_len = add_ad_field(s_adv_data, adv_len, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, uuids, sizeof(uuids));
    adv_len = add_ad_field(s_adv_data, adv_len, BLUETOOTH_DATA_TYPE_APPEARANCE, appearance_le, sizeof(appearance_le));
    adv_len = add_ad_field(s_adv_data, adv_len, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS,
                           santroller_config_service_uuid_le, sizeof(santroller_config_service_uuid_le));

    uint8_t name_len = strlen(s_name);
    uint8_t name_room = sizeof(s_scan_response) - 2;
    uint8_t scan_len = add_ad_field(s_scan_response, 0, name_len <= name_room ? BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME : BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME,
                                    s_name, btstack_min(name_len, name_room));
    gap_advertisements_set_data(adv_len, s_adv_data);
    gap_scan_response_set_data(scan_len, s_scan_response);
}

static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    UNUSED(con_handle);
    if (att_handle == BT_GAP_DEVICE_NAME_VALUE_HANDLE)
    {
        return att_read_callback_handle_blob((const uint8_t *)s_name, strlen(s_name), offset, buffer, buffer_size);
    }
    return 0;
}

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
void set_battery_state(uint8_t state)
{
    BtStackLock lock;
    battery = state;
    battery_service_server_set_battery_value(state);
}
BTGamepadDevice::BTGamepadDevice(bool config_only) : m_config_only(config_only)
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
    // The GATT database may be about to change, so drop the host even if it never subscribed to reports
    if (peripheral_handle != HCI_CON_HANDLE_INVALID)
    {
        gap_disconnect(peripheral_handle);
    }
    else if (con_handle != HCI_CON_HANDLE_INVALID)
    {
        gap_disconnect(con_handle);
    }
    con_handle = HCI_CON_HANDLE_INVALID;
    peripheral_handle = HCI_CON_HANDLE_INVALID;
    s_instance = nullptr;
    hids_device_register_get_report_callback(nullptr);
    hids_device_register_packet_handler(nullptr);
    gap_advertisements_enable(0);
    gap_advertisements_set_data(0, nullptr);
    gap_scan_response_set_data(0, nullptr);
    hci_remove_event_handler(&hci_event_callback_registration);
    sm_remove_event_handler(&sm_event_callback_registration);
    // clears every registered service handler, so the next init starts clean, possibly with the other database
    bt_config_service_deinit();
    bt_gatt_changes_deinit();
    att_server_deinit();
    m_initialized = false;
    printf("btgamepaddevice deinit\r\n");
}

// Sets s_name for this instance, returning true if it changed
bool BTGamepadDevice::update_name()
{
    const char *name = nullptr;
    if (!m_config_only && !profiles.empty() && profiles[0]->name[0])
    {
        name = profiles[0]->name;
    }
    else
    {
        name = ConfigManager::instance().get_device_name();
    }
    if (!name)
    {
        name = "SantrollerBT";
    }
    if (strncmp(s_name, name, sizeof(s_name) - 1) == 0)
    {
        return false;
    }
    strncpy(s_name, name, sizeof(s_name) - 1);
    s_name[sizeof(s_name) - 1] = '\0';
    return true;
}

void BTGamepadDevice::refresh_name()
{
    BtStackLock lock;
    if (m_initialized)
    {
        apply_name();
    }
}

void BTGamepadDevice::apply_name()
{
    if (!update_name())
    {
        return;
    }
    // The GAP name is read from s_name, so only the advertisement needs redoing. A connected host keeps the name it
    // has until it reads it again.
    set_advertisement(s_appearance);
    printf("BT: now advertising as %s\r\n", s_name);
}

void BTGamepadDevice::initialize()
{
    BtStackLock lock;
    if (m_initialized)
    {
        // Kept across a reload (ProfileManager::reuse_bt_gamepad_instance), the profile's name may have changed
        apply_name();
        return;
    }
    if (!ConfigManager::instance().has_bluetooth())
    {
        printf("BT: gamepad not started, bluetooth isn't enabled in the config\r\n");
        return;
    }
    printf("btgamepaddevice init%s\r\n", m_config_only ? " (config only)" : "");
    if (!BluetoothStack::instance().begin())
    {
        printf("BT: gamepad not started, the bluetooth stack failed to start\r\n");
        return;
    }
    s_instance = this;
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    // The SM settings are shared with the BLE central (ble_main). No MITM: with no IO it is Just Works either way, but as
    // a central btstack would then re-pair a Just Works bond instead of re-encrypting it, which bonded devices reject
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // setup ATT server. Each mode has its own database (bt.gatt, bt_keyboard.gatt, bt_config.gatt) whose HID
    // service only references the reports in that mode's report map. They're laid out differently, so a
    // bonded host is sent Service Changed when the mode changed since it last connected (bt_gatt_changes).
    uint16_t config_start = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_02_START_HANDLE;
    uint16_t config_end = ATT_SERVICE_ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE_02_END_HANDLE;
    const uint8_t *db = profile_data;
    if (m_config_only)
    {
        db = bt_config_only_profile(&config_start, &config_end);
    }
    else if (subtype == SubType_KeyboardMouse)
    {
        db = bt_keyboard_profile(&config_start, &config_end);
    }
    update_name();
    att_server_init(db, att_read_callback, NULL);
    att_server_register_packet_handler(packet_handler);
    bt_gatt_changes_init();

    // setup battery service
    battery_service_server_init(battery);

    // setup device information service
    device_information_service_server_init();
    device_information_service_server_set_pnp_id(DEVICE_ID_VENDOR_ID_SOURCE_USB, 0x1209, 0x2883, 0);
    char id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2];
    pico_get_unique_board_id_string(id, sizeof(id));
    device_information_service_server_set_serial_number(id);
    memset(report_storage, 0, sizeof(report_storage));
    uint16_t num_reports = sizeof(report_storage) / sizeof(report_storage[0]);
    // the config only database has no gamepad HID service
    switch (m_config_only ? SubType_Unknown : subtype)
    {
    case SubType_Unknown:
        break;
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
    if (!m_config_only)
    {
        hids_device_register_get_report_callback(get_report_callback);
    }

    // lets the configurator talk to us over bluetooth
    bt_config_service_init(config_start, config_end);

    // setup advertisements, every 30ms, or about once a second when only the configurator needs to find us
    uint16_t adv_int_min = m_config_only ? 0x0664 : 0x0030;
    uint16_t adv_int_max = adv_int_min;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);

    switch (m_config_only ? SubType_Unknown : subtype)
    {
    case SubType_Unknown:
        // While only the configurator can connect, show up as a generic HID device rather than a gamepad
        set_advertisement(APPEARANCE_GENERIC_HID);
        break;
    case SubType_KeyboardMouse:
        set_advertisement(APPEARANCE_KEYBOARD);
        break;
    default:
        set_advertisement(APPEARANCE_GAMEPAD);
        break;
    }
    gap_advertisements_enable(1);
    {
        bd_addr_t local_addr;
        gap_local_bd_addr(local_addr);
        printf("BT: advertising as %s from %s, %s %d\r\n", s_name, bd_addr_to_str(local_addr), m_config_only ? "config only, subtype" : "subtype", subtype);
    }

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for SM events
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // register for HIDS
    if (!m_config_only)
    {
        hids_device_register_packet_handler(packet_handler);
    }

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
    case RockBandGuitar:
    {
        XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)&m_initial_report;
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
    // one report per pass, but a media key report that can't go out doesn't hold the mouse back
    if (protocol_mode && m_reports.consumer_pending(consumer) &&
        hids_device_send_input_report_for_id(con_handle, CONSUMER_REPORT_ID, (const uint8_t *)&consumer, sizeof(consumer)) == ERROR_CODE_SUCCESS)
    {
        m_reports.consumer_sent(consumer);
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

bool bt_peripheral_connected()
{
    return peripheral_handle != HCI_CON_HANDLE_INVALID;
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
    if (m_config_only)
    {
        return;
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
        update_stagekit();
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
        s_capabilities_pending = false;
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
                    // We are inside the ATT write callback, and the outgoing buffer is held for the write
                    // response, so a notification sent now is dropped. Answer once btstack can send again.
                    if (s_instance)
                    {
                        s_capabilities_pending = true;
                        hids_device_request_can_send_now_event(hids_subevent_set_report_get_con_handle(packet));
                    }
                }
            }
            break;
        }
        case HIDS_SUBEVENT_CAN_SEND_NOW:
        {
            printf("can send now\r\n");
            if (s_capabilities_pending && s_instance)
            {
                const uint8_t caps[2] = {s_instance->subtype, s_instance->capabilities};
                s_capabilities_pending = false;
                hids_device_send_input_report_for_id(hids_subevent_can_send_now_get_con_handle(packet),
                                                     ReportIdSantrollerCapabilities, caps, sizeof(caps));
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
}
