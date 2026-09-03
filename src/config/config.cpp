#include "config/config.hpp"
#include "managers/profile_manager.hpp"
#include "managers/device_manager.hpp"
#include "managers/config_manager.hpp"
#include "config/device_factory.hpp"
#include "config/emulation_device_config.hpp"
#include "config/input_factory.hpp"
#include "config/mapping_factory.hpp"
#include "config/trigger_factory.hpp"
#include "config/led_factory.hpp"
#include "config/config_storage.hpp"
#include "config/config_loader.hpp"
#include "input/input.hpp"
#include "input/gpio.hpp"
#include "input/fixed.hpp"
#include "input/wii.hpp"
#include "input/mpr121.hpp"
#include "input/ps2.hpp"
#include "input/midi.hpp"
#include "input/protar_neck.hpp"
#include "input/held.hpp"
#include "input/cycle.hpp"
#include "input/toggle.hpp"
#include "input/usb.hpp"
#include "input/crkd.hpp"
#include "input/crkd_drum.hpp"
#include "input/matrix.hpp"
#include "input/shortcut.hpp"
#include "input/ads1115.hpp"
#include "input/accelerometer.hpp"
#include "input/multiplexer.hpp"
#include "input/vtechexpander.hpp"
#include "input/gh5.hpp"
#include "devices/base.hpp"
#include "devices/accelerometer.hpp"
#include "devices/multiplexer.hpp"
#include "devices/wii.hpp"
#include "devices/bhdrum.hpp"
#include "devices/wtdrum.hpp"
#include "devices/crazyneck.hpp"
#include "devices/debug.hpp"
#include "devices/djh.hpp"
#include "devices/crkd.hpp"
#include "devices/crkd_drum.hpp"
#include "devices/cycle.hpp"
#include "devices/ads1115.hpp"
#include "devices/protar_neck.hpp"
#include "devices/gh5neck.hpp"
#include "devices/usb.hpp"
#include "devices/matrix.hpp"
#include "devices/ps2.hpp"
#include "devices/ps2_emulation.hpp"
#include "devices/wii_emulation.hpp"
#include "devices/max1704x.hpp"
#include "devices/vtechexpander.hpp"
#include "devices/mpr121.hpp"
#include "devices/ws2812.hpp"
#include "devices/apa102.hpp"
#include "devices/midiserial.hpp"
#include "devices/stp16cpc.hpp"
#include "devices/bluetooth.hpp"
#include "devices/dmx.hpp"
#include "mappings/mapping.hpp"
#include "leds/leds.hpp"
#include "tusb.h"
#include "emulation/usb/xinput_device.h"
#include "emulation/usb/ogxbox_device.h"
#include "emulation/usb/xone_device.h"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/ps3_device.h"
#include "emulation/usb/ps4_device.h"
#include "emulation/usb/ps5_device.h"
#include "emulation/usb/gh_arcade_device.h"
#include "emulation/usb/switch_device.h"
#include "emulation/bt/bt_gamepad.h"
#include "devices/usb/host/host.hpp"
#include "emulation/wii_emulation.hpp"
#include "emulation/ps2_emulation.hpp"
#include "emulation/usb/usb_descriptors.h"
#include "hardware/watchdog.h"
#include "main.hpp"
#include "host/usbh.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"
#include "quadrature_encoder.h"
#include <vector>
#include <set>
#include <memory>
#include <algorithm>
#include "utils.h"
#include "hci.h"

static auto& profile_mgr = ProfileManager::instance();
static auto& device_mgr = DeviceManager::instance();
static auto& config_mgr = ConfigManager::instance();
static ConfigStorage config_storage;

ConsoleMode mode = ModeHid;
ConsoleMode newMode = mode;

struct ConfigDecodeContext
{
    std::shared_ptr<Profile> profile;
    const EmulationDeviceConfig *emulation_devices = nullptr;
    ShortcutInput *last_shortcut = nullptr;
    Input *last_special = nullptr;
};

