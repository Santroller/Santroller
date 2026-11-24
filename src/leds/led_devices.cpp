#include "leds/leds.hpp"
#include "instance.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include "devices/bt/bluetooth_stack.hpp"
#include <stdio.h>
#include <algorithm>
bool RgbLedDevice::colour(uint16_t val, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w)
{
    r = ((float)val * scaleR) + startR;
    g = ((float)val * scaleG) + startG;
    b = ((float)val * scaleB) + startB;
    w = ((float)val * scaleBrightness) + m_device.startW;
    return m_device.hasStart || r || g || b;
}
void RgbLedDevice::write_val(uint16_t val)
{
    uint8_t r, g, b, w;
    if (!colour(val, r, g, b, w))
    {
        return;
    }
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], r, g, b, w);
    }
}
void RgbLedDevice::write_val_at(uint8_t i, uint16_t val)
{
    uint8_t r, g, b, w;
    if (colour(val, r, g, b, w))
    {
        m_led_device->set_led(m_device.activeLed[i], r, g, b, w);
    }
}
void RgbLedDevice::write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b, brightness);
}
uint8_t RgbLedDevice::led_count()
{
    return m_device.activeLed_count;
}
LedId RgbLedDevice::led_id(uint8_t i)
{
    return {m_led_device.get(), (uint32_t)m_device.activeLed[i]};
}
bool RgbLedDevice::supports_brightness()
{
    return m_led_device->supports_brightness();
}

void RgbLedDevice::setup()
{
    startR = m_device.startR;
    startG = m_device.startG;
    startB = m_device.startB;
    if (!m_device.hasStart)
    {
        startR = startG = startB = 0;
    }
    endR = m_device.endR;
    endG = m_device.endG;
    endB = m_device.endB;
    if (m_led_device->supports_brightness())
    {
        float startW = m_device.startW;
        float endW = m_device.endW;
        scaleBrightness = ((float)endW - startW) / UINT16_MAX;
    }
    else
    {

        startR *= m_device.startW / 255.0f;
        startG *= m_device.startW / 255.0f;
        startB *= m_device.startW / 255.0f;
        endR *= m_device.endW / 255.0f;
        endG *= m_device.endW / 255.0f;
        endB *= m_device.endW / 255.0f;
        scaleBrightness = 0;
    }
    scaleR = ((float)endR - startR) / UINT16_MAX;
    scaleG = ((float)endG - startG) / UINT16_MAX;
    scaleB = ((float)endB - startB) / UINT16_MAX;
}
void RgbLedDevice::off()
{
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], 0, 0, 0, 0);
    }
}
void RgbLedDevice::test_on()
{
    if (endR || endG || endB)
    {
        write_val(UINT16_MAX);
    }
    else
    {
        for (int i = 0; i < m_device.activeLed_count; i++)
        {
            m_led_device->set_led(m_device.activeLed[i], 255, 255, 255, 255);
        }
    }
}
void STP16CPCLedDevice::write_val(uint16_t val)
{
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], val, val, val, 0);
    }
}
void STP16CPCLedDevice::write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b, brightness);
}
void STP16CPCLedDevice::write_val_at(uint8_t i, uint16_t val)
{
    // On or off, so a partly lit LED rounds to the nearer one
    uint8_t on = val > UINT16_MAX / 2 ? 255 : 0;
    m_led_device->set_led(m_device.activeLed[i], on, on, on, 0);
}
uint8_t STP16CPCLedDevice::led_count()
{
    return m_device.activeLed_count;
}
LedId STP16CPCLedDevice::led_id(uint8_t i)
{
    return {m_led_device.get(), (uint32_t)m_device.activeLed[i]};
}
bool STP16CPCLedDevice::supports_brightness()
{
    return false;
}
void STP16CPCLedDevice::setup()
{
}
void STP16CPCLedDevice::off()
{
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], 0, 0, 0, 0);
    }
}

