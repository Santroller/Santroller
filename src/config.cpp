#include "config.hpp"
#include "input/input.hpp"
#include "input/gpio.hpp"
#include "input/wii.hpp"
#include "input/crkd.hpp"
#include "input/ads1115.hpp"
#include "input/accelerometer.hpp"
#include "input/gh5.hpp"
#include "devices/base.hpp"
#include "devices/accelerometer.hpp"
#include "devices/wii.hpp"
#include "devices/bhdrum.hpp"
#include "devices/crazyneck.hpp"
#include "devices/debug.hpp"
#include "devices/djh.hpp"
#include "devices/crkd.hpp"
#include "devices/ads1115.hpp"
#include "devices/gh5neck.hpp"
#include "devices/usb.hpp"
#include "devices/ps2.hpp"
#include "devices/max1704x.hpp"
#include "devices/mpr121.hpp"
#include "devices/ws2812.hpp"
#include "devices/apa102.hpp"
#include "devices/stp16cpc.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/device/xinput_device.h"
#include "usb/device/ogxbox_device.h"
#include "usb/device/xone_device.h"
#include "usb/device/hid_device.h"
#include "usb/device/ps3_device.h"
#include "usb/device/ps4_device.h"
#include "usb/device/ps5_device.h"
#include "usb/device/gh_arcade_device.h"
#include "usb/device/switch_device.h"
#include "usb/usb_descriptors.h"
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
std::vector<std::shared_ptr<Instance>> instances;
std::vector<std::shared_ptr<Instance>> active_instances;
std::unordered_map<uint32_t, std::shared_ptr<Profile>> all_profiles;
std::set<uint32_t> active_profiles;
// devices that are connected
std::vector<std::shared_ptr<Device>> valid_devices;
std::vector<std::shared_ptr<Device>> active_devices;
// devices that have not yet been assigned to a profile
std::vector<std::shared_ptr<Device>> assignable_devices;
std::unordered_map<uint8_t, std::shared_ptr<UsbDevice>> usb_instances;
std::unordered_map<uint8_t, std::shared_ptr<UsbDevice>> usb_instances_by_epnum;
proto_SubType current_type;
ConsoleMode mode = ModeHid;
ConsoleMode newMode = mode;
bool working = false;
bool loadedAny = false;

bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_Device device;
    uint16_t *dev_id = (uint16_t *)*arg;
    pb_decode(stream, proto_Device_fields, &device);
    switch (device.which_device)
    {
    case proto_Device_accelerometer_tag:
        active_devices.emplace_back(new AccelerometerDevice(device.device.accelerometer, *dev_id));
        break;
    case proto_Device_crkdNeck_tag:
        active_devices.emplace_back(new CrkdDevice(device.device.crkdNeck, *dev_id));
        break;
    case proto_Device_wii_tag:
        active_devices.emplace_back(new WiiDevice(device.device.wii, *dev_id));
        break;
    case proto_Device_psx_tag:
    {
        auto &ps2Dev = active_devices.emplace_back(new PS2Device(device.device.psx, *dev_id, MultitapPort::BASE));
        ps2Dev->rescan(true);
        break;
    }
    case proto_Device_bhDrum_tag:
        active_devices.emplace_back(new BandHeroDrumDevice(device.device.bhDrum, *dev_id));
        break;
    case proto_Device_crazyGuitarNeck_tag:
        active_devices.emplace_back(new CrazyGuitarNeckDevice(device.device.crazyGuitarNeck, *dev_id));
        break;
    case proto_Device_djhTurntable_tag:
        active_devices.emplace_back(new DjHeroTurntableDevice(device.device.djhTurntable, *dev_id));
        break;
    case proto_Device_gh5Neck_tag:
        active_devices.emplace_back(new GH5NeckDevice(device.device.gh5Neck, *dev_id));
        break;
    case proto_Device_max1704x_tag:
        active_devices.emplace_back(new Max1704XDevice(device.device.max1704x, *dev_id));
        break;
    case proto_Device_mpr121_tag:
        active_devices.emplace_back(new MPR121Device(device.device.mpr121, *dev_id));
        break;
    case proto_Device_usbHost_tag:
        active_devices.emplace_back(new USBDevice(device.device.usbHost, *dev_id));
        break;
    case proto_Device_ads1115_tag:
        active_devices.emplace_back(new ADS1115Device(device.device.ads1115, *dev_id));
        break;
    case proto_Device_debug_tag:
        active_devices.emplace_back(new DebugDevice(device.device.debug, *dev_id));
        break;
    case proto_Device_ws2812_tag:
        active_devices.emplace_back(new WS2812Device(device.device.ws2812, *dev_id));
        break;
    case proto_Device_stp16cpc_tag:
        active_devices.emplace_back(new STP16CPCDevice(device.device.stp16cpc, *dev_id));
        break;
    case proto_Device_apa102_tag:
        active_devices.emplace_back(new APA102Device(device.device.apa102, *dev_id));
        break;
    }
    *dev_id += 1;
    return true;
}
std::unique_ptr<Input> make_input(proto_Input input, Profile *profile)
{
    switch (input.which_input)
    {
    case proto_Input_wiiAxis_tag:
        return std::unique_ptr<Input>(new WiiAxisInput(input.input.wiiAxis, std::static_pointer_cast<WiiDevice>(profile->devices[input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_wiiButton_tag:
        return std::unique_ptr<Input>(new WiiButtonInput(input.input.wiiButton, std::static_pointer_cast<WiiDevice>(profile->devices[input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_crkd_tag:
        return std::unique_ptr<Input>(new CrkdButtonInput(input.input.crkd, std::static_pointer_cast<CrkdDevice>(profile->devices[input.input.crkd.deviceid])));
        break;
    case proto_Input_gpio_tag:
        return std::unique_ptr<Input>(new GPIOInput(input.input.gpio));
        break;
    case proto_Input_ads1115_tag:
        return std::unique_ptr<Input>(new ADS1115Input(input.input.ads1115, std::static_pointer_cast<ADS1115Device>(profile->devices[input.input.ads1115.deviceid])));
        break;
    case proto_Input_accelerometer_tag:
        return std::unique_ptr<Input>(new AccelerometerInput(input.input.accelerometer, std::static_pointer_cast<AccelerometerDevice>(profile->devices[input.input.accelerometer.deviceid])));
        break;
    case proto_Input_gh5Neck_tag:
        return std::unique_ptr<Input>(new Gh5ButtonInput(input.input.gh5Neck, std::static_pointer_cast<GH5NeckDevice>(profile->devices[input.input.gh5Neck.deviceid])));
        break;
    default:
        return nullptr;
    }
}
bool load_mapping(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto instance = *(Profile **)arg;
    if (instance->devices.empty())
    {
        for (auto &device : active_devices)
        {
            instance->devices[device->m_id] = device;
        }
    }
    proto_Mapping mapping;
    pb_decode(stream, proto_Mapping_fields, &mapping);
    std::unique_ptr<Input> input = make_input(mapping.input, instance);
    if (input == nullptr)
    {
        return true;
    }
    size_t mapping_id = instance->mappings.size();
    switch (mapping.which_mapping)
    {
    case proto_Mapping_gamepadAxis_tag:
        printf("axis %d\r\n", mapping.mapping.gamepadAxis);
        instance->mappings.emplace_back(new GamepadAxisMapping(mapping, std::move(input), mapping_id, instance->profile_id));
        break;
    case proto_Mapping_ghAxis_tag:
        printf("axis %d\r\n", mapping.mapping.ghAxis);
        instance->mappings.emplace_back(new GuitarHeroGuitarAxisMapping(mapping, std::move(input), mapping_id, instance->profile_id));
        break;
    case proto_Mapping_ghButton_tag:
        printf("button %d\r\n", mapping.mapping.ghButton);
        instance->mappings.emplace_back(new GuitarHeroGuitarButtonMapping(mapping, std::move(input), mapping_id, instance->profile_id));
        break;
    case proto_Mapping_gamepadButton_tag:
        printf("button %d\r\n", mapping.mapping.gamepadButton);
        instance->mappings.emplace_back(new GamepadButtonMapping(mapping, std::move(input), mapping_id, instance->profile_id));
        break;
    }
    return true;
}

bool load_assignment_info(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto instance = (Profile *)*arg;
    auto &list = instance->triggers.back();
    proto_ProfileAssignmentInfo assignment;
    pb_decode(stream, proto_ProfileAssignmentInfo_fields, &assignment);
    switch (assignment.which_assignment)
    {
    case proto_ProfileAssignmentInfo_input_tag:
    {

        std::unique_ptr<Input> input = make_input(assignment.assignment.input.input, instance);
        if (input == nullptr)
        {
            return true;
        }
        list->triggers.emplace_back(new InputActivationTrigger(false, assignment.assignment.input, std::move(input), instance->profile_id));
        break;
    }
    case proto_ProfileAssignmentInfo_inputAnyTime_tag:
    {

        std::unique_ptr<Input> input = make_input(assignment.assignment.inputAnyTime.input, instance);
        if (input == nullptr)
        {
            return true;
        }
        list->triggers.emplace_back(new InputActivationTrigger(true, assignment.assignment.inputAnyTime, std::move(input), instance->profile_id));
        break;
    }
    case proto_ProfileAssignmentInfo_consoleType_tag:
        list->triggers.emplace_back(new ConsoleTypeActivationTrigger(assignment.assignment.consoleType, instance->profile_id));
        break;
    case proto_ProfileAssignmentInfo_wiiExt_tag:
        list->triggers.emplace_back(new WiiExtTypeActivationTrigger(assignment.assignment.wiiExt, instance->profile_id));
        break;
    case proto_ProfileAssignmentInfo_ps2Cnt_tag:
        list->triggers.emplace_back(new PS2ControllerTypeActivationTrigger(assignment.assignment.ps2Cnt, instance->profile_id));
        break;
    case proto_ProfileAssignmentInfo_usbType_tag:
        list->triggers.emplace_back(new UsbTypeActivationTrigger(assignment.assignment.usbType, instance->profile_id));
        break;
    case proto_ProfileAssignmentInfo_usbDevice_tag:
        list->triggers.emplace_back(new SpecificUsbDeviceActivationTrigger(assignment.assignment.usbDevice, instance->profile_id));
        break;
    case proto_ProfileAssignmentInfo_catchall_tag:
        list->triggers.emplace_back(new CatchAllActivationTrigger(instance->profile_id));
        break;
    case proto_ProfileAssignmentInfo_midiChannel_tag:
        list->triggers.emplace_back(new MidiChannelActivationTrigger(assignment.assignment.consoleType, instance->profile_id));
        break;
    case proto_ProfileAssignmentInfo_copilotProfile_tag:
        // TODO: how do we handle this
        break;
    }
    return true;
}

bool load_assignments(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = (Profile *)*arg;
    auto list = new ActivationTriggerList();
    profile->triggers.emplace_back(list);
    proto_ProfileAssignment proto_assignment;
    proto_assignment.assignments.funcs.decode = &load_assignment_info;
    proto_assignment.assignments.arg = profile;
    pb_decode(stream, proto_ProfileAssignment_fields, &proto_assignment);
    return true;
}
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    auto profile = std::make_shared<Profile>();
    proto_Profile proto_profile;
    memset(&proto_profile, 0, sizeof(proto_profile));
    proto_profile.assignments.funcs.decode = &load_assignments;
    proto_profile.mappings.funcs.decode = &load_mapping;
    proto_profile.assignments.arg = profile.get();
    proto_profile.mappings.arg = profile.get();
    pb_decode(stream, proto_Profile_fields, &proto_profile);
    profile->profile_id = proto_profile.uid;
    profile->subtype = proto_profile.deviceToEmulate;
    // TODO: handle this once we support emulating non usb devices
    profile->output = OutputUSB;
    all_profiles[profile->profile_id] = profile;
    std::shared_ptr<UsbDevice> instance = nullptr;
    // on a partial load, we just update the profile for the existing instance
    // if (!HIDConfigDevice::tool_closed())
    // {
    //     for (auto &instance : active_instances)
    //     {
    //         auto it = instance->profiles.begin();
    //         while (it != instance->profiles.end())
    //         {
    //             auto match_profile = *it;
    //             if (match_profile->profile_id == profile->profile_id)
    //             {
    //                 *it = profile;
    //                 break;
    //             }
    //             it++;
    //         }
    //     }
    //     return true;
    // }
    for (auto &list : profile->triggers)
    {
        if (list->validate(true))
        {
            printf("profile assigned! %d\r\n", profile->profile_id);
            if (instance == nullptr)
            {
                if (profile->output == OutputUSB)
                {
                    switch (mode)
                    {
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
                        break;
                    case ModeWiiRb:
                        // wii rb is the same as ps3 but different ids
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
                    }
                    instances.push_back(instance);
                    instance->profiles.push_back(profile);
                    instance->interface_id = active_instances.size();
                    active_instances.push_back(instance);
                    usb_instances[usb_instances.size()] = instance;
                    instance->initialize();
                }
                active_profiles.insert(profile->profile_id);
                printf("instance: %d\r\n", instance->interface_id);
            }
            break;
        }
    }
    return true;
}
struct ConfigFooter
{
    uint32_t dataSize;
    uint32_t dataCrc;
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
bool save_device(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    return true;
}
bool save_profile(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    return true;
}
bool save_assignments(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    return true;
}
bool save(proto_Config *config)
{
    // need to do the opposite of load, aka setting all the encode functions and then encode
    config->devices.funcs.encode = save_device;
    config->profiles.funcs.encode = save_profile;
    pb_ostream_t outputStream = pb_ostream_from_buffer(EEPROM.writeCache, EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    if (!pb_encode(&outputStream, proto_Config_fields, config))
    {
        return false;
    }

    // Create the new footer
    ConfigFooter newFooter;
    newFooter.dataSize = outputStream.bytes_written;
    newFooter.dataCrc = CRC32::calculate(EEPROM.writeCache, newFooter.dataSize);
    newFooter.magic = FOOTER_MAGIC;
    newFooter.currentProfile = 0;

    // The data has changed when the footer content has changed. Only then do we actually need to save.
    const ConfigFooter &oldFooter = *reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    if (newFooter == oldFooter)
    {
        // The data has not changed, no saving neccessary.
        return true;
    }

    // Write the footer
    ConfigFooter *cacheFooter = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    memcpy(cacheFooter, &newFooter, sizeof(ConfigFooter));

    // Move the encoded data in memory down to the footer
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - newFooter.dataSize, EEPROM.writeCache, newFooter.dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - newFooter.dataSize);

    EEPROM.commit();
    return true;
}

bool inner_load(proto_Config &config, const uint32_t currentProfile, const uint8_t *dataPtr, uint32_t size)
{
    printf("inner_load\r\n");
    // We are now sufficiently confident that the data is valid so we run the deserialization
    // load just the current profile to begin with
    pb_istream_t inputStream = pb_istream_from_buffer(dataPtr, size);
    uint16_t dev_id = 0;
    config.devices.arg = &dev_id;
    config.devices.funcs.decode = &load_device;
    config.profiles.funcs.decode = &load_profile;
    active_devices.clear();
    valid_devices.clear();
    assignable_devices.clear();
    instances.clear();
    active_instances.clear();
    usb_instances.clear();
    all_profiles.clear();
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
        secDevice->interface_id = instances.size();
        instances.push_back(secDevice);
        active_instances.push_back(secDevice);
        usb_instances[secDevice->interface_id] = secDevice;
        secDevice->initialize();
        auto confDevice2 = HIDConfigDevice::instance;
        confDevice2->interface_id = instances.size();
        instances.push_back(confDevice2);
        active_instances.push_back(confDevice2);
        usb_instances[confDevice2->interface_id] = confDevice2;
        confDevice2->initialize();
        break;
    }
    case ModeGuitarHeroArcade:
    {
        auto venDevice = std::make_shared<GHArcadeVendorDevice>();
        venDevice->interface_id = instances.size();
        instances.push_back(std::move(venDevice));
        active_instances.push_back(venDevice);
        usb_instances[venDevice->interface_id] = venDevice;
        venDevice->initialize();
        break;
    }
    }
    auto ret = pb_decode(&inputStream, proto_Config_fields, &config);
    if (active_instances.empty())
    {
        auto confDevice2 = HIDConfigDevice::instance;
        confDevice2->interface_id = instances.size();
        instances.push_back(confDevice2);
        active_instances.push_back(confDevice2);
        usb_instances[confDevice2->interface_id] = confDevice2;
        confDevice2->initialize();
    }
    update(true);
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
    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 64);
    if (!pb_encode(&outputStream, proto_ConfigInfo_fields, &info))
    {
        return 0;
    }
    return outputStream.bytes_written;
}

void reload()
{
    reinit = true;
}

bool write_config_info(const uint8_t *buffer, uint16_t bufsize)
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
    if (!pb_decode(&inputStream, proto_ConfigInfo_fields, &info))
    {
        printf("Didn't decode info?\r\n");
        return false;
    }
    footer->dataCrc = info.dataCrc;
    footer->dataSize = info.dataSize;
    footer->magic = info.magic;
    return true;
}

bool write_config(const uint8_t *buffer, uint16_t bufsize, uint32_t start)
{
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    memcpy(EEPROM.writeCache + start, buffer, bufsize);
    if (start + bufsize < footer.dataSize)
    {
        printf("writing up to: %d < %d\r\n", start + bufsize, footer.dataSize);
        working = true;
        return true;
    }
    uint32_t crc = CRC32::calculate(EEPROM.writeCache, footer.dataSize);
    if (crc != footer.dataCrc)
    {
        printf("Crc didnt match after writing?\r\n");
        return false;
    }
    printf("Everything matched, saving!\r\n");
    // Move the encoded data in memory down to the footer
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize, EEPROM.writeCache, footer.dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize);
    proto_Config config;
    EEPROM.commit();
    inner_load(config, footer.currentProfile, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize, footer.dataSize);
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
bool load(proto_Config &config)
{
    config = proto_Config proto_Config_init_zero;

    const uint8_t *flashEnd = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START) + EEPROM_SIZE_BYTES;
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(flashEnd - sizeof(ConfigFooter));

    // Check for presence of magic value
    if (footer.magic != FOOTER_MAGIC)
    {
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

    return inner_load(config, footer.currentProfile, dataPtr, footer.dataSize);
}

void first_load()
{
    active_devices.clear();
    instances.clear();
    active_instances.clear();
    usb_instances.clear();
    all_profiles.clear();
    auto confDevice = std::make_shared<HIDConfigDevice>();
    confDevice->interface_id = instances.size();
    instances.push_back(confDevice);
    active_instances.push_back(confDevice);
    usb_instances[confDevice->interface_id] = confDevice;
}