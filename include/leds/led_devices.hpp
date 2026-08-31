#pragma once
#include "leds/base_led.hpp"
#include "config.pb.h"
#include "devices/base.hpp"
#include "devices/dmx.hpp"
#include "devices/stp16cpc.hpp"
#include "devices/vtechexpander.hpp"
#include <memory>

class RgbLedDevice : public LedMappingDevice
{
public:
    RgbLedDevice(proto_RGBLedDevice device, std::shared_ptr<LedDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
    void off();
    void set_val(uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();

protected:
    proto_RGBLedDevice m_device;
    std::shared_ptr<LedDevice> m_led_device;
    float scaleR = 0;
    float scaleG = 0;
    float scaleB = 0;
    float scaleBrightness = 0;
    uint16_t startR;
    uint16_t startG;
    uint16_t startB;
    uint16_t endR;
    uint16_t endG;
    uint16_t endB;
    uint32_t m_pos = 0;
};

class GpioLedDevice : public LedMappingDevice
{
public:
    GpioLedDevice(proto_GpioLedDevice device) : LedMappingDevice(), m_device(device)
    {
        setup();
    }
    void off();
    void set_val(uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();

protected:
    proto_GpioLedDevice m_device;
};

class VTechGuitarIoExpanderLedDevice : public LedMappingDevice
{
public:
    VTechGuitarIoExpanderLedDevice(proto_VTechGuitarIOExpanderLedDevice device, std::shared_ptr<VTechGuitarIOExpanderDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
    void off();
    void set_val(uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();

protected:
    proto_VTechGuitarIOExpanderLedDevice m_device;
    std::shared_ptr<VTechGuitarIOExpanderDevice> m_led_device;
    uint8_t m_led_count = 0;
};

class STP16CPCLedDevice : public LedMappingDevice
{
public:
    STP16CPCLedDevice(proto_STP16CPCLedDevice device, std::shared_ptr<STP16CPCDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
    void off();
    void set_val(uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();

protected:
    proto_STP16CPCLedDevice m_device;
    std::shared_ptr<STP16CPCDevice> m_led_device;
};

class DMXLedDevice : public LedMappingDevice
{
public:
    DMXLedDevice(proto_DMXLedDevice device, std::shared_ptr<DMXDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
    void off();
    void set_val(uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();

protected:
    proto_DMXLedDevice m_device;
    std::shared_ptr<DMXDevice> m_led_device;
};
