#include "leds/leds.hpp"
#include "instance.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
void RgbLedDevice::set_val(uint16_t val)
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
void RgbLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b, brightness);
}
uint8_t RgbLedDevice::led_count()
{
    return m_device.activeLed_count;
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
void STP16CPCLedDevice::set_val(uint16_t val)
{
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], val, val, val, 0);
    }
}
void STP16CPCLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b, brightness);
}
uint8_t STP16CPCLedDevice::led_count()
{
    return m_device.activeLed_count;
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

void VTechGuitarIoExpanderLedDevice::set_val(uint16_t val)
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
void VTechGuitarIoExpanderLedDevice::set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
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
void GpioLedDevice::setup()
{
    if (m_device.analog)
    {
        gpio_set_function(m_device.pin, GPIO_FUNC_PWM);
        pwm_set_gpio_level(m_device.pin, 0);
    }
    else
    {
        gpio_init(m_device.pin);
        gpio_set_dir(m_device.pin, true);
    }
}
void GpioLedDevice::set_val(uint16_t val)
{
    if (m_device.analog)
    {
        pwm_set_gpio_level(m_device.pin, val);
        return;
    }
    gpio_put(m_device.pin, val);
}
void GpioLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    if (m_device.analog)
    {
        pwm_set_gpio_level(m_device.pin, r);
    }
    gpio_put(m_device.pin, r || g || b);
}
uint8_t GpioLedDevice::led_count()
{
    return 1;
}
bool GpioLedDevice::supports_brightness()
{
    return false;
}
void GpioLedDevice::off()
{
    gpio_put(m_device.pin, 0);
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
void DMXLedDevice::set_val(uint16_t val)
{
    m_led_device->set_led(m_device.channel, val, val, val, 0);
}
void DMXLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
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
bool DMXLedDevice::supports_brightness()
{
    return true;
}

void LedMapping::off()
{
    m_device->off();
}