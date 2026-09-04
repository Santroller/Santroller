#include "config/config_loader.hpp"

#include "config/device_factory.hpp"
#include "config/emulation_device_config.hpp"
#include "devices/usb.hpp"
#include "emulation/usb/gh_arcade_device.h"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/xinput_device.h"
#include "managers/device_manager.hpp"
#include "managers/profile_manager.hpp"
#include "managers/config_manager.hpp"
#include "main.hpp"

#include <memory>

bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_cycle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_toggle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_bluetooth_states(pb_istream_t *stream, const pb_field_t *field, void **arg);

bool ConfigLoader::apply(const ConfigImage &image, ConsoleMode current_mode)
{
    proto_Config config proto_Config_init_zero;
    DeviceManager &device_mgr = DeviceManager::instance();
    ProfileManager &profile_mgr = ProfileManager::instance();
    ConfigManager &config_mgr = ConfigManager::instance();
    EmulationDeviceConfig emulation_devices;

    config_mgr.begin_config_load();

    DeviceFactory::clear_cycle_states();
    DeviceFactory::clear_toggle_states();
    DeviceFactory::clear_bluetooth_pairing_states();
    pb_istream_t inputStream = pb_istream_from_buffer(image.data, image.main_size);
    device_mgr.clear_assignable_devices();

    config.devices.funcs.decode = &load_device;
    config.devices.arg = &emulation_devices;
    config.profiles.funcs.decode = &load_profile;
    config.profiles.arg = &emulation_devices;
    config.guiConfig.funcs.decode = nullptr;
    config_mgr.clear_seen_masks();
    device_mgr.clear_active_devices();
    device_mgr.mark_root_devices_disconnected();
    profile_mgr.prepare_for_config_reload();
    UsbDevice::reset_ep();

    switch (current_mode)
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

    pb_istream_t auxInputStream = pb_istream_from_buffer(image.data + image.main_size, image.aux_size);
    proto_AuxConfigBlock block proto_AuxConfigBlock_init_zero;
    block.states.funcs.decode = decode_cycle_input_states;
    block.toggleStates.funcs.decode = decode_toggle_input_states;
    block.bluetoothStates.funcs.decode = decode_bluetooth_states;
    pb_decode(&auxInputStream, proto_AuxConfigBlock_fields, &block);
    auto ret = pb_decode(&inputStream, proto_Config_fields, &config);

    const ConsoleMode resolved_mode = config_mgr.get_requested_mode();
    if (!profile_mgr.has_active_instances() || resolved_mode == ModeHid || resolved_mode == ModeXbox360)
    {
        printf("adding HID config device\r\n");
        auto confDevice = HIDConfigDevice::instance;
        confDevice->interface_id = profile_mgr.instance_count();
        profile_mgr.add_instance(confDevice);
        profile_mgr.set_usb_instance(confDevice->interface_id, confDevice);
        confDevice->initialize();
    }
    device_mgr.remove_disconnected_root_devices();
    if (config_mgr.has_mode_changed() || resolved_mode != current_mode || profile_mgr.has_previous_types())
    {
        reinitialize_device_stack();
    }
    return ret;
}
