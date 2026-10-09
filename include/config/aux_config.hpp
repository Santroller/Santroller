#pragma once

// The auxiliary block stored after the main config: the state the firmware saves at runtime (cycle and
// toggle inputs, bluetooth pairings and BTstack's TLV entries) rather than the config tool
#include <stdint.h>
#include <string.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include "config.pb.h"
#include "config/device_factory.hpp"
#include "devices/bt/bt_tlv_storage.hpp"

inline bool decode_cycle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_CyclingInputState proto_cycle;
    auto ret = pb_decode(stream, proto_CyclingInputState_fields, &proto_cycle);
    if (ret)
    {
        DeviceFactory::set_cycle_state(proto_cycle.id, proto_cycle.state);
    }
    return ret;
}
inline bool encode_cycle_input_states(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    proto_CyclingInputState proto_cycle;
    // Stop at the first entry that doesn't fit, so a partial block is never saved
    bool ok = true;
    DeviceFactory::foreach_cycle_state([&](int32_t id, int32_t state)
                                       {
        proto_cycle.id = id;
        proto_cycle.state = state;
        ok = ok && pb_encode_tag_for_field(stream, field) &&
             pb_encode_submessage(stream, proto_CyclingInputState_fields, &proto_cycle); });
    return ok;
}
inline bool decode_toggle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_ToggleInputState proto_toggle;
    auto ret = pb_decode(stream, proto_ToggleInputState_fields, &proto_toggle);
    if (ret)
    {
        DeviceFactory::set_toggle_state(proto_toggle.id, proto_toggle.state);
    }
    return ret;
}
inline bool decode_bluetooth_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_BluetoothPairingState proto_bluetooth = proto_BluetoothPairingState_init_zero;
    auto ret = pb_decode(stream, proto_BluetoothPairingState_fields, &proto_bluetooth);
    if (ret)
    {
        SubType subtype = proto_bluetooth.has_subtype ? proto_bluetooth.subtype : SubType_Gamepad;
        BtControllerType ctrl_type = proto_bluetooth.has_controllerType ? proto_bluetooth.controllerType : BtControllerType_BtControllerTypeGeneric;
        uint16_t vid = proto_bluetooth.has_vid ? proto_bluetooth.vid : 0;
        uint16_t pid = proto_bluetooth.has_pid ? proto_bluetooth.pid : 0;
        const uint8_t *link_key = proto_bluetooth.has_linkKey ? proto_bluetooth.linkKey : nullptr;
        DeviceFactory::set_bluetooth_pairing_state(proto_bluetooth.id, proto_bluetooth.macAddress, proto_bluetooth.name, proto_bluetooth.ble, subtype, ctrl_type, vid, pid, link_key);
    }
    return ret;
}

inline bool decode_bluetooth_tlv_entries(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_BluetoothTlvEntry proto_tlv = proto_BluetoothTlvEntry_init_zero;
    auto ret = pb_decode(stream, proto_BluetoothTlvEntry_fields, &proto_tlv);
    if (ret)
    {
        BtTlvStorage::instance().set_tag_from_config(proto_tlv.tag, proto_tlv.value.bytes, proto_tlv.value.size);
    }
    return ret;
}

inline bool encode_toggle_input_states(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    proto_ToggleInputState proto_toggle;
    // Stop at the first entry that doesn't fit, so a partial block is never saved
    bool ok = true;
    DeviceFactory::foreach_toggle_state([&](int32_t id, bool state)
                                        {
        proto_toggle.id = id;
        proto_toggle.state = state;
        ok = ok && pb_encode_tag_for_field(stream, field) &&
             pb_encode_submessage(stream, proto_ToggleInputState_fields, &proto_toggle); });
    return ok;
}

inline bool encode_bluetooth_states(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    // Stop at the first entry that doesn't fit, so a partial block is never saved
    bool ok = true;
    DeviceFactory::foreach_bluetooth_pairing_state([&](int32_t id, const DeviceFactory::BluetoothPairingStateData &state)
                                                   {
        proto_BluetoothPairingState proto_bluetooth = proto_BluetoothPairingState_init_zero;
        proto_bluetooth.id = id;
        memcpy(proto_bluetooth.macAddress, state.mac, sizeof(proto_bluetooth.macAddress));
        strncpy(proto_bluetooth.name, state.name, sizeof(proto_bluetooth.name) - 1);
        proto_bluetooth.name[sizeof(proto_bluetooth.name) - 1] = '\0';
        proto_bluetooth.ble = state.ble;
        proto_bluetooth.has_subtype = true;
        proto_bluetooth.subtype = state.subtype;
        proto_bluetooth.has_controllerType = true;
        proto_bluetooth.controllerType = state.controller_type;
        proto_bluetooth.has_vid = true;
        proto_bluetooth.vid = state.vid;
        proto_bluetooth.has_pid = true;
        proto_bluetooth.pid = state.pid;
        if (state.has_link_key)
        {
            proto_bluetooth.has_linkKey = true;
            memcpy(proto_bluetooth.linkKey, state.link_key, sizeof(proto_bluetooth.linkKey));
        }
        ok = ok && pb_encode_tag_for_field(stream, field) &&
             pb_encode_submessage(stream, proto_BluetoothPairingState_fields, &proto_bluetooth); });
    return ok;
}

inline bool encode_bluetooth_tlv_entries(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    proto_BluetoothTlvEntry proto_tlv = proto_BluetoothTlvEntry_init_zero;
    // Stop at the first entry that doesn't fit, so a partial block is never saved
    bool ok = true;
    BtTlvStorage::instance().foreach_entry([&](uint32_t tag, const uint8_t *data, uint8_t length)
                                           {
        proto_tlv.tag = tag;
        proto_tlv.value.size = length;
        memcpy(proto_tlv.value.bytes, data, length);
        ok = ok && pb_encode_tag_for_field(stream, field) &&
             pb_encode_submessage(stream, proto_BluetoothTlvEntry_fields, &proto_tlv); });
    return ok;
}

inline bool encode_auxiliary(uint8_t *buffer, uint32_t capacity, uint32_t &written, void *context)
{
    proto_AuxConfigBlock block proto_AuxConfigBlock_init_zero;
    block.states.funcs.encode = encode_cycle_input_states;
    block.toggleStates.funcs.encode = encode_toggle_input_states;
    block.bluetoothStates.funcs.encode = encode_bluetooth_states;
    block.tlvEntries.funcs.encode = encode_bluetooth_tlv_entries;

    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, capacity);
    if (!pb_encode(&outputStream, proto_AuxConfigBlock_fields, &block))
    {
        return false;
    }
    written = outputStream.bytes_written;
    return true;
}