void VTechGuitarIoExpanderLedDevice::write_val(uint16_t val)
{
    for (int i = 0; i < 8; i++)
    {
        if (m_device.activeLed & 1 << i)
        {
            // led order is 7,6,5,4,0,1,2,3
            int actual = 7 - i;
            if (i > 3)
            {
                actual = i - 4;
            }
            m_led_device->set_led(actual, val);
        }
    }
}
void VTechGuitarIoExpanderLedDevice::write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(index, r || g || b);
}
void VTechGuitarIoExpanderLedDevice::write_val_at(uint8_t index, uint16_t val)
{
    uint32_t bit = led_id(index).index;
    if (bit >= 8)
    {
        return;
    }
    // led order is 7,6,5,4,0,1,2,3, and they're only on or off, so a partly lit LED rounds to the nearer one
    m_led_device->set_led(bit > 3 ? bit - 4 : 7 - bit, val > UINT16_MAX / 2);
}
void VTechGuitarIoExpanderLedDevice::setup()
{
    m_led_count = 0;
    for (int i = 0; i < 8; i++)
    {
        if (m_device.activeLed & 1 << i)
        {
            m_led_count++;
        }
    }
}
bool VTechGuitarIoExpanderLedDevice::supports_brightness()
{
    return false;
}
uint8_t VTechGuitarIoExpanderLedDevice::led_count()
{
    return m_led_count;
}
LedId VTechGuitarIoExpanderLedDevice::led_id(uint8_t i)
{
    // the i'th active LED
    for (uint32_t bit = 0; bit < 8; bit++)
    {
        if ((m_device.activeLed & 1 << bit) && i-- == 0)
        {
            return {m_led_device.get(), bit};
        }
    }
    return {m_led_device.get(), UINT32_MAX};
}
void VTechGuitarIoExpanderLedDevice::off()
{
    for (int i = 0; i < 8; i++)
    {
        if (m_device.activeLed & 1 << i)
        {
            // led order is 7,6,5,4,0,1,2,3
            int actual = 7 - i;
            if (i > 3)
            {
                actual = i - 4;
            }
            m_led_device->set_led(actual, 0);
        }
    }
}
// The onboard LED's pin on a Pico. On a Pico W GP25 is the CYW43's chip select instead, and the LED is
// on the CYW43, so outputs on GP25 drive that while it's in use, as other libraries do
static constexpr int32_t ONBOARD_LED_PIN = 25;

// The onboard LED of a Pico W is on the wireless chip, so it's driven through that while the chip is used
static bool write_cyw43(const proto_GpioLedDevice &led, bool on)
{
    if (led.pin != ONBOARD_LED_PIN || !BluetoothStack::instance().using_cyw43_pin(led.pin))
    {
        return false;
    }
    bool inv = led.has_inverted && led.inverted;
    BluetoothStack::instance().set_cyw43_led(inv ? !on : on);
    return true;
}

void GpioLedDevice::setup_pin(const proto_GpioLedDevice &led)
{
    if (led.pin < 0 || write_cyw43(led, false))
    {
        return;
    }
    bool inv = led.has_inverted && led.inverted;
    if (led.analog)
    {
        gpio_set_function(led.pin, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(led.pin);
        pwm_set_enabled(slice_num, true);
        pwm_set_gpio_level(led.pin, inv ? 65535 : 0);
    }
    else
    {
        gpio_init(led.pin);
        gpio_set_dir(led.pin, true);
        gpio_put(led.pin, inv ? 1 : 0);
    }
}
void GpioLedDevice::write_pin(const proto_GpioLedDevice &led, uint16_t val)
{
    if (led.pin < 0 || write_cyw43(led, val != 0))
    {
        return;
    }
    bool inv = led.has_inverted && led.inverted;
    if (led.analog)
    {
        pwm_set_gpio_level(led.pin, inv ? (65535 - val) : val);
        return;
    }
    gpio_put(led.pin, inv ? !val : !!val);
}
void GpioLedDevice::write_pin_raw(const proto_GpioLedDevice &led, uint8_t r, uint8_t g, uint8_t b)
{
    // Dimmed by red alone, as only RGB LEDs have the other colours
    if (led.analog)
    {
        write_pin(led, ((uint16_t)r << 8) | r);
        return;
    }
    write_pin(led, (r || g || b) ? UINT16_MAX : 0);
}
LedId GpioLedDevice::pin_id(int32_t pin)
{
    static const uint8_t pico_pins = 0;
    return {&pico_pins, (uint32_t)pin};
}

void GpioLedDevice::setup()
{
    setup_pin(m_device);
}
void GpioLedDevice::write_val(uint16_t val)
{
    write_pin(m_device, val);
}
void GpioLedDevice::write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    write_pin_raw(m_device, r, g, b);
}
uint8_t GpioLedDevice::led_count()
{
    return 1;
}
LedId GpioLedDevice::led_id(uint8_t i)
{
    return pin_id(m_device.pin);
}
bool GpioLedDevice::supports_brightness()
{
    return false;
}
void GpioLedDevice::off()
{
    write_pin(m_device, 0);
}

