#include "config.hpp"
#include "input/input.hpp"
#include "input/gpio.hpp"
#include "input/wii.hpp"
#include "input/crkd.hpp"
#include "input/ads1115.hpp"
#include "input/accelerometer.hpp"
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
#include "usb/usb_descriptors.h"
#include "hardware/watchdog.h"
#include "main.hpp"
#include "host/usbh.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"
#include "quadrature_encoder.h"
#include <vector>
#include <memory>
static const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
std::vector<std::unique_ptr<Mapping>> mappings;
std::map<uint32_t, std::shared_ptr<Device>> devices;
std::vector<std::unique_ptr<ActivationTrigger>> triggers;
proto_SubType current_type;
ConsoleMode mode = ConsoleMode::Hid;
ConsoleMode newMode = mode;
typedef struct
{
    uint32_t current;
    uint32_t target;
} profile_args_t;

uint8_t gh5_mapping[] = {
    0x80, 0x15, 0x4D, 0x30, 0x9A, 0x99, 0x66,
    0x65, 0xC9, 0xC7, 0xC8, 0xC6, 0xAF, 0xAD,
    0xAE, 0xAC, 0xFF, 0xFB, 0xFD, 0xF9, 0xFE,
    0xFA, 0xFC, 0xF8, 0xE6, 0xE2, 0xE4, 0xE0,
    0xE5, 0xE1, 0xE3, 0xDF};