bool load_cycle_state(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint32_t state;
    auto ret = pb_decode_varint32(stream, &state);
    DeviceFactory::add_last_cycle_state(state);
    return ret;
}
bool load_device_dev(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    if (field->tag == proto_Device_cycle_tag)
    {
        proto_CycleDevice *msg = (proto_CycleDevice *)field->pData;
        msg->values.funcs.decode = load_cycle_state;
        msg->values.arg = *arg;
    }
    return true;
}
bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *emulation_devices = static_cast<EmulationDeviceConfig *>(arg ? *arg : nullptr);
    proto_Device proto_device proto_Device_init_zero;
    proto_device.cb_device.funcs.decode = load_device_dev;
    proto_device.cb_device.arg = arg ? *arg : nullptr;
    pb_decode(stream, proto_Device_fields, &proto_device);
    
    if (emulation_devices)
    {
        if (proto_device.which_device == proto_Device_psxEmulation_tag)
        {
            emulation_devices->psx = proto_device.device.psxEmulation;
            emulation_devices->has_psx = true;
        }
        else if (proto_device.which_device == proto_Device_wiiEmulation_tag)
        {
            emulation_devices->wii = proto_device.device.wiiEmulation;
            emulation_devices->has_wii = true;
        }
    }
    
    auto device_id = proto_device.deviceid;
    DeviceReloadState previous_state;
    auto previous_device = device_mgr.get_root_device(device_id);
    if (previous_device)
    {
        previous_device->save_reload_state(previous_state);
        previous_device->end(false);
        device_mgr.remove_root_device(device_id);
        previous_device.reset();
    }
    
    auto device = DeviceFactory::create_device(
        proto_device,
        device_id,
        previous_state.valid ? &previous_state : nullptr
    );
    if (!device)
    {
        return false;
    }
    
    device_mgr.set_root_device(device_id, device);
    device_mgr.add_active_device(device);
    device->still_connected = true;
    device->begin();
    device->rescan(true);
    return true;
}
std::unique_ptr<Input> make_input(proto_Input input, ConfigDecodeContext &context, pb_istream_t *stream)
{
    if (input.which_input == proto_Input_held_tag ||
        input.which_input == proto_Input_cycle_tag ||
        input.which_input == proto_Input_toggle_tag ||
        input.which_input == proto_Input_shortcut_tag ||
        input.which_input == 0)
    {
        auto ret = context.last_special;
        context.last_special = nullptr;
        context.last_shortcut = nullptr;
        return std::unique_ptr<Input>(ret);
    }
    
    return InputFactory::create_input(input, context.profile);
}

