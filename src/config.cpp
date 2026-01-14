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
#include <memory>
std::vector<std::shared_ptr<Instance>> instances;
std::map<uint32_t, std::shared_ptr<Instance>> active_instances;
std::map<uint32_t, std::shared_ptr<Device>> devices;
std::map<uint8_t, std::shared_ptr<UsbDevice>> usb_instances;
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
        devices[*dev_id] = std::shared_ptr<Device>(new AccelerometerDevice(device.device.accelerometer, *dev_id));
        break;
    case proto_Device_crkdNeck_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new CrkdDevice(device.device.crkdNeck, *dev_id));
        break;
    case proto_Device_wii_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new WiiDevice(device.device.wii, *dev_id));
        break;
    case proto_Device_bhDrum_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new BandHeroDrumDevice(device.device.bhDrum, *dev_id));
        break;
    case proto_Device_crazyGuitarNeck_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new CrazyGuitarNeckDevice(device.device.crazyGuitarNeck, *dev_id));
        break;
    case proto_Device_djhTurntable_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new DjHeroTurntableDevice(device.device.djhTurntable, *dev_id));
        break;
    case proto_Device_gh5Neck_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new GH5NeckDevice(device.device.gh5Neck, *dev_id));
        break;
    case proto_Device_max1704x_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new Max1704XDevice(device.device.max1704x, *dev_id));
        break;
    case proto_Device_mpr121_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new MPR121Device(device.device.mpr121, *dev_id));
        break;
    case proto_Device_usbHost_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new USBDevice(device.device.usbHost, *dev_id));
        break;
    case proto_Device_ads1115_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new ADS1115Device(device.device.ads1115, *dev_id));
        break;
    case proto_Device_debug_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new DebugDevice(device.device.debug, *dev_id));
        break;
    case proto_Device_ws2812_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new WS2812Device(device.device.ws2812, *dev_id));
        break;
    case proto_Device_stp16cpc_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new STP16CPCDevice(device.device.stp16cpc, *dev_id));
        break;
    case proto_Device_apa102_tag:
        devices[*dev_id] = std::shared_ptr<Device>(new APA102Device(device.device.apa102, *dev_id));
        break;
    }
    *dev_id += 1;
    return true;
}
std::unique_ptr<Input> make_input(proto_Input input)
{
    switch (input.which_input)
    {
    case proto_Input_wiiAxis_tag:
        return std::unique_ptr<Input>(new WiiAxisInput(input.input.wiiAxis, std::static_pointer_cast<WiiDevice>(devices[input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_wiiButton_tag:
        return std::unique_ptr<Input>(new WiiButtonInput(input.input.wiiButton, std::static_pointer_cast<WiiDevice>(devices[input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_crkd_tag:
        return std::unique_ptr<Input>(new CrkdButtonInput(input.input.crkd, std::static_pointer_cast<CrkdDevice>(devices[input.input.crkd.deviceid])));
        break;
    case proto_Input_gpio_tag:
        return std::unique_ptr<Input>(new GPIOInput(input.input.gpio));
        break;
    case proto_Input_ads1115_tag:
        return std::unique_ptr<Input>(new ADS1115Input(input.input.ads1115, std::static_pointer_cast<ADS1115Device>(devices[input.input.ads1115.deviceid])));
        break;
    case proto_Input_accelerometer_tag:
        return std::unique_ptr<Input>(new AccelerometerInput(input.input.accelerometer, std::static_pointer_cast<AccelerometerDevice>(devices[input.input.accelerometer.deviceid])));
        break;
    case proto_Input_gh5Neck_tag:
        return std::unique_ptr<Input>(new Gh5ButtonInput(input.input.gh5Neck, std::static_pointer_cast<GH5NeckDevice>(devices[input.input.gh5Neck.deviceid])));
        break;
    default:
        return nullptr;
    }
}
bool load_mapping(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    Instance *instance = (Instance *)*arg;
    proto_Mapping mapping;
    pb_decode(stream, proto_Mapping_fields, &mapping);
    std::unique_ptr<Input> input = make_input(mapping.input);
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
bool load_activation_method(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    Instance *instance = (Instance *)*arg;
    proto_ActivationTrigger trigger;
    pb_decode(stream, proto_ActivationTrigger_fields, &trigger);
    switch (trigger.which_trigger)
    {
    case proto_ActivationTrigger_input_tag:
    {

        std::unique_ptr<Input> input = make_input(trigger.trigger.input.input);
        if (input == nullptr)
        {
            return true;
        }
        instance->triggers.emplace_back(new InputActivationTrigger(trigger.trigger.input, std::move(input), instance->profile_id));
        break;
    }
    }
    return true;
}
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    std::shared_ptr<UsbDevice> instance = nullptr;
    // Instance *instance = nullptr;
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
    proto_Profile profile;
    profile.activationMethod.arg = instance.get();
    profile.activationMethod.funcs.decode = &load_activation_method;
    profile.mappings.funcs.decode = &load_mapping;
    profile.mappings.arg = instance.get();
    pb_decode(stream, proto_Profile_fields, &profile);
    instances.push_back(instance);
    instance->profile_id = profile.uid;
    auto active = active_instances.find(instance->profile_id);
    if (active != active_instances.end())
    {
        active_instances[instance->profile_id] = instance;
    }
    if (profile.defaultProfile && !loadedAny) {
        loadedAny = true;
        instance->m_interface = active_instances.size();
        active_instances[instance->profile_id] = instance;
        usb_instances[usb_instances.size()] = instance;
        printf("instance: %d\r\n", instance->m_interface);
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
    // we just have to encode the entire device map by hand here, we write them all at once.
    return true;
}
bool save_profile(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    // We just have to encode all the profiles by hand here, we write them all at once.
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
    // We are now sufficiently confident that the data is valid so we run the deserialization
    // load just the current profile to begin with
    pb_istream_t inputStream = pb_istream_from_buffer(dataPtr, size);
    config.devices.funcs.decode = nullptr;
    config.profiles.funcs.decode = nullptr;
    pb_decode(&inputStream, proto_Config_fields, &config);
    // now actually load the entire config
    inputStream = pb_istream_from_buffer(dataPtr, size);
    config.devices.funcs.decode = &load_device;
    uint16_t dev_id = 0;
    config.devices.arg = &dev_id;
    config.profiles.funcs.decode = &load_profile;
    devices.clear();
    instances.clear();
    active_instances.clear();
    usb_instances.clear();
    switch (mode)
    {
    case ModeHid:
    {
        auto confDevice = std::make_shared<HIDConfigDevice>();
        confDevice->m_interface = instances.size();
        instances.push_back(confDevice);
        active_instances[confDevice->m_interface] = confDevice;
        usb_instances[confDevice->m_interface] = confDevice;
        break;
    }
    case ModeOgXbox:
    case ModeXboxOne:
    case ModeWiiRb:
    case ModePs3:
    case ModePs4:
    case ModePs5:
    case ModeSwitch:
        break;
    case ModeXbox360:
    {
        auto secDevice = std::make_shared<XInputSecurityDevice>();
        secDevice->m_interface = instances.size();
        instances.push_back(secDevice);
        active_instances[secDevice->m_interface] = secDevice;
        usb_instances[secDevice->m_interface] = secDevice;
        auto confDevice2 = std::make_shared<HIDConfigDevice>();
        confDevice2->m_interface = instances.size();
        instances.push_back(confDevice2);
        active_instances[confDevice2->m_interface] = confDevice2;
        usb_instances[confDevice2->m_interface] = confDevice2;
        break;
    }
    case ModeGuitarHeroArcade:
    {
        auto venDevice = std::make_shared<GHArcadeVendorDevice>();
        venDevice->m_interface = instances.size();
        instances.push_back(std::move(venDevice));
        active_instances[venDevice->m_interface] = std::move(venDevice);
        usb_instances[venDevice->m_interface] = std::move(venDevice);
        break;
    }
    }
    auto ret = pb_decode(&inputStream, proto_Config_fields, &config);
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
    info.currentProfile = footer.currentProfile;
    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 64);
    if (!pb_encode(&outputStream, proto_ConfigInfo_fields, &info))
    {
        return 0;
    }
    return outputStream.bytes_written;
}

void set_current_profile(uint32_t profile)
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    if (footer->currentProfile != profile)
    {
        footer->currentProfile = profile;
        EEPROM.commit_now();
        proto_Config config;
        load(config);
        reinit = true;
        printf("profile set!\r\n");
    }
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
    footer->currentProfile = info.currentProfile;
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