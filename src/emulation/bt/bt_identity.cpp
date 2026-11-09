#include "emulation/bt/bt_identity.h"
#include "btstack.h"
#include "btstack_tlv.h"
#include "ble/le_device_db_tlv.h"
#include "config/device_factory.hpp"
#include <string.h>

// For each LE device db index, the identity the bond was made with. Bonds from before this was stored were made
// as a gamepad, or with a controller (which are recognised by their pairing anyway)
static uint32_t tag_for_device(int index)
{
    return (((uint32_t)'S') << 24) | (((uint32_t)'I') << 16) | (((uint32_t)'D') << 8) | (uint8_t)index;
}

// Bonds made while we were the central (with controllers), visible to every identity
static constexpr uint8_t SHARED_IDENTITY = 0xfe;

static uint8_t s_identity = BtIdentityGamepad;
static bool s_initialized = false;
static btstack_packet_callback_registration_t s_hci_registration;
static btstack_packet_callback_registration_t s_sm_registration;

static bool stored_identity(int index, uint8_t &identity)
{
    const btstack_tlv_t *tlv_impl = nullptr;
    void *tlv_context = nullptr;
    btstack_tlv_get_instance(&tlv_impl, &tlv_context);
    return tlv_impl && tlv_impl->get_tag(tlv_context, tag_for_device(index), &identity, 1) == 1;
}

static void store_identity(int index, uint8_t identity)
{
    const btstack_tlv_t *tlv_impl = nullptr;
    void *tlv_context = nullptr;
    btstack_tlv_get_instance(&tlv_impl, &tlv_context);
    uint8_t stored;
    if (!tlv_impl || index < 0 || (stored_identity(index, stored) && stored == identity))
    {
        return;
    }
    tlv_impl->store_tag(tlv_context, tag_for_device(index), &identity, 1);
}

static bool bond_visible(int index, int addr_type, const bd_addr_t addr)
{
    (void)addr_type;
    // a controller we read is bonded with whatever we advertise as
    if (DeviceFactory::find_bluetooth_pairing_id_by_mac(addr) >= 0)
    {
        return true;
    }
    uint8_t identity = BtIdentityGamepad;
    stored_identity(index, identity);
    return identity == SHARED_IDENTITY || identity == s_identity;
}

// A random static address for the identity: the controller's public address, with the top bits of the first byte
// replaced by the static marker (0b11) and the identity, so each identity of each controller gets its own.
// The fixed identities use 1-3 in those bits (the plain gamepad is public), and gamepad subtypes use 0 with the
// subtype mixed into the next byte.
static void random_address_for(uint8_t identity, bd_addr_t out)
{
    gap_local_bd_addr(out);
    if (identity & BT_IDENTITY_SUBTYPE)
    {
        out[0] = (uint8_t)(0xc0 | (out[0] & 0x0f));
        out[1] ^= (uint8_t)(identity & ~BT_IDENTITY_SUBTYPE);
    }
    else
    {
        out[0] = (uint8_t)(0xc0 | (identity << 4) | (out[0] & 0x0f));
    }
}

// Has to be redone whenever the controller starts, which forgets it (and could be a different USB adapter)
static void apply_random_address()
{
    if (s_identity == BtIdentityGamepad || hci_get_state() != HCI_STATE_WORKING)
    {
        return;
    }
    bd_addr_t addr;
    random_address_for(s_identity, addr);
    hci_le_random_address_set(addr);
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
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
        {
            apply_random_address();
        }
        break;
    case SM_EVENT_PAIRING_COMPLETE: {
        if (sm_event_pairing_complete_get_status(packet) != ERROR_CODE_SUCCESS)
        {
            break;
        }
        hci_con_handle_t handle = sm_event_pairing_complete_get_handle(packet);
        // a host bonded with the identity it connected to, a controller with all of them
        uint8_t identity = gap_get_role(handle) == HCI_ROLE_MASTER ? SHARED_IDENTITY : s_identity;
        store_identity(sm_le_device_index(handle), identity);
        break;
    }
    default:
        break;
    }
}

static void reload_bonds()
{
    le_device_db_tlv_refresh();
    gap_load_resolving_list_from_le_device_db();
}

void bt_identity_init()
{
    if (s_initialized)
    {
        return;
    }
    s_initialized = true;
    s_hci_registration.callback = &packet_handler;
    hci_add_event_handler(&s_hci_registration);
    s_sm_registration.callback = &packet_handler;
    sm_add_event_handler(&s_sm_registration);
    le_device_db_tlv_set_filter(bond_visible);
}

static uint8_t identity_for(SubType subtype, bool config_only)
{
    if (config_only)
    {
        return BtIdentityConfigOnly;
    }
    switch (subtype)
    {
    case SubType_KeyboardMouse:
        return BtIdentityKeyboard;
    case SubType_Dancepad:
        return BtIdentityDancepad;
    case SubType_Gamepad:
    case SubType_Unknown:
        return BtIdentityGamepad;
    default:
        return (uint8_t)(BT_IDENTITY_SUBTYPE | ((uint8_t)subtype & ~BT_IDENTITY_SUBTYPE));
    }
}

void bt_identity_set(SubType subtype, bool config_only)
{
    uint8_t identity = identity_for(subtype, config_only);
    if (identity == s_identity)
    {
        return;
    }
    s_identity = identity;
    // first, as changing the address runs HCI, which then reloads the resolving list too
    reload_bonds();
    if (identity == BtIdentityGamepad)
    {
        gap_advertisements_set_own_address_type(BD_ADDR_TYPE_LE_PUBLIC);
    }
    else
    {
        // btstack holds advertising back until the address is set
        gap_advertisements_set_own_address_type(BD_ADDR_TYPE_LE_RANDOM);
        apply_random_address();
    }
}

void bt_identity_filter_bonds(bool filter)
{
    le_device_db_tlv_set_filter(filter ? bond_visible : nullptr);
    gap_load_resolving_list_from_le_device_db();
}
