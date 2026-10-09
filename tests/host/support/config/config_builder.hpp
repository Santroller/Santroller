#pragma once
// Builds configs with nanopb's encoder, so tests can describe a config as plain structs
#include <cstring>
#include <optional>
#include <utility>
#include <vector>
#include <pb_encode.h>
#include "config.pb.h"

namespace config_test
{
struct DeviceSpec
{
    proto_Device device = proto_Device_init_zero;
    // For a CycleDevice
    std::vector<int32_t> cycle_values;
};

struct ProfileSpec
{
    proto_ProfileOpts opts = proto_ProfileOpts_init_zero;
    // Each inner list is one ProfileAssignment
    std::vector<std::vector<proto_ProfileAssignmentInfo>> assignments;
    std::vector<proto_Mapping> mappings;
    std::vector<proto_Led> leds;
};

struct ConfigSpec
{
    std::vector<DeviceSpec> devices;
    std::vector<ProfileSpec> profiles;
    std::optional<proto_InactivityConfig> inactivity;
    std::optional<proto_PeripheralBootConfig> peripheral_boot;
    std::optional<bool> sync_calibrations;
};

struct AuxSpec
{
    std::vector<std::pair<int32_t, int32_t>> cycles;
    std::vector<std::pair<int32_t, bool>> toggles;
    std::vector<proto_BluetoothPairingState> bluetooth;
    std::vector<proto_BluetoothTlvEntry> tlv;
};

namespace detail
{
template <typename T>
struct Repeated
{
    const std::vector<T> *items;
    const pb_msgdesc_t *fields;
};

template <typename T>
inline bool encode_repeated(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    const auto *repeated = static_cast<const Repeated<T> *>(*arg);
    for (const T &item : *repeated->items)
    {
        if (!pb_encode_tag_for_field(stream, field) || !pb_encode_submessage(stream, repeated->fields, &item))
        {
            return false;
        }
    }
    return true;
}

inline bool encode_int32s(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    for (int32_t value : *static_cast<const std::vector<int32_t> *>(*arg))
    {
        // int32 is a plain varint, sign extended to 64 bits when negative
        if (!pb_encode_tag_for_field(stream, field) ||
            !pb_encode_varint(stream, static_cast<uint64_t>(static_cast<int64_t>(value))))
        {
            return false;
        }
    }
    return true;
}

inline bool encode_devices(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    for (const DeviceSpec &spec : *static_cast<const std::vector<DeviceSpec> *>(*arg))
    {
        proto_Device device = spec.device;
        if (device.which_device == proto_Device_cycle_tag)
        {
            device.device.cycle.values.funcs.encode = encode_int32s;
            device.device.cycle.values.arg = const_cast<std::vector<int32_t> *>(&spec.cycle_values);
        }
        if (!pb_encode_tag_for_field(stream, field) || !pb_encode_submessage(stream, proto_Device_fields, &device))
        {
            return false;
        }
    }
    return true;
}

inline bool encode_opts(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_submessage(stream, proto_ProfileOpts_fields, *arg);
}

inline bool encode_assignments(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    for (const auto &list : *static_cast<const std::vector<std::vector<proto_ProfileAssignmentInfo>> *>(*arg))
    {
        Repeated<proto_ProfileAssignmentInfo> infos{&list, proto_ProfileAssignmentInfo_fields};
        proto_ProfileAssignment assignment = proto_ProfileAssignment_init_zero;
        assignment.assignments.funcs.encode = encode_repeated<proto_ProfileAssignmentInfo>;
        assignment.assignments.arg = &infos;
        if (!pb_encode_tag_for_field(stream, field) ||
            !pb_encode_submessage(stream, proto_ProfileAssignment_fields, &assignment))
        {
            return false;
        }
    }
    return true;
}

inline bool encode_profiles(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    for (const ProfileSpec &spec : *static_cast<const std::vector<ProfileSpec> *>(*arg))
    {
        Repeated<proto_Mapping> mappings{&spec.mappings, proto_Mapping_fields};
        Repeated<proto_Led> leds{&spec.leds, proto_Led_fields};
        proto_Profile profile = proto_Profile_init_zero;
        profile.opts.funcs.encode = encode_opts;
        profile.opts.arg = const_cast<proto_ProfileOpts *>(&spec.opts);
        profile.assignments.funcs.encode = encode_assignments;
        profile.assignments.arg = const_cast<std::vector<std::vector<proto_ProfileAssignmentInfo>> *>(&spec.assignments);
        profile.mappings.funcs.encode = encode_repeated<proto_Mapping>;
        profile.mappings.arg = &mappings;
        profile.leds.funcs.encode = encode_repeated<proto_Led>;
        profile.leds.arg = &leds;
        if (!pb_encode_tag_for_field(stream, field) || !pb_encode_submessage(stream, proto_Profile_fields, &profile))
        {
            return false;
        }
    }
    return true;
}

template <typename Message>
inline std::vector<uint8_t> encode(const pb_msgdesc_t *fields, const Message &message)
{
    pb_ostream_t sizing = PB_OSTREAM_SIZING;
    if (!pb_encode(&sizing, fields, &message))
    {
        return {};
    }
    std::vector<uint8_t> out(sizing.bytes_written);
    pb_ostream_t stream = pb_ostream_from_buffer(out.data(), out.size());
    pb_encode(&stream, fields, &message);
    return out;
}
}

inline std::vector<uint8_t> encode_config(const ConfigSpec &spec)
{
    proto_Config config = proto_Config_init_zero;
    config.devices.funcs.encode = detail::encode_devices;
    config.devices.arg = const_cast<std::vector<DeviceSpec> *>(&spec.devices);
    config.profiles.funcs.encode = detail::encode_profiles;
    config.profiles.arg = const_cast<std::vector<ProfileSpec> *>(&spec.profiles);
    if (spec.inactivity)
    {
        config.has_inactivity = true;
        config.inactivity = *spec.inactivity;
    }
    if (spec.peripheral_boot)
    {
        config.has_peripheralBoot = true;
        config.peripheralBoot = *spec.peripheral_boot;
    }
    if (spec.sync_calibrations)
    {
        config.has_syncCalibrations = true;
        config.syncCalibrations = *spec.sync_calibrations;
    }
    return detail::encode(proto_Config_fields, config);
}

inline std::vector<uint8_t> encode_aux(const AuxSpec &spec)
{
    std::vector<proto_CyclingInputState> cycles;
    for (auto [id, state] : spec.cycles)
    {
        cycles.push_back({id, state});
    }
    std::vector<proto_ToggleInputState> toggles;
    for (auto [id, state] : spec.toggles)
    {
        toggles.push_back({id, state});
    }
    detail::Repeated<proto_CyclingInputState> cycle_list{&cycles, proto_CyclingInputState_fields};
    detail::Repeated<proto_ToggleInputState> toggle_list{&toggles, proto_ToggleInputState_fields};
    detail::Repeated<proto_BluetoothPairingState> bluetooth_list{&spec.bluetooth, proto_BluetoothPairingState_fields};
    detail::Repeated<proto_BluetoothTlvEntry> tlv_list{&spec.tlv, proto_BluetoothTlvEntry_fields};
    proto_AuxConfigBlock block = proto_AuxConfigBlock_init_zero;
    block.states.funcs.encode = detail::encode_repeated<proto_CyclingInputState>;
    block.states.arg = &cycle_list;
    block.toggleStates.funcs.encode = detail::encode_repeated<proto_ToggleInputState>;
    block.toggleStates.arg = &toggle_list;
    block.bluetoothStates.funcs.encode = detail::encode_repeated<proto_BluetoothPairingState>;
    block.bluetoothStates.arg = &bluetooth_list;
    block.tlvEntries.funcs.encode = detail::encode_repeated<proto_BluetoothTlvEntry>;
    block.tlvEntries.arg = &tlv_list;
    return detail::encode(proto_AuxConfigBlock_fields, block);
}

// Shorthands for the messages the tests use most
inline DeviceSpec usb_host_device(int32_t id, int32_t first_pin)
{
    DeviceSpec spec;
    spec.device.deviceid = id;
    spec.device.which_device = proto_Device_usbHost_tag;
    spec.device.device.usbHost.firstPin = first_pin;
    spec.device.device.usbHost.enable5v = true;
    spec.device.device.usbHost.dmFirst = false;
    spec.device.device.usbHost.mappingMode = PerInput;
    return spec;
}

inline DeviceSpec ws2812_device(int32_t id, int32_t pin, int32_t count)
{
    DeviceSpec spec;
    spec.device.deviceid = id;
    spec.device.which_device = proto_Device_ws2812_tag;
    spec.device.device.ws2812.pin = pin;
    spec.device.device.ws2812.type = Ws2812Grb;
    spec.device.device.ws2812.count = count;
    return spec;
}

inline DeviceSpec cycle_device(int32_t id, std::vector<int32_t> values)
{
    DeviceSpec spec;
    spec.device.deviceid = id;
    spec.device.which_device = proto_Device_cycle_tag;
    spec.device.device.cycle.type = custom;
    spec.cycle_values = std::move(values);
    return spec;
}

inline proto_ProfileOpts profile_opts(uint32_t uid, const char *name, SubType subtype)
{
    proto_ProfileOpts opts = proto_ProfileOpts_init_zero;
    opts.uid = uid;
    opts.faceButtonMappingMode = LegendBased;
    strncpy(opts.name, name, sizeof(opts.name) - 1);
    opts.deviceToEmulate = subtype;
    return opts;
}

inline proto_Mapping gpio_button(int32_t pin, GamepadButtonType button)
{
    proto_Mapping mapping = proto_Mapping_init_zero;
    mapping.mapping.which_mapping = proto_Output_gamepadButton_tag;
    mapping.mapping.mapping.gamepadButton = button;
    mapping.input.which_input = proto_Input_gpio_tag;
    mapping.input.input.gpio.pin = pin;
    mapping.input.input.gpio.pinMode = PullUp;
    mapping.input.input.gpio.analog = false;
    return mapping;
}

inline proto_ProfileAssignmentInfo usb_type_assignment(SubType subtype)
{
    proto_ProfileAssignmentInfo info = proto_ProfileAssignmentInfo_init_zero;
    info.which_assignment = proto_ProfileAssignmentInfo_usbType_tag;
    info.assignment.usbType = subtype;
    return info;
}

inline proto_Led gpio_static_led(int32_t pin)
{
    proto_Led led = proto_Led_init_zero;
    led.device.which_device = proto_LedDevice_gpio_tag;
    led.device.device.gpio.pin = pin;
    led.device.device.gpio.analog = false;
    led.mapping.which_led = proto_LedMapping_staticMapping_tag;
    return led;
}

inline proto_BluetoothPairingState bluetooth_pairing(int32_t id, std::initializer_list<uint8_t> mac, const char *name, bool ble)
{
    proto_BluetoothPairingState state = proto_BluetoothPairingState_init_zero;
    state.id = id;
    std::copy(mac.begin(), mac.end(), state.macAddress);
    strncpy(state.name, name, sizeof(state.name) - 1);
    state.ble = ble;
    return state;
}
}
