#include "config/config.hpp"
#include "managers/profile_manager.hpp"
#include "managers/device_manager.hpp"
#include "managers/config_manager.hpp"
#include "config/device_factory.hpp"
#include "config/input_factory.hpp"
#include "config/mapping_factory.hpp"
#include "config/trigger_factory.hpp"
#include "config/led_factory.hpp"
#include "input/input.hpp"
#include "input/gpio.hpp"
#include "input/fixed.hpp"
#include "input/wii.hpp"
#include "input/mpr121.hpp"
#include "input/ps2.hpp"
#include "input/midi.hpp"
#include "input/protarNeck.hpp"
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

ConsoleMode mode = ModeHid;
ConsoleMode newMode = mode;

// Protobuf callback state
std::shared_ptr<Profile> working_profile;
bool working = false;
bool loadedAny = false;
bool modeChanged = false;

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
    }
    return true;
}
bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_Device proto_device proto_Device_init_zero;
    proto_device.cb_device.funcs.decode = load_device_dev;
    pb_decode(stream, proto_Device_fields, &proto_device);
    
    auto device_id = proto_device.deviceid;
    auto prev_device = device_mgr.get_root_device(device_id);
    if (prev_device)
    {
        prev_device->end(false);
    }
    
    auto device = DeviceFactory::create_device(proto_device, device_id, prev_device);
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
ShortcutInput *last_shortcut = nullptr;
Input *last_special = nullptr;

std::unique_ptr<Input> make_input(proto_Input input, std::shared_ptr<Profile> profile, pb_istream_t *stream)
{
    if (input.which_input == proto_Input_held_tag ||
        input.which_input == proto_Input_cycle_tag ||
        input.which_input == proto_Input_toggle_tag ||
        input.which_input == proto_Input_shortcut_tag ||
        input.which_input == 0)
    {
        auto ret = last_special;
        last_special = nullptr;
        last_shortcut = nullptr;
        return std::unique_ptr<Input>(ret);
    }
    
    return InputFactory::create_input(input, profile);
}

