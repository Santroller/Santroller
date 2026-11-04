#include "config/input_decoder.hpp"
#include "config/input_factory.hpp"
#include "input/cycle.hpp"
#include "input/held.hpp"
#include "input/shifted.hpp"
#include "input/shortcut.hpp"
#include "input/toggle.hpp"

namespace
{
void prepare_child(proto_Input &input, InputDecoder &decoder)
{
    input.cb_input.funcs.decode = InputDecoder::prepare;
    input.cb_input.arg = &decoder;
}

struct ShortcutDecoder
{
    std::shared_ptr<Profile> profile;
    ShortcutInput &shortcut;
};

bool decode_shortcut_child(pb_istream_t *stream, const pb_field_t *,
                           void **arg)
{
    auto &parent = *static_cast<ShortcutDecoder *>(*arg);
    InputDecoder decoder{parent.profile};
    proto_Input input = proto_Input_init_default;
    prepare_child(input, decoder);
    if (!pb_decode_ex(stream, proto_Input_fields, &input, PB_DECODE_NOINIT))
    {
        return false;
    }
    if (auto child = decoder.take(input))
    {
        parent.shortcut.inputs.push_back(std::move(child));
    }
    return true;
}
} // namespace

std::unique_ptr<Input> InputDecoder::take(const proto_Input &input)
{
    proto_Input trigger_config = input;
    std::unique_ptr<Input> decoded;
    switch (input.which_input)
    {
    case proto_Input_held_tag:
    case proto_Input_cycle_tag:
    case proto_Input_toggle_tag:
    case proto_Input_shortcut_tag:
    case proto_Input_shifted_tag:
        decoded = std::move(special);
        break;
    case 0:
        special.reset();
        return nullptr;
    default:
        decoded = InputFactory::create_input(input, profile);
        break;
    }
    if (!decoded)
    {
        return nullptr;
    }
    if ((has_legacy_cycle_trigger || has_legacy_cycle_trigger_value ||
         has_legacy_cycle_max_trigger_value) &&
        !input.has_digitalTrigger && !input.has_digitalTriggerValue &&
        !input.has_digitalMaxTriggerValue)
    {
        trigger_config.has_digitalTrigger = has_legacy_cycle_trigger;
        trigger_config.digitalTrigger = legacy_cycle_trigger;
        trigger_config.has_digitalTriggerValue = has_legacy_cycle_trigger_value;
        trigger_config.digitalTriggerValue = legacy_cycle_trigger_value;
        trigger_config.has_digitalMaxTriggerValue =
            has_legacy_cycle_max_trigger_value;
        trigger_config.digitalMaxTriggerValue = legacy_cycle_max_trigger_value;
    }
    decoded->configure_digital_trigger(
        trigger_config.has_digitalTrigger, trigger_config.digitalTrigger,
        trigger_config.has_digitalTriggerValue,
        trigger_config.digitalTriggerValue,
        trigger_config.has_digitalMaxTriggerValue,
        trigger_config.digitalMaxTriggerValue);
    return decoded;
}

bool InputDecoder::prepare(pb_istream_t *, const pb_field_t *field,
                           void **arg)
{
    switch (field->tag)
    {
    case proto_Input_held_tag:
    case proto_Input_cycle_tag:
    case proto_Input_toggle_tag:
    case proto_Input_shortcut_tag:
    case proto_Input_shifted_tag: {
        auto *callback = static_cast<pb_callback_t *>(field->pData);
        callback->funcs.decode = decode_special;
        callback->arg = *arg;
        break;
    }
    }
    return true;
}

