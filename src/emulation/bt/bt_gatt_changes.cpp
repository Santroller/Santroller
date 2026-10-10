#include "emulation/bt/bt_gatt_changes.h"
#include "btstack.h"
#include "btstack_tlv.h"
#include <string.h>
#include <stdio.h>

// For each bonded host (by LE device db index), the database hash it last discovered
static uint32_t tag_for_device(int index)
{
    return (((uint32_t)'S') << 24) | (((uint32_t)'G') << 16) | (((uint32_t)'C') << 8) | (uint8_t)index;
}

static btstack_packet_callback_registration_t s_hci_registration;
static btstack_packet_callback_registration_t s_sm_registration;
static att_service_handler_t s_service_handler;
static btstack_context_callback_registration_t s_indication_registration;
static bool s_initialized = false;
// the host being told to re-discover, and its device db index
static hci_con_handle_t s_pending_handle = HCI_CON_HANDLE_INVALID;
static int s_pending_index = -1;
static uint16_t s_client_configuration = 0;

static bool stored_hash(int index, uint8_t *hash)
{
    const btstack_tlv_t *tlv_impl = nullptr;
    void *tlv_context = nullptr;
    btstack_tlv_get_instance(&tlv_impl, &tlv_context);
    return tlv_impl && tlv_impl->get_tag(tlv_context, tag_for_device(index), hash, 16) == 16;
}

// Records that the host with this index has discovered the current database
static void remember(int index)
{
    uint8_t hash[16];
    const btstack_tlv_t *tlv_impl = nullptr;
    void *tlv_context = nullptr;
    btstack_tlv_get_instance(&tlv_impl, &tlv_context);
    if (!tlv_impl || index < 0 || !gatt_server_get_database_hash(hash))
    {
        return;
    }
    uint8_t stored[16];
    if (stored_hash(index, stored) && memcmp(stored, hash, sizeof(hash)) == 0)
    {
        return;
    }
    tlv_impl->store_tag(tlv_context, tag_for_device(index), hash, sizeof(hash));
}

static void send_service_changed(void *context)
{
    (void)context;
    if (s_pending_handle == HCI_CON_HANDLE_INVALID)
    {
        return;
    }
    // everything may have moved
    uint8_t range[4];
    little_endian_store_16(range, 0, 0x0001);
    little_endian_store_16(range, 2, 0xffff);
    att_server_indicate(s_pending_handle, BT_GATT_SERVICE_CHANGED_VALUE_HANDLE, range, sizeof(range));
}

// A bonded host is back. If the database changed since it last discovered it, tell it to discover again.
// Its subscription to Service Changed isn't checked, as btstack forgets every subscription when the
// database changes, while the host still considers itself subscribed.
static void check_host(hci_con_handle_t handle)
{
    // already on its way (both encryption events below arrive for the same reconnect)
    if (s_pending_handle == handle)
    {
        return;
    }
    int index = sm_le_device_index(handle);
    uint8_t hash[16];
    uint8_t stored[16];
    if (index < 0 || !gatt_server_get_database_hash(hash))
    {
        return;
    }
    if (stored_hash(index, stored) && memcmp(stored, hash, sizeof(hash)) == 0)
    {
        return;
    }
    printf("BT: GATT database changed since host %d last connected, sending Service Changed\r\n", index);
    s_pending_handle = handle;
    s_pending_index = index;
    s_indication_registration.callback = &send_service_changed;
    s_indication_registration.context = nullptr;
    att_server_request_to_send_indication(&s_indication_registration, handle);
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }
    switch (hci_event_packet_get_type(packet))
    {
    case HCI_EVENT_ENCRYPTION_CHANGE:
        // a host that is still pairing has no index yet, it gets one (and the current database) below
        if (hci_event_encryption_change_get_status(packet) == ERROR_CODE_SUCCESS &&
            hci_event_encryption_change_get_encryption_enabled(packet))
        {
            check_host(hci_event_encryption_change_get_connection_handle(packet));
        }
        break;
    case SM_EVENT_REENCRYPTION_COMPLETE:
        if (sm_event_reencryption_complete_get_status(packet) == ERROR_CODE_SUCCESS)
        {
            check_host(sm_event_reencryption_complete_get_handle(packet));
        }
        break;
    case SM_EVENT_PAIRING_COMPLETE:
        // a new bond discovers everything from scratch
        if (sm_event_pairing_complete_get_status(packet) == ERROR_CODE_SUCCESS)
        {
            remember(sm_le_device_index(sm_event_pairing_complete_get_handle(packet)));
        }
        break;
    case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
        // only once the host has confirmed it, so a dropped connection gets it again next time
        if (att_event_handle_value_indication_complete_get_conn_handle(packet) == s_pending_handle &&
            att_event_handle_value_indication_complete_get_attribute_handle(packet) == BT_GATT_SERVICE_CHANGED_VALUE_HANDLE)
        {
            if (att_event_handle_value_indication_complete_get_status(packet) == ERROR_CODE_SUCCESS)
            {
                remember(s_pending_index);
            }
            s_pending_handle = HCI_CON_HANDLE_INVALID;
            s_pending_index = -1;
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (hci_event_disconnection_complete_get_connection_handle(packet) == s_pending_handle)
        {
            s_pending_handle = HCI_CON_HANDLE_INVALID;
            s_pending_index = -1;
        }
        s_client_configuration = 0;
        break;
    }
}

static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    (void)con_handle;
    if (att_handle == BT_GATT_SERVICE_CHANGED_CLIENT_CONFIGURATION_HANDLE)
    {
        return att_read_callback_handle_little_endian_16(s_client_configuration, offset, buffer, buffer_size);
    }
    return 0;
}

static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    (void)con_handle;
    if (transaction_mode != ATT_TRANSACTION_MODE_NONE)
    {
        return 0;
    }
    if (att_handle == BT_GATT_SERVICE_CHANGED_CLIENT_CONFIGURATION_HANDLE && offset == 0 && buffer_size == 2)
    {
        s_client_configuration = little_endian_read_16(buffer, 0);
    }
    return 0;
}

void bt_gatt_changes_init()
{
    if (s_initialized)
    {
        return;
    }
    s_initialized = true;
    s_pending_handle = HCI_CON_HANDLE_INVALID;
    s_pending_index = -1;
    s_client_configuration = 0;

    s_service_handler.start_handle = BT_GATT_SERVICE_START_HANDLE;
    s_service_handler.end_handle = BT_GATT_SERVICE_END_HANDLE;
    s_service_handler.read_callback = &att_read_callback;
    s_service_handler.write_callback = &att_write_callback;
    s_service_handler.packet_handler = &packet_handler;
    att_server_register_service_handler(&s_service_handler);

    s_hci_registration.callback = &packet_handler;
    hci_add_event_handler(&s_hci_registration);
    s_sm_registration.callback = &packet_handler;
    sm_add_event_handler(&s_sm_registration);
}

void bt_gatt_changes_deinit()
{
    if (!s_initialized)
    {
        return;
    }
    s_initialized = false;
    // att_server_deinit drops the service handler
    hci_remove_event_handler(&s_hci_registration);
    sm_remove_event_handler(&s_sm_registration);
    s_pending_handle = HCI_CON_HANDLE_INVALID;
    s_pending_index = -1;
}