GpioGroupLedDevice::GpioGroupLedDevice(proto_GpioGroupLedDevice device, std::shared_ptr<GpioLedGroupDevice> group)
    : LedMappingDevice(), m_group(std::move(group))
{
    for (pb_size_t i = 0; i < device.activeLed_count; i++)
    {
        if (device.activeLed[i] < m_group->pin_count())
        {
            m_pins.push_back(m_group->pin(device.activeLed[i]));
        }
    }
    setup();
}
void GpioGroupLedDevice::setup()
{
    for (auto &pin : m_pins)
    {
        GpioLedDevice::setup_pin(pin);
    }
}
void GpioGroupLedDevice::write_val(uint16_t val)
{
    for (auto &pin : m_pins)
    {
        GpioLedDevice::write_pin(pin, val);
    }
}
void GpioGroupLedDevice::write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (i < m_pins.size())
    {
        GpioLedDevice::write_pin_raw(m_pins[i], r, g, b);
    }
}
void GpioGroupLedDevice::write_val_at(uint8_t i, uint16_t val)
{
    if (i >= m_pins.size())
    {
        return;
    }
    // Pins that are only on or off round a partly lit LED to the nearer one, as other on / off LEDs do
    if (!m_pins[i].analog)
    {
        val = val > UINT16_MAX / 2 ? UINT16_MAX : 0;
    }
    GpioLedDevice::write_pin(m_pins[i], val);
}
uint8_t GpioGroupLedDevice::led_count()
{
    return m_pins.size();
}
LedId GpioGroupLedDevice::led_id(uint8_t i)
{
    return GpioLedDevice::pin_id(m_pins[i].pin);
}
bool GpioGroupLedDevice::supports_brightness()
{
    return false;
}
void GpioGroupLedDevice::off()
{
    for (auto &pin : m_pins)
    {
        GpioLedDevice::write_pin(pin, 0);
    }
}
void InputLedMapping::reload()
{
    m_device->setup();
    m_input->setup();
}
void PatternLedMapping::reload()
{
    m_device->setup();
}
void StaticLedMapping::reload()
{
    m_device->setup();
}
void DMXLedDevice::setup()
{
}
// Channels are 1 based, as DMX numbers them: byte 0 is the start code, which must stay 0 for fixtures to
// take the packet. Channels the device doesn't send are left alone.
void DMXLedDevice::write_val(uint16_t val)
{
    if (m_led_device->has_channel(m_device.channel))
    {
        m_led_device->set_led(m_device.channel, val >> 8, 0, 0, 0);
    }
}
void DMXLedDevice::write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (m_led_device->has_channel(m_device.channel))
    {
        m_led_device->set_led(m_device.channel, r, g, b, brightness);
    }
}
void DMXLedDevice::off()
{
    if (m_led_device->has_channel(m_device.channel))
    {
        m_led_device->set_led(m_device.channel, 0, 0, 0, 0);
    }
}
uint8_t DMXLedDevice::led_count()
{
    return 1;
}
LedId DMXLedDevice::led_id(uint8_t i)
{
    return {m_led_device.get(), (uint32_t)m_device.channel};
}
bool DMXLedDevice::supports_brightness()
{
    return true;
}

void MPR121LedDevice::setup()
{
    m_led_device->use_gpio_output(m_device.pin);
}
void MPR121LedDevice::write_val(uint16_t val)
{
    m_led_device->set_output(m_device.pin, val);
}
void MPR121LedDevice::write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_output(m_device.pin, r || g || b);
}
void MPR121LedDevice::off()
{
    m_led_device->set_output(m_device.pin, false);
}
bool MPR121LedDevice::supports_brightness()
{
    return false;
}
uint8_t MPR121LedDevice::led_count()
{
    return 1;
}
LedId MPR121LedDevice::led_id(uint8_t i)
{
    return {m_led_device.get(), (uint32_t)m_device.pin};
}

