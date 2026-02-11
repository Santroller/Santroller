#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "config.pb.h"
#include "devices/base.hpp"
#include "devices/stp16cpc.hpp"
#include <memory>

class LedMappingDevice
{
public:
    LedMappingDevice() {}
    virtual void set_val(uint16_t val) = 0;
    virtual void setup() = 0;
};
class RgbLedDevice : public LedMappingDevice
{
public:
    RgbLedDevice(proto_RGBLedDevice device, std::shared_ptr<LedDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(led_device) {}
    void set_val(uint16_t val);
    void setup();

protected:
    proto_RGBLedDevice m_device;
    std::shared_ptr<LedDevice> m_led_device;
};
class GpioLedDevice : public LedMappingDevice
{
public:
    GpioLedDevice(proto_GpioLedDevice device) : LedMappingDevice(), m_device(device) {}
    void set_val(uint16_t val);
    void setup();

protected:
    proto_GpioLedDevice m_device;
};
class STP16CPCLedDevice : public LedMappingDevice
{
public:
    STP16CPCLedDevice(proto_STP16CPCLedDevice device, std::shared_ptr<STP16CPCDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(led_device) {}
    void set_val(uint16_t val);
    void setup();

protected:
    proto_STP16CPCLedDevice m_device;
    std::shared_ptr<STP16CPCDevice> m_led_device;
};
class LedMapping
{
public:
    LedMapping(uint16_t id, uint32_t profile, std::unique_ptr<LedMappingDevice> device) : m_id(id), m_profile(profile), m_device(std::move(device)) {}
    virtual ~LedMapping() {}
    virtual void update() = 0;
    virtual void reload() = 0;

protected:
    uint16_t m_id;
    uint32_t m_profile;
    std::unique_ptr<LedMappingDevice> m_device;
};

class InputLedMapping : public LedMapping
{
public:
    InputLedMapping(std::unique_ptr<LedMappingDevice> device, proto_InputLedMapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : LedMapping(id, profile, std::move(device)), m_mapping(mapping) {}
    void update();
    void reload();

protected:
    std::unique_ptr<Input> m_input;
    proto_InputLedMapping m_mapping;
};

class PatternLedMapping : public LedMapping
{
public:
    PatternLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PatternLedMapping mapping, uint16_t id, uint32_t profile) : LedMapping(id, profile, std::move(device)), m_mapping(mapping) {}
    void update();
    void reload();

protected:
    proto_PatternLedMapping m_mapping;
};

class StaticLedMapping : public LedMapping
{
public:
    StaticLedMapping(std::unique_ptr<LedMappingDevice> device, proto_StaticLedMapping mapping, uint16_t id, uint32_t profile) : LedMapping(id, profile, std::move(device)), m_mapping(mapping) {}
    void update();
    void reload();

protected:
    proto_StaticLedMapping m_mapping;
};