bool load_input_dev(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool load_shortcut_input(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    proto_Input input;
    if (!pb_decode(stream, proto_Input_fields, &input))
    {
        // printf("couldnt decode shortcut input?\r\n");
        return false;
    }
    auto inputPtr = make_input(input, profile, stream);
    if (!inputPtr)
    {
        return true;
    }
    last_shortcut->inputs.push_back(std::move(inputPtr));
    // printf("shortcut added: %d\r\n", last_shortcut->inputs.size());
    return true;
}
bool load_shortcut(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    // printf("found shortcut!\r\n");
    last_shortcut = new ShortcutInput();
    last_special = last_shortcut;
    proto_ShortcutInput input;
    input.inputs.funcs.decode = &load_shortcut_input;
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
    auto profile = working_profile;
    // printf("found held!\r\n");
    auto last_held = new HeldInput();
    last_special = last_held;
    proto_HeldInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    if (!pb_decode(stream, proto_HeldInput_fields, &input))
    {
        // printf("couldnt decode held input?\r\n");
        return false;
    }
    last_held->load(input, make_input(input.input, profile, stream));
    return true;
}
bool load_cycle(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    // printf("found cycle! %p\r\n", profile.get());
    auto last_cycle = new CycleInput();
    last_special = last_cycle;
    proto_CycleInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
    input.inputReverse.cb_input.funcs.decode = load_input_dev;
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
    last_cycle->load(input, std::static_pointer_cast<CycleDevice>(profile->devices[input.deviceid]), input.has_input ? make_input(input.input, profile, stream) : nullptr, input.has_inputReverse ? make_input(input.inputReverse, profile, stream) : nullptr);
    // printf("loaded cycle\r\n");
    return true;
}
bool load_toggle(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    // printf("found toggle! %p\r\n", profile.get());
    auto last_toggle = new ToggleInput();
    last_special = last_toggle;
    proto_ToggleInput input;
    input.input.cb_input.funcs.decode = load_input_dev;
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
    last_toggle->load(input, std::static_pointer_cast<ToggleDevice>(profile->devices[input.deviceid]), input.has_input ? make_input(input.input, profile, stream) : nullptr);
    // printf("loaded toggle\r\n");
    return true;
}

bool load_input_dev(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    // //printf("input_dev: %d %p\r\n", field->tag, profile.get());

    if (field->tag == proto_Input_cycle_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_cycle;
    }
    if (field->tag == proto_Input_toggle_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_toggle;
    }
    if (field->tag == proto_Input_held_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_held;
    }
    if (field->tag == proto_Input_shortcut_tag)
    {
        pb_callback_t *msg = (pb_callback_t *)field->pData;
        msg->funcs.decode = &load_shortcut;
    }
    return true;
}
bool load_mapping(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    proto_Mapping proto_mapping;
    proto_mapping.input.cb_input.funcs.decode = load_input_dev;
    pb_decode(stream, proto_Mapping_fields, &proto_mapping);
    
    std::unique_ptr<Input> input = make_input(proto_mapping.input, profile, stream);
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
    auto profile = working_profile;
    // printf("load_assignment_dev: %d %p\r\n", field->tag, profile.get());
    proto_ProfileAssignmentInfo *info = (proto_ProfileAssignmentInfo *)field->message;
    if (field->tag == proto_ProfileAssignmentInfo_input_tag)
    {
        info->assignment.input.input.cb_input.funcs.decode = load_input_dev;
    }
    if (field->tag == proto_ProfileAssignmentInfo_inputAnyTime_tag)
    {
        info->assignment.inputAnyTime.input.cb_input.funcs.decode = load_input_dev;
    }
    return true;
}
bool load_assignment_info(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    auto &list = profile->triggers.back();
    proto_ProfileAssignmentInfo proto_assignment;
    proto_assignment.cb_assignment.funcs.decode = load_assignment_dev;
    pb_decode(stream, proto_ProfileAssignmentInfo_fields, &proto_assignment);
    
    // Get input for input-based triggers
    std::unique_ptr<Input> input;
    if (proto_assignment.which_assignment == proto_ProfileAssignmentInfo_input_tag)
    {
        input = make_input(proto_assignment.assignment.input.input, profile, stream);
    }
    else if (proto_assignment.which_assignment == proto_ProfileAssignmentInfo_inputAnyTime_tag)
    {
        input = make_input(proto_assignment.assignment.inputAnyTime.input, profile, stream);
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
    auto profile = working_profile;
    // printf("load_assignments: %p\r\n", profile.get());
    auto list = new ActivationTriggerList();
    profile->triggers.emplace_back(list);
    proto_ProfileAssignment proto_assignment;
    proto_assignment.assignments.funcs.decode = &load_assignment_info;
    // printf("load_assignments start?\r\n");
    pb_decode(stream, proto_ProfileAssignment_fields, &proto_assignment);
    // printf("load_assignments done?\r\n");
    list->validate(true, false, false);
    return true;
}
bool load_leds(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = working_profile;
    proto_Led proto_led;
    proto_led.mapping.led.inputMapping.input.cb_input.funcs.decode = load_input_dev;
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
        input = make_input(proto_led.mapping.led.inputMapping.input, profile, stream);
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
    auto profile = working_profile;
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
    profile->drum_state.cymbalGlitchFix = opts.has_cymbalGlitchFix && opts.cymbalGlitchFix;
    profile->subtype = opts.deviceToEmulate;
    ProfileManager::instance().add_profile(profile->profile_id, profile);
    return true;
}
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    // printf("load_profile\r\n");
    auto profile = std::make_shared<Profile>();
    device_mgr.for_each_active_device([profile](const auto &device)
    {
        profile->devices.emplace(device->m_id, device);
        // printf("load device: %p %p %d\r\n", profile.get(), device.get(), device->m_id);
    });
    working_profile = profile;
    proto_Profile proto_profile;
    memset(&proto_profile, 0, sizeof(proto_profile));
    proto_profile.assignments.funcs.decode = &load_assignments;
    proto_profile.mappings.funcs.decode = &load_mapping;
    proto_profile.opts.funcs.decode = &load_opts;
    proto_profile.leds.funcs.decode = &load_leds;
    pb_decode(stream, proto_Profile_fields, &proto_profile);
    
    // Validate triggers and assign profile to devices
    for (auto &list : profile->triggers)
    {
        if (list->validate(true, false, false))
        {
            int assignedDevices = list->assignedDevices();
            printf("profile assigned! profile_id=%d\r\n", profile->profile_id);
            
            // Track subtype changes
            profile_mgr.track_profile_type(profile->profile_id, profile->subtype);
            
            // Assign profile to appropriate devices
            profile_mgr.assign_profile_to_devices(profile, assignedDevices);
            break;
        }
    }
    working_profile = nullptr;
    return true;
}
struct __attribute__((packed)) ConfigFooter
{
    uint32_t dataSize;
    uint32_t dataCrc;
    uint32_t mainSize;
    uint32_t auxSize;
    uint32_t magic;
    uint32_t currentProfile;

    bool operator==(const ConfigFooter &other) const
    {
        return dataSize == other.dataSize &&
               dataCrc == other.dataCrc &&
               magic == other.magic;
    }
};

static const uint32_t FOOTER_MAGIC = 0xd2f1e365;
bool load_empty()
{

    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));

    footer->dataSize = 0;
    footer->mainSize = 0;
    footer->auxSize = 0;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, 0);
    footer->magic = FOOTER_MAGIC;
    footer->currentProfile = 0;
    EEPROM.commit();

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
bool inner_load(const uint32_t currentProfile, const uint8_t *dataPtr, uint32_t size, uint32_t mainSize, uint32_t auxSize)
{

    proto_Config config proto_Config_init_zero;
    // printf("inner_load\r\n");
    DeviceFactory::clear_cycle_states();
    DeviceFactory::clear_toggle_states();
    // We are now sufficiently confident that the data is valid so we run the deserialization
    // load just the current profile to begin with
    pb_istream_t inputStream = pb_istream_from_buffer(dataPtr, mainSize);
    device_mgr.clear_assignable_devices();

    config.devices.funcs.decode = &load_device;
    config.profiles.funcs.decode = &load_profile;
    // gui only, dont need to decode
    config.guiConfig.funcs.decode = nullptr;
    config_mgr.clear_seen_masks();
    device_mgr.clear_active_devices();
    device_mgr.mark_root_devices_disconnected();
    profile_mgr.prepare_for_config_reload();
    UsbDevice::reset_ep();
    switch (mode)
    {
    case ModeOgXbox:
    case ModeXboxOne:
    case ModeWiiRb:
    case ModePs3:
    case ModePs4:
    case ModePs5:
    case ModeSwitch:
        break;
    case ModeHid:
    case ModeXbox360:
    {
        auto secDevice = std::make_shared<XInputSecurityDevice>();
        secDevice->interface_id = profile_mgr.instance_count();
        profile_mgr.add_instance(secDevice);
        profile_mgr.set_usb_instance(secDevice->interface_id, secDevice);
        secDevice->initialize();
        break;
    }
    case ModeGuitarHeroArcade:
    {
        auto venDevice = std::make_shared<GHArcadeVendorDevice>();
        venDevice->interface_id = profile_mgr.instance_count();
        profile_mgr.add_instance(venDevice);
        profile_mgr.set_usb_instance(venDevice->interface_id, venDevice);
        venDevice->initialize();
        break;
    }
    }

    pb_istream_t auxInputStream = pb_istream_from_buffer(dataPtr + mainSize, auxSize);
    proto_AuxConfigBlock block proto_AuxConfigBlock_init_zero;
    block.states.funcs.decode = decode_cycle_input_states;
    block.toggleStates.funcs.decode = decode_toggle_input_states;
    block.bluetoothStates.funcs.decode = decode_bluetooth_states;
    pb_decode(&auxInputStream, proto_AuxConfigBlock_fields, &block);
    auto ret = pb_decode(&inputStream, proto_Config_fields, &config);

    if (!profile_mgr.has_active_instances() || mode == ModeHid || mode == ModeXbox360)
    {
        auto confDevice2 = HIDConfigDevice::instance;
        confDevice2->interface_id = profile_mgr.instance_count();
        profile_mgr.add_instance(confDevice2);
        profile_mgr.set_usb_instance(confDevice2->interface_id, confDevice2);
        confDevice2->initialize();
    }
    device_mgr.remove_disconnected_root_devices();
    if (modeChanged || profile_mgr.has_previous_types())
    {
        reinitialize_device_stack();
    }
    return ret;
}
uint32_t copy_config_info(uint8_t *buffer)
{
    const uint8_t *flashEnd = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START) + EEPROM_SIZE_BYTES;
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(flashEnd - sizeof(ConfigFooter));
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    info.dataCrc = footer.dataCrc;
    info.dataSize = footer.dataSize;
    info.magic = footer.magic;
    info.mainSize = footer.mainSize;
    info.auxSize = footer.auxSize;
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
    config_mgr.schedule_reinit(millis() + 100);
}