void update(bool full_poll)
{
    uint8_t out[64] = {0};
    for (auto &device : devices)
    {
        device.second->update(full_poll);
    }
    // // If we are configuring, disable triggers
    for (auto &trigger : triggers)
    {
        trigger->update(tool_closed());
    }
    switch (mode)
    {
    case ConsoleMode::Hid:
        if (tud_hid_ready())
        {
            for (auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_hid(out);
            }

            // convert bitmask dpad to actual hid dpad
            PCGamepad_Data_t *report = (PCGamepad_Data_t *)out;
            report->dpad = dpad_bindings[report->dpad];
            if (current_type == GuitarHeroGuitar)
            {
                // convert bitmask slider to actual hid slider
                PCGuitarHeroGuitar_Data_t *reportGh = (PCGuitarHeroGuitar_Data_t *)out;
                reportGh->slider = gh5_mapping[reportGh->slider];
            }

            tud_hid_report(ReportId::ReportIdGamepad, out, sizeof(PCGamepad_Data_t));
        }
        break;
    case ConsoleMode::Switch:
    case ConsoleMode::WiiRb:
    case ConsoleMode::Ps3:
    case ConsoleMode::Ps2EmulatorOnPs3:
        if (tud_hid_ready())
        {
            if (current_type == Gamepad && mode == ConsoleMode::Ps3)
            {
                PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)out;
                report->accelX = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->accelY = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->accelZ = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->gyro = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->leftStickX = PS3_STICK_CENTER;
                report->leftStickY = PS3_STICK_CENTER;
                report->rightStickX = PS3_STICK_CENTER;
                report->rightStickY = PS3_STICK_CENTER;
            }
            else
            {
                PS3Dpad_Data_t *gamepad = (PS3Dpad_Data_t *)out;
                gamepad->accelX = PS3_ACCEL_CENTER;
                gamepad->accelY = PS3_ACCEL_CENTER;
                gamepad->accelZ = PS3_ACCEL_CENTER;
                gamepad->gyro = PS3_ACCEL_CENTER;
                gamepad->leftStickX = PS3_STICK_CENTER;
                gamepad->leftStickY = PS3_STICK_CENTER;
                if (current_type == ProKeys)
                {
                    gamepad->rightStickX = 0;
                    gamepad->rightStickY = 0;
                }
                else
                {
                    gamepad->rightStickX = PS3_STICK_CENTER;
                    gamepad->rightStickY = PS3_STICK_CENTER;
                }
            }
            for (auto &mapping : mappings)
            {
                mapping->update(full_poll);
                if (mode == ConsoleMode::Ps2EmulatorOnPs3)
                {
                    // TODO: ps2 on ps3!
                }
                else
                {
                    mapping->update_ps3(out);
                }
            }
            if (current_type == Gamepad && mode == ConsoleMode::Ps3)
            {
                // Gamepads use a totally different report format
                tud_hid_report(ReportId::ReportIdGamepad, &out, sizeof(PS3Gamepad_Data_t));
                break;
            }
            // convert bitmask dpad to actual hid dpad
            PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)out;
            report->dpad = dpad_bindings[report->dpad];
            if (current_type == GuitarHeroGuitar)
            {
                // convert bitmask slider to actual hid slider
                PCGuitarHeroGuitar_Data_t *reportGh = (PCGuitarHeroGuitar_Data_t *)out;
                reportGh->slider = gh5_mapping[reportGh->slider];
            }

            tud_hid_report(0, out, sizeof(PS3Dpad_Data_t));
        }
        break;
    case ConsoleMode::Ps4:
        if (tud_hid_ready())
        {
            for (auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_ps4(out);
            }
            PS4Dpad_Data_t *gamepad = (PS4Dpad_Data_t *)out;
            gamepad->leftStickX = PS3_STICK_CENTER;
            gamepad->leftStickY = PS3_STICK_CENTER;
            gamepad->rightStickX = PS3_STICK_CENTER;
            gamepad->rightStickY = PS3_STICK_CENTER;
            // convert bitmask dpad to actual hid dpad
            gamepad->dpad = dpad_bindings[gamepad->dpad];

            tud_hid_report(ReportId::ReportIdGamepad, out, sizeof(PS4Dpad_Data_t));
        }
        break;
    case ConsoleMode::OgXbox:
        if (tud_ogxbox_n_ready(0))
        {
            OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)out;
            report->rid = 0;
            report->rsize = sizeof(OGXboxGamepad_Data_t);
            for (auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_ogxbox(out);
            }
            if (current_type == GuitarHeroGuitar)
            {
                // convert bitmask slider to actual hid slider
                OGXboxGuitarHeroGuitar_Data_t *reportGh = (OGXboxGuitarHeroGuitar_Data_t *)out;
                reportGh->slider = -((int8_t)((gh5_mapping[reportGh->slider]) ^ 0x80) * -257);
            }

            tud_ogxbox_n_report(0, out, sizeof(XInputGamepad_Data_t));
        }
        break;
    case ConsoleMode::Xbox360:
        if (tud_xinput_n_ready(0))
        {
            XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)out;
            report->rid = 0;
            report->rsize = sizeof(XInputGamepad_Data_t);
            for (auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_xinput(out);
            }
            if (current_type == GuitarHeroGuitar)
            {
                // convert bitmask slider to actual hid slider
                XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)out;
                reportGh->slider = -((int8_t)((gh5_mapping[reportGh->slider]) ^ 0x80) * -257);
            }

            tud_xinput_n_report(0, out, sizeof(XInputGamepad_Data_t));
        }
        break;
    case ConsoleMode::XboxOne:
        break;
    }
}

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
bool load_mapping(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint16_t *mapping_id = (uint16_t *)*arg;
    proto_Mapping mapping;
    pb_decode(stream, proto_Mapping_fields, &mapping);
    std::unique_ptr<Input> input = nullptr;
    switch (mapping.input.which_input)
    {
    case proto_Input_wiiAxis_tag:
        input = std::unique_ptr<Input>(new WiiAxisInput(mapping.input.input.wiiAxis, std::static_pointer_cast<WiiDevice>(devices[mapping.input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_wiiButton_tag:
        input = std::unique_ptr<Input>(new WiiButtonInput(mapping.input.input.wiiButton, std::static_pointer_cast<WiiDevice>(devices[mapping.input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_crkd_tag:
        input = std::unique_ptr<Input>(new CrkdButtonInput(mapping.input.input.crkd, std::static_pointer_cast<CrkdDevice>(devices[mapping.input.input.crkd.deviceid])));
        break;
    case proto_Input_gpio_tag:
        input = std::unique_ptr<Input>(new GPIOInput(mapping.input.input.gpio));
        break;
    case proto_Input_ads1115_tag:
        input = std::unique_ptr<Input>(new ADS1115Input(mapping.input.input.ads1115, std::static_pointer_cast<ADS1115Device>(devices[mapping.input.input.ads1115.deviceid])));
        break;
    case proto_Input_accelerometer_tag:
        input = std::unique_ptr<Input>(new AccelerometerInput(mapping.input.input.accelerometer, std::static_pointer_cast<AccelerometerDevice>(devices[mapping.input.input.accelerometer.deviceid])));
        break;
    }
    if (input == nullptr)
    {
        return true;
    }
    switch (mapping.which_mapping)
    {
    case proto_Mapping_gamepadAxis_tag:
        printf("axis %d\r\n", mapping.mapping.gamepadAxis);
        mappings.push_back(std::unique_ptr<Mapping>(new GamepadAxisMapping(mapping, std::move(input), *mapping_id)));
        break;
    case proto_Mapping_ghAxis_tag:
        printf("axis %d\r\n", mapping.mapping.ghAxis);
        mappings.push_back(std::unique_ptr<Mapping>(new GuitarHeroGuitarAxisMapping(mapping, std::move(input), *mapping_id)));
        break;
    case proto_Mapping_ghButton_tag:
        printf("button %d\r\n", mapping.mapping.ghButton);
        mappings.push_back(std::unique_ptr<Mapping>(new GuitarHeroGuitarButtonMapping(mapping, std::move(input), *mapping_id)));
        break;
    case proto_Mapping_gamepadButton_tag:
        printf("button %d\r\n", mapping.mapping.gamepadButton);
        mappings.push_back(std::unique_ptr<Mapping>(new GamepadButtonMapping(mapping, std::move(input), *mapping_id)));
        break;
    }
    *mapping_id += 1;
    return true;
}
bool load_activation_method(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    uint16_t *profile_id = (uint16_t *)*arg;
    proto_ActivationTrigger trigger;
    pb_decode(stream, proto_ActivationTrigger_fields, &trigger);
    std::unique_ptr<Input> input = nullptr;
    switch (trigger.input.which_input)
    {
    case proto_Input_wiiAxis_tag:
        input = std::unique_ptr<Input>(new WiiAxisInput(trigger.input.input.wiiAxis, std::static_pointer_cast<WiiDevice>(devices[trigger.input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_wiiButton_tag:
        input = std::unique_ptr<Input>(new WiiButtonInput(trigger.input.input.wiiButton, std::static_pointer_cast<WiiDevice>(devices[trigger.input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_wiiExtType_tag:
        input = std::unique_ptr<Input>(new WiiExtensionTypeInput(trigger.input.input.wiiExtType, std::static_pointer_cast<WiiDevice>(devices[trigger.input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_crkd_tag:
        input = std::unique_ptr<Input>(new CrkdButtonInput(trigger.input.input.crkd, std::static_pointer_cast<CrkdDevice>(devices[trigger.input.input.wiiAxis.deviceid])));
        break;
    case proto_Input_gpio_tag:
        input = std::unique_ptr<Input>(new GPIOInput(trigger.input.input.gpio));
        break;
    case proto_Input_ads1115_tag:
        input = std::unique_ptr<Input>(new ADS1115Input(trigger.input.input.ads1115, std::static_pointer_cast<ADS1115Device>(devices[trigger.input.input.ads1115.deviceid])));
        break;
    }
    if (input == nullptr)
    {
        return true;
    }
    triggers.push_back(std::unique_ptr<ActivationTrigger>(new ActivationTrigger(trigger, std::move(input), *profile_id)));
    return true;
}
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    profile_args_t *profile_args = (profile_args_t *)*arg;
    proto_Profile profile;
    profile.activationMethod.arg = profile_args;
    profile.activationMethod.funcs.decode = &load_activation_method;
    // only properly load the current profile
    if (profile_args->current == profile_args->target)
    {
        printf("loading profile: %d\r\n", profile_args->current);
        profile.mappings.funcs.decode = &load_mapping;
    }
    else
    {
        printf("skipping profile: %d\r\n", profile_args->current);
        profile.mappings.funcs.decode = nullptr;
    }
    uint16_t mapping_id = 0;
    profile.mappings.arg = &mapping_id;
    pb_decode(stream, proto_Profile_fields, &profile);
    if (profile_args->current == profile_args->target)
    {
        current_type = profile.deviceToEmulate;
    }
    profile_args->current++;
    return true;
}
// We put a ConfigFooter struct at the end of the flash area reserved for FlashPROM. It contains a magicvalue, the size
// of the serialized config data and a CRC of that data. This information allows us to both locate and verify the stored
// data. The serialized data is located directly before the footer:
//
//                       FlashPROM block
// ┌────────────────────────────┴─────────────────────────────┐
// ┌──────────────┬────────────────────────────────────┬──────┐
// │Unused memory │Protobuf data                       │Footer│
// └──────────────┴────────────────────────────────────┴──────┘
//
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
    profile_args_t args = {
        current : 0,
        target : currentProfile
    };
    uint16_t dev_id = 0;
    config.devices.arg = &dev_id;
    config.profiles.funcs.decode = &load_profile;
    config.profiles.arg = &args;
    mappings.clear();
    devices.clear();
    triggers.clear();
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