bool load_input_dev(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool load_shortcut_input(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    proto_Input input;
    if (!pb_decode(stream, proto_Input_fields, &input))
    {
        // printf("couldnt decode shortcut input?\r\n");
        return false;
    }
    auto inputPtr = make_input(input, *context, stream);
    if (!inputPtr)
    {
        return true;
    }
    context->last_shortcut->inputs.push_back(std::move(inputPtr));
    // printf("shortcut added: %d\r\n", context->last_shortcut->inputs.size());
    return true;
}
bool load_shortcut(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    // printf("found shortcut!\r\n");
    context->last_shortcut = new ShortcutInput();
    context->last_special = context->last_shortcut;
    proto_ShortcutInput input;
    input.inputs.funcs.decode = &load_shortcut_input;
    input.inputs.arg = *arg;
    if (!pb_decode(stream, proto_ShortcutInput_fields, &input))
    {
        // printf("couldnt decode shortcut input?\r\n");
        return false;
    }
    // printf("loaded shortcut\r\n");
    return true;
}
bool load_held(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    // printf("found held!\r\n");
    auto last_held = new HeldInput();
    context->last_special = last_held;
    proto_HeldInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    input.input.cb_input.arg = *arg;
    if (!pb_decode(stream, proto_HeldInput_fields, &input))
    {
        // printf("couldnt decode held input?\r\n");
        return false;
    }
    last_held->load(input, make_input(input.input, *context, stream));
    return true;
}
bool load_cycle(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    // printf("found cycle! %p\r\n", profile.get());
    auto last_cycle = new CycleInput();
    context->last_special = last_cycle;
    proto_CycleInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    input.input.cb_input.arg = *arg;
    input.inputReverse.cb_input.funcs.decode = load_input_dev;
    input.inputReverse.cb_input.arg = *arg;
    if (!pb_decode(stream, proto_CycleInput_fields, &input))
    {
        // printf("couldnt decode cycle input?\r\n");
        return false;
    }

    // printf("check %d %d\r\n", input.deviceid, profile->devices.size());
    if (profile->devices.find(input.deviceid) == profile->devices.end())
    {
        // printf("why tho\r\n");
        return true;
    }
    // printf("loading cycle\r\n");
    last_cycle->load(input, std::static_pointer_cast<CycleDevice>(profile->devices[input.deviceid]), input.has_input ? make_input(input.input, *context, stream) : nullptr, input.has_inputReverse ? make_input(input.inputReverse, *context, stream) : nullptr);
    // printf("loaded cycle\r\n");
    return true;
}
bool load_toggle(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    // printf("found toggle! %p\r\n", profile.get());
    auto last_toggle = new ToggleInput();
    context->last_special = last_toggle;
    proto_ToggleInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    input.input.cb_input.arg = *arg;
    if (!pb_decode(stream, proto_ToggleInput_fields, &input))
    {
        // printf("couldnt decode toggle input?\r\n");
        return false;
    }

    // printf("check %d %d\r\n", input.deviceid, profile->devices.size());
    if (profile->devices.find(input.deviceid) == profile->devices.end())
    {
        // printf("why tho\r\n");
        return true;
    }
    // printf("loading toggle\r\n");
    last_toggle->load(input, std::static_pointer_cast<ToggleDevice>(profile->devices[input.deviceid]), input.has_input ? make_input(input.input, *context, stream) : nullptr);
    // printf("loaded toggle\r\n");
    return true;
}

bool load_input_dev(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    // //printf("input_dev: %d %p\r\n", field->tag, profile.get());

    if (field->tag == proto_Input_cycle_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_cycle;
        msg->arg = *arg;
    }
    if (field->tag == proto_Input_toggle_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_toggle;
        msg->arg = *arg;
    }
    if (field->tag == proto_Input_held_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_held;
        msg->arg = *arg;
    }
    if (field->tag == proto_Input_shortcut_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_shortcut;
        msg->arg = *arg;
    }
    return true;
}
bool load_mapping(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    proto_Mapping proto_mapping;
    proto_mapping.input.cb_input.funcs.decode = load_input_dev;
    proto_mapping.input.cb_input.arg = *arg;
    pb_decode(stream, proto_Mapping_fields, &proto_mapping);
    
    std::unique_ptr<Input> input = make_input(proto_mapping.input, *context, stream);
    size_t mapping_id = profile->mappings.size();
    
    auto mapping = MappingFactory::create_mapping(proto_mapping, profile, std::move(input), mapping_id);
    if (mapping)
    {
        profile->mappings.push_back(std::move(mapping));
    }
    return true;
}

bool load_assignment_dev(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    // printf("load_assignment_dev: %d %p\r\n", field->tag, profile.get());
    proto_ProfileAssignmentInfo *info = (proto_ProfileAssignmentInfo *)field->message;
    if (field->tag == proto_ProfileAssignmentInfo_input_tag)
    {
        info->assignment.input.input.cb_input.funcs.decode = load_input_dev;
        info->assignment.input.input.cb_input.arg = *arg;
    }
    if (field->tag == proto_ProfileAssignmentInfo_inputAnyTime_tag)
    {
        info->assignment.inputAnyTime.input.cb_input.funcs.decode = load_input_dev;
        info->assignment.inputAnyTime.input.cb_input.arg = *arg;
    }
    return true;
}
bool load_assignment_info(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    auto &list = profile->triggers.back();
    proto_ProfileAssignmentInfo proto_assignment;
    proto_assignment.cb_assignment.funcs.decode = load_assignment_dev;
    proto_assignment.cb_assignment.arg = *arg;
    pb_decode(stream, proto_ProfileAssignmentInfo_fields, &proto_assignment);
    
    // Get input for input-based triggers
    std::unique_ptr<Input> input;
    if (proto_assignment.which_assignment == proto_ProfileAssignmentInfo_input_tag)
    {
        input = make_input(proto_assignment.assignment.input.input, *context, stream);
    }
    else if (proto_assignment.which_assignment == proto_ProfileAssignmentInfo_inputAnyTime_tag)
    {
        input = make_input(proto_assignment.assignment.inputAnyTime.input, *context, stream);
    }
    
    auto trigger = TriggerFactory::create_trigger(
        proto_assignment,
        profile,
        std::move(input),
        list->triggers.size(),
        profile->triggers.size() - 1
    );
    
    if (trigger)
    {
        list->triggers.push_back(std::move(trigger));
    }
    
    return true;
}

bool load_assignments(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    // printf("load_assignments: %p\r\n", profile.get());
    auto list = new ActivationTriggerList();
    profile->triggers.emplace_back(list);
    proto_ProfileAssignment proto_assignment;
    proto_assignment.assignments.funcs.decode = &load_assignment_info;
    proto_assignment.assignments.arg = *arg;
    // printf("load_assignments start?\r\n");
    pb_decode(stream, proto_ProfileAssignment_fields, &proto_assignment);
    // printf("load_assignments done?\r\n");
    list->validate(true, false, false);
    return true;
}
bool load_leds(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    proto_Led proto_led;
    proto_led.mapping.led.inputMapping.input.cb_input.funcs.decode = load_input_dev;
    proto_led.mapping.led.inputMapping.input.cb_input.arg = *arg;
    pb_decode(stream, proto_Led_fields, &proto_led);
    
    auto device = LedFactory::create_led_device(proto_led.device, profile);
    if (!device)
    {
        return false;
    }
    
    // Get input for input-based LED mappings
    std::unique_ptr<Input> input;
    if (proto_led.mapping.which_led == proto_LedMapping_inputMapping_tag)
    {
        input = make_input(proto_led.mapping.led.inputMapping.input, *context, stream);
    }
    
    auto led_mapping = LedFactory::create_led_mapping(
        proto_led.mapping,
        profile,
        std::move(device),
        std::move(input),
        profile->leds.size()
    );
    
    if (led_mapping)
    {
        profile->leds.push_back(std::move(led_mapping));
    }
    
    return true;
}
bool load_opts(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *context = static_cast<ConfigDecodeContext *>(*arg);
    auto profile = context->profile;
    // printf("load_uid: %p\r\n", profile.get());
    proto_ProfileOpts opts;
    if (!pb_decode(stream, proto_ProfileOpts_fields, &opts))
        return false;
    profile->profile_id = opts.uid;
    memcpy(profile->name, opts.name, sizeof(profile->name));
    profile->xinput_on_windows = opts.has_xinputOnWindows && opts.xinputOnWindows;
    profile->invert_y_axis_hid = opts.has_invertYAxisHid && opts.invertYAxisHid;
    profile->supports_ps4 = opts.has_ps4OrPs5Mode && opts.ps4OrPs5Mode;
    profile->supports_slider = opts.has_supportsSlider && opts.supportsSlider;
    profile->cymbal_glitch_fix = opts.has_cymbalGlitchFix && opts.cymbalGlitchFix;
    profile->subtype = opts.deviceToEmulate;
    ProfileManager::instance().add_profile(profile->profile_id, profile);
    return true;
}
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    // printf("load_profile\r\n");
    auto *emulation_devices = static_cast<EmulationDeviceConfig *>(arg ? *arg : nullptr);
    if (!emulation_devices)
    {
        return false;
    }
    // Snapshot this profile submessage's bytes (config is decoded from an in-memory
    // buffer, so this is just a cheap copy of the stream cursor) so it can be
    // re-decoded once per physical device combo that matches its triggers. That
    // gives each matching device (e.g. two identical USB guitars) its own bound
    // Profile/Instance instead of them fighting over one shared one.
    pb_istream_t profile_bytes = *stream;

    while (true)
    {
        auto profile = std::make_shared<Profile>();
        device_mgr.for_each_active_device([profile](const auto &device)
        {
            profile->devices.emplace(device->m_id, device);
            // printf("load device: %p %p %d\r\n", profile.get(), device.get(), device->m_id);
        });
        ConfigDecodeContext context{profile, emulation_devices};
        proto_Profile proto_profile;
        memset(&proto_profile, 0, sizeof(proto_profile));
        proto_profile.assignments.funcs.decode = &load_assignments;
        proto_profile.assignments.arg = &context;
        proto_profile.mappings.funcs.decode = &load_mapping;
        proto_profile.mappings.arg = &context;
        proto_profile.opts.funcs.decode = &load_opts;
        proto_profile.opts.arg = &context;
        proto_profile.leds.funcs.decode = &load_leds;
        proto_profile.leds.arg = &context;
        pb_istream_t decode_stream = profile_bytes;
        pb_decode(&decode_stream, proto_Profile_fields, &proto_profile);

        // Validate triggers and assign profile to devices - every list that matches
        // (not just the first) gets activated, so one profile can drive multiple devices at once.
        bool matched = false;
        for (auto &list : profile->triggers)
        {
            if (list->validate(true, false, false))
            {
                matched = true;
                int assignedDevices = list->assignedDevices();
                ConsoleMode usb_mode = config_mgr.get_requested_mode();
                ConsoleMode forced_usb_mode;
                if (list->forcedConsoleMode(forced_usb_mode))
                {
                    printf("setting requested mode: %d, old: %d\r\n", forced_usb_mode, config_mgr.get_requested_mode());
                    config_mgr.request_mode(forced_usb_mode);
                    usb_mode = forced_usb_mode;
                }
                printf("profile assigned! profile_id=%d\r\n", profile->profile_id);
                
                // Track subtype changes
                profile_mgr.track_profile_type(profile->profile_id, profile->subtype);
                
                // Assign profile to appropriate devices
                profile_mgr.assign_profile_to_devices(profile, assignedDevices, usb_mode, *context.emulation_devices);
            }
        }

        if (!matched)
        {
            break;
        }
        // loop again: other still-assignable devices may satisfy this profile too
    }
    return true;
}
bool load_empty()
{
    config_storage.initialize_empty();

    auto confDevice2 = HIDConfigDevice::instance;
    confDevice2->interface_id = profile_mgr.instance_count();
    profile_mgr.add_instance(confDevice2);
    // HIDConfigDevice is a special singleton instance, not profile-based
    profile_mgr.set_usb_instance(confDevice2->interface_id, confDevice2);
    confDevice2->initialize();
    return true;
}