void update_aux_cycle(uint32_t id, uint32_t state)
{
    if (!loadedAny)
    {
        return;
    }
    DeviceFactory::set_cycle_state(id, state);
    // Aux region contains data that the firmware can update itself
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    // move data to start of cache to make modifying easier
    memmove(EEPROM.writeCache, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, footer->dataSize);
    pb_ostream_t auxOutputStream = pb_ostream_from_buffer(EEPROM.writeCache + footer->mainSize, EEPROM_SIZE_BYTES - footer->mainSize - sizeof(ConfigFooter));
    proto_AuxConfigBlock block;
    block.states.funcs.encode = encode_cycle_input_states;
    block.toggleStates.funcs.encode = encode_toggle_input_states;
    pb_encode(&auxOutputStream, proto_AuxConfigBlock_fields, &block);
    footer->auxSize = auxOutputStream.bytes_written;
    footer->dataSize = footer->mainSize + footer->auxSize;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, footer->dataSize);
    // Move the encoded data to end where it should be
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, EEPROM.writeCache, footer->dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize);
    EEPROM.commit();
}

void update_aux_toggle(uint32_t id, bool state)
{
    // printf("update aux: %d %d %d\r\n", id, state, config_mgr.get_reinit_time());
    if (config_mgr.get_reinit_time())
    {
        return;
    }
    DeviceFactory::set_toggle_state(id, state);
    // Aux region contains data that the firmware can update itself
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    // move data to start of cache to make modifying easier
    memmove(EEPROM.writeCache, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, footer->dataSize);
    pb_ostream_t auxOutputStream = pb_ostream_from_buffer(EEPROM.writeCache + footer->mainSize, EEPROM_SIZE_BYTES - footer->mainSize - sizeof(ConfigFooter));
    proto_AuxConfigBlock block;
    block.states.funcs.encode = encode_cycle_input_states;
    block.toggleStates.funcs.encode = encode_toggle_input_states;
    pb_encode(&auxOutputStream, proto_AuxConfigBlock_fields, &block);
    footer->auxSize = auxOutputStream.bytes_written;
    footer->dataSize = footer->mainSize + footer->auxSize;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, footer->dataSize);
    // Move the encoded data to end where it should be
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize, EEPROM.writeCache, footer->dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize);
    EEPROM.commit();
}