void DisplayLedDevice::set_state(bool visible, uint16_t value)
{
    if (visible == m_visible && value == m_value)
    {
        return;
    }
    // The animation starts over each time it's shown
    if (visible && !m_visible)
    {
        m_frame = 0;
        m_frame_start = millis();
    }
    m_visible = visible;
    m_value = value;
    m_display->element_changed();
}
void DisplayLedDevice::write_val(uint16_t val)
{
    // A bar stays up while it's empty, as does a shape that's outlined while it's off. The rest only show
    // while they're on, and only a bar shows how bright it is, so the others only redraw on and off.
    bool outlined = (m_device.type == DisplayElementRectangle || m_device.type == DisplayElementCircle) &&
                    m_device.has_outlineWhenOff && m_device.outlineWhenOff;
    if (m_device.type == DisplayElementBar)
    {
        set_state(true, val);
        return;
    }
    set_state(outlined || val, val ? 1 : 0);
}
void DisplayLedDevice::write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    write_val(std::max({r, g, b}) * 257);
}
void DisplayLedDevice::off()
{
    set_state(false, 0);
}
void DisplayLedDevice::setup()
{
    if (m_stick_x)
    {
        m_stick_x->setup();
    }
    if (m_stick_y)
    {
        m_stick_y->setup();
    }
}
// How long each frame of an animation shows when the config doesn't say
static constexpr uint32_t DEFAULT_FRAME_MS = 500;
void DisplayLedDevice::tick()
{
    if (!m_visible)
    {
        return;
    }
    if (!m_frames.empty())
    {
        uint32_t frame_ms = m_device.has_frameMs && m_device.frameMs ? m_device.frameMs : DEFAULT_FRAME_MS;
        uint32_t now = millis();
        if (now - m_frame_start >= frame_ms)
        {
            m_frame_start = now;
            m_frame = (m_frame + 1) % (m_frames.size() + 1);
            m_display->element_changed();
        }
    }
    if (m_device.type != DisplayElementStick)
    {
        return;
    }
    uint16_t x = m_stick_x ? m_stick_x->tick_analog() : UINT16_MAX / 2;
    uint16_t y = m_stick_y ? m_stick_y->tick_analog() : UINT16_MAX / 2;
    // Only drawn again once the dot moves a pixel, so a noisy stick doesn't keep redrawing it
    int old_x, old_y, new_x, new_y, size;
    MonoFramebuffer::stick_dot(m_device.width, m_device.height, m_stick_x_value, m_stick_y_value, old_x, old_y, size);
    MonoFramebuffer::stick_dot(m_device.width, m_device.height, x, y, new_x, new_y, size);
    if (old_x != new_x || old_y != new_y)
    {
        m_stick_x_value = x;
        m_stick_y_value = y;
        m_display->element_changed();
    }
}
void DisplayLedDevice::draw(MonoFramebuffer &frame)
{
    if (!m_visible)
    {
        return;
    }
    bool invert = m_device.has_invert && m_device.invert;
    // The current frame of an animation, where it sets anything
    const DisplayFrameData *current = m_frame > 0 && m_frame <= m_frames.size() ? &m_frames[m_frame - 1] : nullptr;
    int32_t x = current && current->frame.has_x ? current->frame.x : m_device.x;
    int32_t y = current && current->frame.has_y ? current->frame.y : m_device.y;
    const char *text = current && current->frame.has_text ? current->frame.text : m_device.text;
    const std::vector<uint8_t> &image = current && !current->image.empty() ? current->image : m_image;
    switch (m_device.type)
    {
    case DisplayElementText:
        frame.draw_text(x, y, text, m_device.has_scale ? m_device.scale : 1, invert);
        break;
    case DisplayElementImage:
        frame.draw_bitmap(x, y, m_device.width, m_device.height, image.data(), image.size(), invert);
        break;
    case DisplayElementBar:
        frame.draw_bar(x, y, m_device.width, m_device.height, m_value, m_device.has_vertical && m_device.vertical, invert);
        break;
    case DisplayElementRectangle:
    case DisplayElementCircle:
        frame.draw_shape(x, y, m_device.width, m_device.height, m_device.type == DisplayElementCircle, m_value, invert);
        break;
    case DisplayElementStick:
        frame.draw_stick(x, y, m_device.width, m_device.height, m_stick_x_value, m_stick_y_value, invert);
        break;
    }
}
