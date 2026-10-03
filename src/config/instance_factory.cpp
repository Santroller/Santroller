#include "config/instance_factory.hpp"
#include "managers/profile_manager.hpp"
#include "managers/config_manager.hpp"
#include "managers/device_manager.hpp"
#include "devices/ps2_emulation.hpp"
#include "devices/wii_emulation.hpp"
#include "emulation/bt/bt_gamepad.h"
#include "emulation/ps2_emulation.hpp"
#include "emulation/wii_emulation.hpp"
#include "emulation/bt/wii_remote_emulation.hpp"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/ogxbox_device.h"
#include "emulation/usb/xinput_device.h"
#include "emulation/usb/xone_device.h"
#include "emulation/usb/ps3_device.h"
#include "emulation/usb/ps4_device.h"
#include "emulation/usb/ps5_device.h"
#include "emulation/usb/switch_device.h"
#include "emulation/usb/gh_arcade_device.h"
#include "emulation/usb/spice2x_device.h"
#include "emulation/usb/pdloader_device.h"

static auto &profile_mgr = ProfileManager::instance();
static auto &config_mgr = ConfigManager::instance();

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
    ConsoleMode usb_mode)
{
    std::shared_ptr<Instance> instance;

    if (assignment_mask & ProfileAssignMask_AssignBluetoothGamepad)
    {
        if (!config_mgr.has_bluetooth())
        {
            return nullptr;
        }
        instance = std::make_shared<BTGamepadDevice>();
    }
    else if (assignment_mask & ProfileAssignMask_AssignBluetoothWiimote)
    {
        if (!config_mgr.has_bluetooth())
        {
            return nullptr;
        }
        instance = std::make_shared<WiiRemoteEmulationDeviceInstance>();
    }
    else if (assignment_mask & ProfileAssignMask_AssignPsx)
    {
        auto psx_dev = DeviceManager::instance().get_psx_emulation_device();
        if (!psx_dev)
        {
            return nullptr;
        }
        instance = std::make_shared<Ps2EmulationDeviceInstance>(
            psx_dev->get_config());
    }
    else if (assignment_mask & ProfileAssignMask_AssignWiimoteExtension)
    {
        auto wii_dev = DeviceManager::instance().get_wii_emulation_device();
        if (!wii_dev)
        {
            return nullptr;
        }
        instance = std::make_shared<WiiExtensionEmulationDeviceInstance>(
            wii_dev->get_config());
    }
    else if (assignment_mask & ProfileAssignMask_AssignUsb)
    {
        if (usb_mode == ModeXboxOne)
        {
            auto existing_dev = profile_mgr.get_emulated_device(ModeXboxOne);
            if (existing_dev)
            {
                existing_dev->profiles.push_back(profile);
                profile_mgr.register_instance(existing_dev, profile);
                printf("Attaching profile to existing Xbox One instance, total profiles: %zu\n", existing_dev->profiles.size());
                return existing_dev;
            }
            auto preserved = profile_mgr.take_preserved_xone();
            if (preserved)
            {
                profile_mgr.restore_preserved_xone(preserved);
                setup_instance_from_profile(preserved, profile);
                profile_mgr.register_instance(preserved, profile);
                printf("Restored preserved Xbox One instance, total profiles: %zu\n", preserved->profiles.size());
                return preserved;
            }
        }
        if (usb_mode == ModePs5)
        {
            auto existing_dev = profile_mgr.get_emulated_device(ModePs5);
            if (existing_dev)
            {
                printf("Multiple PS5 controllers on one pico is not supported\r\n");
                return nullptr;
            }
        }
        if (usb_mode == ModePs4)
        {
            auto existing_dev = profile_mgr.get_emulated_device(ModePs4);
            if (existing_dev)
            {
                printf("Multiple PS4 controllers on one pico is not supported\r\n");
                return nullptr;
            }
        }
        if (usb_mode == ModePs3)
        {
            auto existing_dev = profile_mgr.get_emulated_device(ModePs3);
            if (existing_dev)
            {
                printf("Multiple PS3 controllers on one pico is not supported\r\n");
                return nullptr;
            }
        }
        instance = std::static_pointer_cast<Instance>(
            create_usb_instance(usb_mode, profile->subtype));
    }

    if (!instance)
    {
        return nullptr;
    }

    profile_mgr.add_instance(instance);
    setup_instance_from_profile(instance, profile);
    profile_mgr.register_instance(instance, profile);
    printf("Creating instance for profile with subtype: %d\n", profile->subtype);
    instance->initialize();

    return instance;
}

std::shared_ptr<UsbDevice> InstanceFactory::create_usb_instance(
    ConsoleMode mode,
    SubType subtype)
{
    std::shared_ptr<UsbDevice> instance;

    printf("Creating USB instance with mode: %d, subtype: %d\n", mode, subtype);
    if (subtype == SubType_KeyboardMouse && mode != ModeHid)
    {   
        config_mgr.request_mode(ModeHid);
    }
    switch (mode)
    {
    case ModeHid:
        if (subtype == SubType_KeyboardMouse)
        {
            instance = std::make_shared<HIDKeyboardDevice>();
        }
        else
        {
            instance = std::make_shared<HIDGamepadDevice>();
        }
        break;

    case ModeOgXbox:
        instance = std::make_shared<OGXboxGamepadDevice>();
        break;

    case ModeXbox360:
        instance = std::make_shared<XInputGamepadDevice>();
        break;

    case ModeXboxOne:
        instance = std::make_shared<XboxOneGamepadDevice>();
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
        if (subtype == SubType_ProjectDiva || subtype == SubType_Taiko)
        {
            instance = std::make_shared<SwitchArcadeDevice>(subtype == SubType_Taiko);
        }
        else
        {
            instance = std::make_shared<SwitchGamepadDevice>();
        }
        break;

    case ModeSpice2x:
        instance = std::make_shared<Spice2xDevice>();
        break;

    case ModePdLoader:
        instance = std::make_shared<PDLoaderDevice>();
        break;

    case ModeGuitarHeroArcade:
        instance = std::make_shared<GHArcadeGamepadDevice>();
        break;

    default:
        return nullptr;
    }
    if (!profile_mgr.get_emulated_device(mode))
    {
        profile_mgr.set_emulated_device(mode, instance);
    }

    if (instance)
    {
        instance->interface_id = profile_mgr.usb_instance_count();
        profile_mgr.set_usb_instance(instance->interface_id, instance);
    }

    return instance;
}
