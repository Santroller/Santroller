#include "config.hpp"
#include "input/input.hpp"
#include "input/gpio.hpp"
#include "mappings/mapping.hpp"
#include "protocols/hid.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include <vector>
#include <memory>
/* TODO
load_device should construct some form of device and throw it in a map using the dev_id

the easiest way to do this, is we have a class for each device type, mapping type and each input type. when we get the mapping, we construct an instance of each, push it to a list, and then polling is simply walking the list and updating each thing.
and we would walk the map of devices and poll each device too.
if that ends up not being performant enough THEN we can think about something more optimal but that feels like the clean method.

*/
std::vector<std::unique_ptr<Mapping>> mappings;

void update()
{
    san_base_t gamepad = {0};
    for (auto &mapping : mappings)
    {
        mapping->update(&gamepad);
    }
    PCGamepad_Data_t out = {0};
    out.a = gamepad.gamepad.a;

    tud_hid_report(REPORT_ID_GAMEPAD, &out, sizeof(out));
}

bool load_device(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_Device device;
    uint16_t *dev_id = (uint16_t *)*arg;
    pb_decode(stream, proto_Device_fields, &device);
    switch (device.which_device)
    {
    case proto_Device_adxl_tag:
        printf("adxl %d %d\r\n", *dev_id, device.device.adxl.i2c.sda);
        break;
    case proto_Device_wii_tag:
        printf("wii %d %d\r\n", *dev_id, device.device.wii.i2c.sda);
        break;
    case proto_Device_usbHost_tag:
        printf("usbhost %d %d\r\n", *dev_id, device.device.usbHost.firstPin, device.device.usbHost.dmFirst);
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
    Input *input = nullptr;
    switch (mapping.input.which_input)
    {
    case proto_Input_analogDevice_tag:
        printf("analogDevice %d\r\n", mapping.input.input.analogDevice.deviceid);
        break;
    case proto_Input_digitalDevice_tag:
        printf("digitalDevice %d\r\n", mapping.input.input.digitalDevice.deviceid);
        break;
    case proto_Input_gpio_tag:
        printf("gpio %d %d\r\n", mapping.input.input.gpio.pin, mapping.input.input.gpio.pinMode);
        input = new GPIOInput(mapping.input.input.gpio);
    }
    if (input == nullptr)
    {
        return true;
    }
    switch (mapping.which_mapping)
    {
    case proto_Mapping_axis_tag:
        printf("axis %d\r\n", mapping.mapping.axis.axis);
        mappings.push_back(std::unique_ptr<Mapping>(new AxisMapping(mapping.mapping.axis, *input, *mapping_id)));
        break;
    case proto_Mapping_button_tag:
        printf("button %d\r\n", mapping.mapping.button.button);
        mappings.push_back(std::unique_ptr<Mapping>(new ButtonMapping(mapping.mapping.button, *input, *mapping_id)));
        break;
    }
    *mapping_id += 1;
    return true;
}
bool load_profile(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    proto_Profile profile;
    profile.mappings.funcs.decode = &load_mapping;
    profile.activationMethod.funcs.decode = &load_mapping;
    uint16_t mapping_id = 0;
    profile.mappings.arg = &mapping_id;
    pb_decode(stream, proto_Profile_fields, &profile);
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
    if (!pb_encode(&outputStream, proto_Config_fields, &config))
    {
        return false;
    }

    // Create the new footer
    ConfigFooter newFooter;
    newFooter.dataSize = outputStream.bytes_written;
    newFooter.dataCrc = CRC32::calculate(EEPROM.writeCache, newFooter.dataSize);
    newFooter.magic = FOOTER_MAGIC;

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

bool inner_load(proto_Config &config, const uint8_t *dataPtr, uint32_t size) {
    // We are now sufficiently confident that the data is valid so we run the deserialization
    pb_istream_t inputStream = pb_istream_from_buffer(dataPtr, size);
    config.devices.funcs.decode = &load_device;
    uint16_t dev_id = 0;
    config.devices.arg = &dev_id;
    config.profiles.funcs.decode = &load_profile;
    return pb_decode(&inputStream, proto_Config_fields, &config);
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
    inner_load(config, EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize, footer.dataSize);
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

    return inner_load(config, dataPtr, footer.dataSize);
}