bool decode_cycle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_CyclingInputState proto_cycle;
    auto ret = pb_decode(stream, proto_CyclingInputState_fields, &proto_cycle);
    DeviceFactory::set_cycle_state(proto_cycle.id, proto_cycle.state);
    return ret;
}
bool encode_cycle_input_states(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    proto_CyclingInputState proto_cycle;
    DeviceFactory::foreach_cycle_state([&](int32_t id, int32_t state) {
        proto_cycle.id = id;
        proto_cycle.state = state;
        pb_encode_tag_for_field(stream, field);
        pb_encode_submessage(stream, proto_CyclingInputState_fields, &proto_cycle);
    });
    return true;
}
bool decode_toggle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_ToggleInputState proto_toggle;
    auto ret = pb_decode(stream, proto_ToggleInputState_fields, &proto_toggle);
    DeviceFactory::set_toggle_state(proto_toggle.id, proto_toggle.state);
    return ret;
}
bool decode_bluetooth_states(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_BluetoothPairingState proto_bluetooth;
    auto ret = pb_decode(stream, proto_BluetoothPairingState_fields, &proto_bluetooth);
    
    return ret;
}
bool encode_toggle_input_states(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    proto_ToggleInputState proto_toggle;
    DeviceFactory::foreach_toggle_state([&](int32_t id, bool state) {
        proto_toggle.id = id;
        proto_toggle.state = state;
        pb_encode_tag_for_field(stream, field);
        pb_encode_submessage(stream, proto_ToggleInputState_fields, &proto_toggle);
    });
    return true;
}