bool InputDecoder::decode_special(pb_istream_t *stream, const pb_field_t *field,
                                  void **arg)
{
    auto &decoder = *static_cast<InputDecoder *>(*arg);
    switch (field->tag)
    {
    case proto_Input_shortcut_tag: {
        auto shortcut = std::make_unique<ShortcutInput>();
        ShortcutDecoder children{decoder.profile, *shortcut};
        proto_ShortcutInput config = proto_ShortcutInput_init_default;
        config.inputs.funcs.decode = decode_shortcut_child;
        config.inputs.arg = &children;
        if (!pb_decode_ex(stream, proto_ShortcutInput_fields, &config,
                          PB_DECODE_NOINIT))
        {
            return false;
        }
        decoder.special = std::move(shortcut);
        return true;
    }
    case proto_Input_held_tag: {
        InputDecoder child{decoder.profile};
        proto_HeldInput config = proto_HeldInput_init_default;
        prepare_child(config.input, child);
        if (!pb_decode_ex(stream, proto_HeldInput_fields, &config,
                          PB_DECODE_NOINIT))
        {
            return false;
        }
        auto held = std::make_unique<HeldInput>();
        held->load(config, child.take(config.input));
        decoder.special = std::move(held);
        return true;
    }
    case proto_Input_cycle_tag: {
        InputDecoder forward{decoder.profile};
        InputDecoder reverse{decoder.profile};
        proto_CycleInput config = proto_CycleInput_init_default;
        prepare_child(config.input, forward);
        prepare_child(config.inputReverse, reverse);
        if (!pb_decode_ex(stream, proto_CycleInput_fields, &config,
                          PB_DECODE_NOINIT))
        {
            return false;
        }
        decoder.has_legacy_cycle_trigger = config.has_trigger;
        decoder.legacy_cycle_trigger = config.trigger;
        decoder.has_legacy_cycle_trigger_value = config.has_triggerValue;
        decoder.legacy_cycle_trigger_value = config.triggerValue;
        decoder.has_legacy_cycle_max_trigger_value = config.has_maxTriggerValue;
        decoder.legacy_cycle_max_trigger_value = config.maxTriggerValue;
        auto cycle = std::make_unique<CycleInput>();
        auto device = decoder.profile->devices.find(config.deviceid);
        // Inactive profiles can refer to a cycle device they haven't claimed.
        if (device != decoder.profile->devices.end())
        {
            cycle->load(config, std::static_pointer_cast<CycleDevice>(device->second),
                        config.has_input ? forward.take(config.input) : nullptr,
                        config.has_inputReverse ? reverse.take(config.inputReverse)
                                                : nullptr);
        }
        decoder.special = std::move(cycle);
        return true;
    }
    case proto_Input_toggle_tag: {
        InputDecoder child{decoder.profile};
        proto_ToggleInput config = proto_ToggleInput_init_default;
        prepare_child(config.input, child);
        if (!pb_decode_ex(stream, proto_ToggleInput_fields, &config,
                          PB_DECODE_NOINIT))
        {
            return false;
        }
        auto toggle = std::make_unique<ToggleInput>();
        auto device = decoder.profile->devices.find(config.deviceid);
        if (device != decoder.profile->devices.end())
        {
            toggle->load(config,
                         std::static_pointer_cast<ToggleDevice>(device->second),
                         config.has_input ? child.take(config.input) : nullptr);
        }
        decoder.special = std::move(toggle);
        return true;
    }
    case proto_Input_shifted_tag: {
        InputDecoder value{decoder.profile};
        InputDecoder shift{decoder.profile};
        proto_ShiftedInput config = proto_ShiftedInput_init_default;
        prepare_child(config.input, value);
        prepare_child(config.shift, shift);
        if (!pb_decode_ex(stream, proto_ShiftedInput_fields, &config,
                          PB_DECODE_NOINIT))
        {
            return false;
        }
        auto shifted = std::make_unique<ShiftedInput>();
        shifted->load(config, config.has_input ? value.take(config.input) : nullptr,
                      config.has_shift ? shift.take(config.shift) : nullptr);
        decoder.special = std::move(shifted);
        return true;
    }
    default:
        PB_RETURN_ERROR(stream, "unknown special input");
    }
}
