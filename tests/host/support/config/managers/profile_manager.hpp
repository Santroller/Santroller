#pragma once
// Fake managers/profile_manager.hpp: records what ConfigLoader asks of it
#include <memory>
#include "emulation/usb/device.hpp"
#include "config_fakes.hpp"

class ProfileManager
{
public:
    static ProfileManager &instance()
    {
        static ProfileManager manager;
        return manager;
    }
    void prepare_for_config_reload() { fake_loader::state.calls.push_back("prepare_for_config_reload"); }
    void finish_config_reload() { fake_loader::state.calls.push_back("finish_config_reload"); }
    bool has_active_instances() const { return fake_loader::state.has_active_instances; }
    uint8_t usb_instance_count() const { return uint8_t(fake_loader::state.usb_instances_added); }
    void add_instance(std::shared_ptr<Instance>) { fake_loader::state.usb_instances_added++; }
    void set_usb_instance(uint8_t, std::shared_ptr<UsbDevice>) {}
    std::shared_ptr<UsbDevice> reuse_usb_instance(uint8_t, ConsoleMode, SubType, bool) { return nullptr; }
    void set_usb_reload_identity(uint8_t, ConsoleMode, SubType, bool) {}
    void finish_usb_instance_initialization(uint8_t) {}
    bool changed_types() const { return fake_loader::state.changed_types; }
};
