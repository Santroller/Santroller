#include "config/instance_factory.hpp"
#include "config/emulation_device_config.hpp"
#include "managers/profile_manager.hpp"
#include "managers/config_manager.hpp"
#include "emulation/bt/bt_gamepad.h"
#include "emulation/ps2_emulation.hpp"
#include "emulation/wii_emulation.hpp"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/ogxbox_device.h"
#include "emulation/usb/xinput_device.h"
#include "emulation/usb/xone_device.h"
#include "emulation/usb/ps3_device.h"
#include "emulation/usb/ps4_device.h"
#include "emulation/usb/ps5_device.h"
#include "emulation/usb/switch_device.h"
#include "emulation/usb/gh_arcade_device.h"

static auto& profile_mgr = ProfileManager::instance();
static auto& config_mgr = ConfigManager::instance();

void InstanceFactory::setup_instance_from_profile(
    std::shared_ptr<Instance> instance,
    std::shared_ptr<Profile> profile)
{
    instance->profiles.push_back(profile);
    instance->subtype = profile->subtype;
    instance->xinput_on_windows = profile->xinput_on_windows;
    instance->invert_y_axis_hid = profile->invert_y_axis_hid;
    instance->supports_ps4 = profile->supports_ps4;
}

std::shared_ptr<Instance> InstanceFactory::create_instance(
    int assignment_mask,
    std::shared_ptr<Profile> profile,
    ConsoleMode usb_mode,
    const EmulationDeviceConfig& emulation_devices)
{
    std::shared_ptr<Instance> instance;
    
    if (assignment_mask & ProfileAssignMask_AssignBluetoothGamepad) {
        instance = std::make_shared<BTGamepadDevice>();
    }
    else if (assignment_mask & ProfileAssignMask_AssignPsx) {
        if (!emulation_devices.has_psx) {
            return nullptr;
        }
        instance = std::make_shared<Ps2EmulationDeviceInstance>(
            emulation_devices.psx
        );
    }
    else if (assignment_mask & ProfileAssignMask_AssignWiimoteExtension) {
        if (!emulation_devices.has_wii) {
            return nullptr;
        }
        instance = std::make_shared<WiiEmulationDeviceInstance>(
            emulation_devices.wii
        );
    }
    else if (assignment_mask & ProfileAssignMask_AssignUsb) {
        instance = std::static_pointer_cast<Instance>(
            create_usb_instance(usb_mode, profile->subtype));
    }
    
    if (!instance) {
        return nullptr;
    }
    
    profile_mgr.add_instance(instance);
    setup_instance_from_profile(instance, profile);
    profile_mgr.register_instance(instance, profile);
    instance->initialize();
    profile_mgr.set_current_type(profile->profile_id, profile->subtype);
    
    return instance;
}

std::shared_ptr<UsbDevice> InstanceFactory::create_usb_instance(
    ConsoleMode mode,
    SubType subtype)
{
    std::shared_ptr<UsbDevice> instance;
    
    if (subtype == SubType_KeyboardMouse) {
        instance = std::make_shared<HIDKeyboardDevice>();
        config_mgr.request_mode(ModeHid);
        return instance;
    }
    
    switch (mode) {
    case ModeHid:
        instance = std::make_shared<HIDGamepadDevice>();
        break;
        
    case ModeOgXbox:
        instance = std::make_shared<OGXboxGamepadDevice>();
        break;
        
    case ModeXbox360:
        instance = std::make_shared<XInputGamepadDevice>();
        break;
        
    case ModeXboxOne:
        instance = std::make_shared<XboxOneGamepadDevice>();
        if (!profile_mgr.get_emulated_device(mode)) {
            profile_mgr.set_emulated_device(mode, instance);
        }
        break;
        
    case ModeWiiRb:
        instance = std::make_shared<PS3GamepadDevice>(true);
        break;
        
    case ModePs3:
        instance = std::make_shared<PS3GamepadDevice>(false);
        break;
        
    case ModePs4:
        instance = std::make_shared<PS4GamepadDevice>();
        break;
        
    case ModePs5:
        instance = std::make_shared<PS5GamepadDevice>();
        break;
        
    case ModeSwitch:
        instance = std::make_shared<SwitchGamepadDevice>();
        break;
        
    case ModeGuitarHeroArcade:
        instance = std::make_shared<GHArcadeGamepadDevice>();
        break;
        
    default:
        return nullptr;
    }
    
    if (instance) {
        instance->interface_id = profile_mgr.instance_count();
        profile_mgr.set_usb_instance(instance->interface_id, instance);
    }
    
    return instance;
}
