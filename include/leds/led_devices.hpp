#pragma once
#include "leds/base_led.hpp"
#include "config.pb.h"
#include "devices/base.hpp"
#include "devices/dmx.hpp"
#include "devices/stp16cpc.hpp"
#include "devices/vtechexpander.hpp"
#include "devices/mpr121.hpp"
#include "devices/display.hpp"
#include "input/input.hpp"
#include <memory>
#include <vector>

class RgbLedDevice : public LedMappingDevice
{
public:
    RgbLedDevice(proto_RGBLedDevice device, std::shared_ptr<LedDevice> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
    void off();
    void test_on() override;
    void write_val(uint16_t val);
    void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();
    LedId led_id(uint8_t i);

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
    void write_val(uint16_t val);
    void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();
    LedId led_id(uint8_t i);

protected:
    bool write_cyw43(bool on);
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
    void write_val(uint16_t val);
    void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();
    LedId led_id(uint8_t i);

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
    void write_val(uint16_t val);
    void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();
    LedId led_id(uint8_t i);

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
    void write_val(uint16_t val);
    void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();
    LedId led_id(uint8_t i);

protected:
    proto_DMXLedDevice m_device;
    std::shared_ptr<DMXDevice> m_led_device;
};

class MPR121LedDevice : public LedMappingDevice
{
public:
    MPR121LedDevice(proto_MPR121LedDevice device, std::shared_ptr<MPR121Device> led_device) : LedMappingDevice(), m_device(device), m_led_device(std::move(led_device))
    {
        setup();
    }
    void off();
    void write_val(uint16_t val);
    void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness();
    uint8_t led_count();
    LedId led_id(uint8_t i);

protected:
    proto_MPR121LedDevice m_device;
    std::shared_ptr<MPR121Device> m_led_device;
};

// One element on a display: text and images show while the value is non zero, bars are filled by it. Each
// one is its own LED, so they never cover each other up.
class DisplayLedDevice : public LedMappingDevice, public DisplayElement
{
public:
    // stick_x and stick_y move a joystick's dot, they're null for the other elements
    DisplayLedDevice(proto_DisplayLedDevice device, std::vector<uint8_t> image, std::unique_ptr<Input> stick_x,
                     std::unique_ptr<Input> stick_y, std::shared_ptr<DisplayDevice> display)
        : LedMappingDevice(), m_device(device), m_image(std::move(image)), m_stick_x(std::move(stick_x)),
          m_stick_y(std::move(stick_y)), m_display(std::move(display))
    {
        m_display->add_element(this);
    }
    ~DisplayLedDevice() { m_display->remove_element(this); }
    void off();
    void write_val(uint16_t val);
    void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    void setup();
    bool supports_brightness() { return false; }
    uint8_t led_count() { return 1; }
    LedId led_id(uint8_t i) { return {this, 0}; }
    void draw(MonoFramebuffer &frame);
    void tick();

protected:
    void set_state(bool visible, uint16_t value);
    proto_DisplayLedDevice m_device;
    std::vector<uint8_t> m_image;
    std::unique_ptr<Input> m_stick_x;
    std::unique_ptr<Input> m_stick_y;
    std::shared_ptr<DisplayDevice> m_display;
    bool m_visible = false;
    uint16_t m_value = 0;
    // Where a joystick is, centred until its inputs say otherwise
    uint16_t m_stick_x_value = UINT16_MAX / 2;
    uint16_t m_stick_y_value = UINT16_MAX / 2;
};
