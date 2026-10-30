#include "leds/leds.hpp"
#include "instance.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include "devices/bt/bluetooth_stack.hpp"
#include <stdio.h>
void RgbLedDevice::write_val(uint16_t val)
{
    uint16_t r = ((float)val * scaleR) + startR;
    uint16_t g = ((float)val * scaleG) + startG;
    uint16_t b = ((float)val * scaleB) + startB;
    uint16_t w = ((float)val * scaleBrightness) + m_device.startW;
    if (!m_device.hasStart && !r && !g && !b)
    {
        return;
    }
    for (int i = 0; i < m_device.activeLed_count; i++)
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

bool GpioLedDevice::write_cyw43(bool on)
{
    if (m_device.pin != ONBOARD_LED_PIN || !BluetoothStack::instance().using_cyw43_pin(m_device.pin))
    {
        return false;
    }
    bool inv = m_device.has_inverted && m_device.inverted;
    BluetoothStack::instance().set_cyw43_led(inv ? !on : on);
    return true;
}

void GpioLedDevice::setup()
{
    if (write_cyw43(false))
    {
        return;
    }
    bool inv = m_device.has_inverted && m_device.inverted;
    if (m_device.analog)
    {
        gpio_set_function(m_device.pin, GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(m_device.pin);
        pwm_set_enabled(slice_num, true);
        pwm_set_gpio_level(m_device.pin, inv ? 65535 : 0);
    }
    else
    {
        gpio_init(m_device.pin);
        gpio_set_dir(m_device.pin, true);
        gpio_put(m_device.pin, inv ? 1 : 0);
    }
}
void GpioLedDevice::write_val(uint16_t val)
{
    if (write_cyw43(val != 0))
    {
        return;
    }
    bool inv = m_device.has_inverted && m_device.inverted;
    if (m_device.analog)
    {
        pwm_set_gpio_level(m_device.pin, inv ? (65535 - val) : val);
        return;
    }
    gpio_put(m_device.pin, inv ? !val : !!val);
}
void GpioLedDevice::write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (write_cyw43(r || g || b))
    {
        return;
    }
    bool inv = m_device.has_inverted && m_device.inverted;
    if (m_device.analog)
    {
        uint16_t val = ((uint16_t)r << 8) | r;
        pwm_set_gpio_level(m_device.pin, inv ? (65535 - val) : val);
        return;
    }
    bool on = r || g || b;
    gpio_put(m_device.pin, inv ? !on : on);
}
uint8_t GpioLedDevice::led_count()
{
    return 1;
}
LedId GpioLedDevice::led_id(uint8_t i)
{
    // Every GPIO LED shares the Pico's pins
    static const uint8_t pico_pins = 0;
    return {&pico_pins, (uint32_t)m_device.pin};
}
bool GpioLedDevice::supports_brightness()
{
    return false;
}
void GpioLedDevice::off()
{
    if (write_cyw43(false))
    {
        return;
    }
    bool inv = m_device.has_inverted && m_device.inverted;
    if (m_device.analog)
    {
        pwm_set_gpio_level(m_device.pin, inv ? 65535 : 0);
        return;
    }
    gpio_put(m_device.pin, inv ? 1 : 0);
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
void DMXLedDevice::write_val(uint16_t val)
{
    m_led_device->set_led(m_device.channel, val, val, val, 0);
}
void DMXLedDevice::write_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(m_device.channel, r, g, b, brightness);
}
void DMXLedDevice::off()
{
    m_led_device->set_led(m_device.channel, 0, 0, 0, 0);
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