bool write_config_info(const uint8_t *buffer, uint16_t bufsize)
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
    if (!pb_decode_delimited(&inputStream, proto_ConfigInfo_fields, &info))
    {
        // printf("Didn't decode info?\r\n");
        return false;
    }
    footer->dataCrc = info.dataCrc;
    footer->dataSize = info.dataSize;
    footer->magic = info.magic;
    footer->mainSize = info.mainSize;
    footer->auxSize = info.auxSize;
    return true;
}

bool write_config(const uint8_t *buffer, uint16_t bufsize, uint32_t start)
{
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    if (bufsize + start > footer.dataSize)
    {
        bufsize = footer.dataSize - start;
    }
    memcpy(EEPROM.writeCache + start, buffer, bufsize);
    if (start + bufsize < footer.dataSize)
    {
        // //printf("writing up to: %d < %d\r\n", start + bufsize, footer.dataSize);
        working = true;
        return true;
    }
    uint32_t crc = CRC32::calculate(EEPROM.writeCache, footer.dataSize);
    if (crc != footer.dataCrc)
    {
        // printf("Crc didnt match after writing? %d\r\n", footer.dataCrc);
        return false;
    }
    printf("Everything matched, saving!\r\n");
    //  Move the encoded data in memory down to the footer
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize, EEPROM.writeCache, footer.dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize);
    EEPROM.commit();
    reload();
    working = false;
    return true;
}

uint32_t copy_config(uint8_t *buffer, uint32_t start)
{
    const uint8_t *flashEnd = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START) + EEPROM_SIZE_BYTES;
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(flashEnd - sizeof(ConfigFooter));

    // Check for presence of magic value
    if (footer.magic != FOOTER_MAGIC)
    {
        return 0;
    }

    // Check if dataSize exceeds the reserved space
    if (footer.dataSize + sizeof(ConfigFooter) > EEPROM_SIZE_BYTES)
    {
        return 0;
    }

    const uint8_t *dataPtr = flashEnd - sizeof(ConfigFooter) - footer.dataSize;

    // Verify CRC32 hash
    if (CRC32::calculate(dataPtr, footer.dataSize) != footer.dataCrc)
    {
        return 0;
    }
    const uint32_t remaining = footer.dataSize - start;
    const uint32_t size = remaining > 63 ? 63 : remaining;
    memcpy(buffer, dataPtr + start, size);
    return size;
}
bool load()
{
    const uint8_t *flashEnd = reinterpret_cast<const uint8_t *>(EEPROM.writeCache) + EEPROM_SIZE_BYTES;
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));

    // Check for presence of magic value
    if (footer.magic != FOOTER_MAGIC)
    {
        // printf("footer wrong %x != %x\r\n", footer.magic, FOOTER_MAGIC);
        return false;
    }

    // Check if dataSize exceeds the reserved space
    if (footer.dataSize + sizeof(ConfigFooter) > EEPROM_SIZE_BYTES)
    {
        return false;
    }

    const uint8_t *dataPtr = flashEnd - sizeof(ConfigFooter) - footer.dataSize;

    // Verify CRC32 hash
    if (CRC32::calculate(dataPtr, footer.dataSize) != footer.dataCrc)
    {
        return false;
    }

    return inner_load(footer.currentProfile, dataPtr, footer.dataSize, footer.mainSize, footer.auxSize);
}