bool encode_auxiliary(uint8_t *buffer, uint32_t capacity, uint32_t &written, void *context)
{
    proto_AuxConfigBlock block proto_AuxConfigBlock_init_zero;
    block.states.funcs.encode = encode_cycle_input_states;
    block.toggleStates.funcs.encode = encode_toggle_input_states;

    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, capacity);
    if (!pb_encode(&outputStream, proto_AuxConfigBlock_fields, &block))
    {
        return false;
    }
    written = outputStream.bytes_written;
    return true;
}

uint32_t copy_config_info(uint8_t *buffer)
{
    ConfigMetadata metadata = config_storage.read_metadata(false);
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    info.dataCrc = metadata.data_crc;
    info.dataSize = metadata.data_size;
    info.magic = metadata.magic;
    info.mainSize = metadata.main_size;
    info.auxSize = metadata.aux_size;
    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 64);
    if (!pb_encode(&outputStream, proto_ConfigInfo_fields, &info))
    {
        return 0;
    }
    return outputStream.bytes_written;
}

void reload()
{
    printf("reload called\r\n");
    config_mgr.schedule_reinit(millis());
}

void update_aux_cycle(uint32_t id, uint32_t state)
{
    if (!config_mgr.has_loaded_any())
    {
        return;
    }
    DeviceFactory::set_cycle_state(id, state);
    config_storage.update_auxiliary(encode_auxiliary);
}

