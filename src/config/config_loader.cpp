#include "config/config_loader.hpp"

#include "config/aux_config.hpp"
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

// Collects the ids of the config's devices without building them
static bool collect_device_id(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto *ids = static_cast<std::vector<uint32_t> *>(*arg);
    pb_wire_type_t wire_type;
    uint32_t tag;
    bool eof;
    while (pb_decode_tag(stream, &wire_type, &tag, &eof))
    {
        if (tag == proto_Device_deviceid_tag && wire_type == PB_WT_VARINT)
        {
            uint64_t id;
            if (!pb_decode_varint(stream, &id))
            {
                return false;
            }
            ids->push_back(static_cast<uint32_t>(id));
        }
        else if (!pb_skip_field(stream, wire_type))
        {
            return false;
        }
    }
    return eof;
}

bool ConfigLoader::apply(const ConfigImage &image, ConsoleMode current_mode)
{
    proto_Config config proto_Config_init_zero;
    DeviceManager &device_mgr = DeviceManager::instance();
    ProfileManager &profile_mgr = ProfileManager::instance();
    ConfigManager &config_mgr = ConfigManager::instance();

    // Pairings and bonds are learned by the device itself, so after the first load the copy in memory is
    // the real one. The tool's copy in an uploaded config can be older, and would forget new pairings
    const bool load_bluetooth = !s_bluetooth_loaded;
    s_bluetooth_loaded = true;

    DeviceFactory::clear_cycle_states();
    DeviceFactory::clear_toggle_states();
    DeviceFactory::clear_arcade_side();
    if (load_bluetooth)
    {
        DeviceFactory::clear_bluetooth_pairing_states();
    }
    pb_istream_t inputStream = pb_istream_from_buffer(image.data, image.main_size);
    device_mgr.clear_assignable_devices();

    config.devices.funcs.decode = &load_device;
    config.profiles.funcs.decode = &load_profile;
    config.guiConfig.funcs.decode = nullptr;
    config_mgr.clear_seen_masks();
    config_mgr.clear_deferred_forced_mode();
    if (config_mgr.is_mode_from_profile() && !HIDConfigDevice::tool_may_be_open())
    {
        // A profile forced the current mode. Go back to the detected host's mode in case none of the active
        // profiles force it now, any that still do put it back. Left alone while the tool is open, as
        // switching modes would drop its connection.
        config_mgr.set_mode_from_profile(false);
        config_mgr.request_mode(config_mgr.get_detected_mode());
    }
    BatteryManager::instance().begin_config_reload();
    device_mgr.clear_active_devices();
    device_mgr.mark_root_devices_disconnected();
    profile_mgr.prepare_for_config_reload();
    UsbDevice::reset_ep();


    pb_istream_t auxInputStream = pb_istream_from_buffer(image.data + image.main_size, image.aux_size);
    proto_AuxConfigBlock block proto_AuxConfigBlock_init_zero;
    block.states.funcs.decode = decode_cycle_input_states;
    block.toggleStates.funcs.decode = decode_toggle_input_states;
    if (load_bluetooth)
    {
        block.bluetoothStates.funcs.decode = decode_bluetooth_states;
        block.tlvEntries.funcs.decode = decode_bluetooth_tlv_entries;
    }
    pb_decode(&auxInputStream, proto_AuxConfigBlock_fields, &block);
    if (block.has_ghArcadeSide && (block.ghArcadeSide == 1 || block.ghArcadeSide == 2))
    {
        DeviceFactory::set_arcade_side(block.ghArcadeSide);
    }
    {
        // Devices the new config doesn't have go before it builds its own. One whose id changed would
        // otherwise run alongside its replacement on the same hardware until the end of the reload,
        // eg still polling a bus from its alarms while the replacement resets it
        // (reusing config for the scan, it's big and core0's stack is tight during a reload)
        std::vector<uint32_t> ids;
        config = proto_Config_init_zero;
        config.devices.funcs.decode = &collect_device_id;
        config.devices.arg = &ids;
        pb_istream_t scanStream = pb_istream_from_buffer(image.data, image.main_size);
        if (pb_decode(&scanStream, proto_Config_fields, &config))
        {
            device_mgr.remove_root_devices_not_in(ids);
        }
        config = proto_Config_init_zero;
        config.devices.funcs.decode = &load_device;
        config.profiles.funcs.decode = &load_profile;
        config.guiConfig.funcs.decode = nullptr;
    }
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
    case ModeKeyboardMania:
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
