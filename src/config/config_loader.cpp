#include "config/config_loader.hpp"

#include "config/device_factory.hpp"
#include "devices/usb.hpp"
#include "emulation/usb/gh_arcade_device.h"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/xinput_device.h"
#include "managers/device_manager.hpp"
#include "managers/profile_manager.hpp"
#include "managers/config_manager.hpp"
#include "managers/inactivity_manager.hpp"
#include "managers/battery_manager.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "secondary_pico.hpp"
#include "main.hpp"

#include <memory>

bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_cycle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_toggle_input_states(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_bluetooth_states(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_bluetooth_tlv_entries(pb_istream_t *stream, const pb_field_t *field, void **arg);

bool ConfigLoader::apply(const ConfigImage &image, ConsoleMode current_mode)
{
    proto_Config config proto_Config_init_zero;
    DeviceManager &device_mgr = DeviceManager::instance();
    ProfileManager &profile_mgr = ProfileManager::instance();
    ConfigManager &config_mgr = ConfigManager::instance();

    DeviceFactory::clear_cycle_states();
    DeviceFactory::clear_toggle_states();
    DeviceFactory::clear_bluetooth_pairing_states();
    pb_istream_t inputStream = pb_istream_from_buffer(image.data, image.main_size);
    device_mgr.clear_assignable_devices();

    config.devices.funcs.decode = &load_device;
    config.profiles.funcs.decode = &load_profile;
    config.guiConfig.funcs.decode = nullptr;
    config_mgr.clear_seen_masks();
    BatteryManager::instance().begin_config_reload();
    device_mgr.clear_active_devices();
    device_mgr.mark_root_devices_disconnected();
    profile_mgr.prepare_for_config_reload();
    UsbDevice::reset_ep();


    pb_istream_t auxInputStream = pb_istream_from_buffer(image.data + image.main_size, image.aux_size);
    proto_AuxConfigBlock block proto_AuxConfigBlock_init_zero;
    block.states.funcs.decode = decode_cycle_input_states;
    block.toggleStates.funcs.decode = decode_toggle_input_states;
    block.bluetoothStates.funcs.decode = decode_bluetooth_states;
    block.tlvEntries.funcs.decode = decode_bluetooth_tlv_entries;
    pb_decode(&auxInputStream, proto_AuxConfigBlock_fields, &block);
    auto ret = pb_decode(&inputStream, proto_Config_fields, &config);
    InactivityManager::instance().configure(config.has_inactivity ? &config.inactivity : nullptr);
    if (config.has_peripheralBoot)
    {
        i2c_inst_t *i2c_block = config.peripheralBoot.i2c.block == 1 ? i2c1 : i2c0;
        secondary_pico_slave_init(i2c_block, config.peripheralBoot.i2c.sda, config.peripheralBoot.i2c.scl, config.peripheralBoot.idPin);
    }
    const ConsoleMode resolved_mode = config_mgr.get_requested_mode();
    if (!profile_mgr.has_active_instances() || resolved_mode == ModeHid || resolved_mode == ModeXbox360)
    {
        auto confDevice = HIDConfigDevice::instance;
        confDevice->interface_id = profile_mgr.usb_instance_count();
        profile_mgr.add_instance(confDevice);
        profile_mgr.set_usb_instance(confDevice->interface_id, confDevice);
        confDevice->initialize();
    }
    switch (resolved_mode)
    {
    case ModeOgXbox:
    case ModeXboxOne:
    case ModeWiiRb:
    case ModePs3:
    case ModePs4:
    case ModePs5:
    case ModeSwitch:
    case ModeSpice2x:
    case ModePdLoader:
        break;
    case ModeHid:
    case ModeXbox360:
    {
        const auto id = profile_mgr.usb_instance_count();
        auto secDevice = profile_mgr.reuse_usb_instance(id, resolved_mode, static_cast<SubType>(0), true);
        if (!secDevice)
        {
            secDevice = std::make_shared<XInputSecurityDevice>();
            secDevice->interface_id = id;
            profile_mgr.add_instance(secDevice);
            profile_mgr.set_usb_instance(id, secDevice);
            profile_mgr.set_usb_reload_identity(id, resolved_mode, static_cast<SubType>(0), true);
            secDevice->initialize();
            profile_mgr.finish_usb_instance_initialization(id);
        }
        break;
    }
    case ModeGuitarHeroArcade:
    {
        const auto id = profile_mgr.usb_instance_count();
        auto venDevice = profile_mgr.reuse_usb_instance(id, resolved_mode, static_cast<SubType>(0), true);
        if (!venDevice)
        {
            venDevice = std::make_shared<GHArcadeVendorDevice>();
            venDevice->interface_id = id;
            profile_mgr.add_instance(venDevice);
            profile_mgr.set_usb_instance(id, venDevice);
            profile_mgr.set_usb_reload_identity(id, resolved_mode, static_cast<SubType>(0), true);
            venDevice->initialize();
            profile_mgr.finish_usb_instance_initialization(id);
        }
        break;
    }
    }
    device_mgr.remove_disconnected_root_devices();
    // the HID descriptor gains or loses its battery report, so the host has to see it again
    if (BatteryManager::instance().present_changed())
    {
        config_mgr.request_device_stack_reinit();
    }
    if (config_mgr.should_reinitialize_device_stack() || resolved_mode != current_mode || profile_mgr.changed_types())
    {
        reinitialize_device_stack();
    }
    profile_mgr.finish_config_reload();
    config_mgr.sync_requested_mode_to_current();
    return ret;
}