void update_aux_toggle(uint32_t id, bool state)
{
    // printf("update aux: %d %d %d\r\n", id, state, config_mgr.get_reinit_time());
    if (config_mgr.get_reinit_time())
    {
        return;
    }
    DeviceFactory::set_toggle_state(id, state);
    config_storage.update_auxiliary(encode_auxiliary);
}

bool write_config_info(const uint8_t *buffer, uint16_t bufsize)
{
    return config_storage.write_info(buffer, bufsize);
}

bool write_config(const uint8_t *buffer, uint16_t bufsize, uint32_t start)
{
    auto result = config_storage.write_chunk(buffer, bufsize, start);
    if (result == ConfigStorage::WriteResult::InProgress)
    {
        // //printf("writing up to: %d < %d\r\n", start + bufsize, footer.dataSize);
        config_mgr.set_working(true);
        return true;
    }
    if (result == ConfigStorage::WriteResult::Invalid)
    {
        // printf("Crc didnt match after writing? %d\r\n", footer.dataCrc);
        return false;
    }
    printf("Everything matched, saving!\r\n");
    reload();
    config_mgr.set_working(false);
    return true;
}

uint32_t copy_config(uint8_t *buffer, uint32_t start)
{
    ConfigImage image;
    if (!config_storage.read_flash(image))
    {
        return 0;
    }
    const uint32_t remaining = image.data_size - start;
    const uint32_t size = remaining > 63 ? 63 : remaining;
    memcpy(buffer, image.data + start, size);
    return size;
}
bool load()
{
    ConfigImage image;
    if (!config_storage.read_cached(image))
    {
        return false;
    }

    const bool loaded = ConfigLoader::apply(image, config_mgr.get_current_mode());
    config_mgr.set_loaded_any(loaded);
    return loaded;
}