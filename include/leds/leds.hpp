#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "config.pb.h"
#include "devices/base.hpp"
#include "devices/stp16cpc.hpp"
#include <memory>
#include <stdio.h>

class LedMappingDevice
{
public:
    LedMappingDevice() {}
    virtual ~LedMappingDevice() {}
    virtual void set_val(uint16_t val) = 0;
    virtual void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) = 0;
    virtual void setup() = 0;
    virtual bool supports_brightness() = 0;
    virtual uint8_t led_count() = 0;
};
class RgbLedDevice : public LedMappingDevice
{
public:
    RgbLedDevice(proto_RGBLedDevice device, std::shared_ptr<LedDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
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
};
class GpioLedDevice : public LedMappingDevice
{
public:
    GpioLedDevice(proto_GpioLedDevice device) : LedMappingDevice(), m_device(device)
    {
        setup();
    }
    void set_val(uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();

protected:
    proto_GpioLedDevice m_device;
};
class STP16CPCLedDevice : public LedMappingDevice
{
public:
    STP16CPCLedDevice(proto_STP16CPCLedDevice device, std::shared_ptr<STP16CPCDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
    void set_val(uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();

protected:
    proto_STP16CPCLedDevice m_device;
    std::shared_ptr<STP16CPCDevice> m_led_device;
};
class LedMapping
{
public:
    LedMapping(std::unique_ptr<LedMappingDevice> device, uint32_t profile_id, uint32_t id) : m_device(std::move(device)), m_id(id), m_profile_id(profile_id) {}
    virtual ~LedMapping() {}
    virtual void update(bool full_poll, bool send_events) = 0;
    virtual void reload() = 0;

protected:
    std::unique_ptr<LedMappingDevice> m_device;
    uint32_t m_id;
    uint32_t m_profile_id;
};

class InputLedMapping : public LedMapping
{
public:
    InputLedMapping(std::unique_ptr<LedMappingDevice> device, proto_InputLedMapping mapping, std::unique_ptr<Input> input, uint32_t profile_id, uint32_t id) : LedMapping(std::move(device), profile_id, id), m_input(std::move(input)), m_mapping(mapping) {}
    void update(bool full_poll, bool send_events);
    void reload();

protected:
    std::unique_ptr<Input> m_input;
    proto_InputLedMapping m_mapping;

private:
    uint16_t m_last_val = 0;
    bool m_resend = false;
    uint32_t m_last_poll = 0;
};

class PatternLedMapping : public LedMapping
{
public:
    PatternLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PatternLedMapping mapping, uint32_t profile_id, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();

protected:
    proto_PatternLedMapping m_mapping;
    uint32_t m_pos = 0;
    bool m_dir = false;
    uint32_t m_next_poll = 0;
    uint32_t m_speed = 0;
    uint32_t m_brightness = 0;
};

class StaticLedMapping : public LedMapping
{
public:
    StaticLedMapping(std::unique_ptr<LedMappingDevice> device, proto_StaticLedMapping mapping, uint32_t profile_id, uint32_t id) : LedMapping(std::move(device), profile_id, id), m_mapping(mapping) {}
    void update(bool full_poll, bool send_events);
    void reload();

protected:
    proto_StaticLedMapping m_mapping;
};
