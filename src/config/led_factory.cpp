#include "config/led_factory.hpp"
#include "profiles/profile.hpp"
#include "leds/leds.hpp"
#include "devices/dmx.hpp"
#include "devices/stp16cpc.hpp"
#include "devices/vtechexpander.hpp"

std::unique_ptr<LedMappingDevice> LedFactory::create_led_device(
    const proto_LedDevice& proto_led_device,
    std::shared_ptr<Profile> profile)
{
    switch (proto_led_device.which_device)
    {
    case proto_LedDevice_rgb_tag:
        if (profile->devices.find(proto_led_device.device.rgb.deviceId) == profile->devices.end()) {
            return nullptr;
        }
        return std::make_unique<RgbLedDevice>(
            proto_led_device.device.rgb,
            std::static_pointer_cast<LedDevice>(profile->devices[proto_led_device.device.rgb.deviceId])
        );
        
    case proto_LedDevice_gpio_tag:
        return std::make_unique<GpioLedDevice>(proto_led_device.device.gpio);
        
    case proto_LedDevice_dmx_tag:
        if (profile->devices.find(proto_led_device.device.dmx.deviceId) == profile->devices.end()) {
            return nullptr;
        }
        return std::make_unique<DMXLedDevice>(
            proto_led_device.device.dmx,
            std::static_pointer_cast<DMXDevice>(profile->devices[proto_led_device.device.dmx.deviceId])
        );
        
    case proto_LedDevice_stp16_tag:
        if (profile->devices.find(proto_led_device.device.stp16.deviceId) == profile->devices.end()) {
            return nullptr;
        }
        return std::make_unique<STP16CPCLedDevice>(
            proto_led_device.device.stp16,
            std::static_pointer_cast<STP16CPCDevice>(profile->devices[proto_led_device.device.stp16.deviceId])
        );
        
    case proto_LedDevice_vtechExpander_tag:
        if (profile->devices.find(proto_led_device.device.vtechExpander.deviceId) == profile->devices.end()) {
            return nullptr;
        }
        return std::make_unique<VTechGuitarIoExpanderLedDevice>(
            proto_led_device.device.vtechExpander,
            std::static_pointer_cast<VTechGuitarIOExpanderDevice>(profile->devices[proto_led_device.device.vtechExpander.deviceId])
        );
        
    default:
        return nullptr;
    }
}

std::unique_ptr<LedMapping> LedFactory::create_led_mapping(
    const proto_LedMapping& proto_led_mapping,
    std::shared_ptr<Profile> profile,
    std::unique_ptr<LedMappingDevice> device,
    std::unique_ptr<Input> input,
    uint32_t led_id)
{
    if (!device) {
        return nullptr;
    }
    
    switch (proto_led_mapping.which_led)
    {
    case proto_LedMapping_inputMapping_tag:
        if (!input) return nullptr;
        return std::make_unique<InputLedMapping>(
            std::move(device),
            proto_led_mapping.led.inputMapping,
            std::move(input),
            profile,
            led_id
        );
        
    case proto_LedMapping_staticMapping_tag:
        return std::make_unique<StaticLedMapping>(
            std::move(device),
            proto_led_mapping.led.staticMapping,
            profile,
            led_id
        );
        
    case proto_LedMapping_patternMapping_tag:
        return std::make_unique<PatternLedMapping>(
            std::move(device),
            proto_led_mapping.led.patternMapping,
            profile,
            led_id
        );
        
    default:
        return nullptr;
    }
}
