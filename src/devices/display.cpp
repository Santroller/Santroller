#include "devices/display.hpp"
#include <algorithm>
#include "events.pb.h"
#include "main.hpp"
#include "utils.h"
#include "emulation/usb/hid_device.h"
#include "managers/inactivity_manager.hpp"

// Frames are drawn at most this often, which is plenty for a gauge following an axis, and this often while
// the display isn't answering
#define DISPLAY_DRAW_INTERVAL_MS 33
#define DISPLAY_DISCONNECTED_DRAW_INTERVAL_MS 100

static mono_display_controller_e controller(DisplayController controller)
{
    switch (controller)
    {
    case DisplaySH1106:
        return MONO_DISPLAY_SH1106;
    case DisplaySSD1309:
        return MONO_DISPLAY_SSD1309;
    case DisplaySH1107:
        return MONO_DISPLAY_SH1107;
    default:
        return MONO_DISPLAY_SSD1306;
    }
}

DisplayDevice::DisplayDevice(proto_DisplayDevice device, uint16_t id)
    : Device(id),
      m_display(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock,
                device.has_address ? device.address : 0x3C, controller(device.controller),
                device.has_width ? device.width : 128, device.has_height ? device.height : 64,
                device.has_rotation ? device.rotation : 0,
                device.has_contrast ? std::min<uint32_t>(device.contrast, 255) : 207),
      m_device(device)
{
}

void DisplayDevice::begin()
{
    m_display.begin();
}

void DisplayDevice::end(bool full)
{
    m_display.end();
}

void DisplayDevice::add_element(DisplayElement *element)
{
    m_elements.push_back(element);
    m_dirty = true;
    // A frame being drawn would skip or repeat elements now, so it starts again
    m_drawing = false;
}

void DisplayDevice::remove_element(DisplayElement *element)
{
    m_elements.erase(std::remove(m_elements.begin(), m_elements.end(), element), m_elements.end());
    m_dirty = true;
    m_drawing = false;
}

void DisplayDevice::update(bool full_poll, bool send_events)
{
    // Elements can change every update (eg a bar following an axis), but drawing faster than the display
    // takes frames would be wasted, so the next one waits until the last one has been sent
    uint32_t since_draw = millis() - m_last_draw;
    bool can_draw = m_display.is_connected()
                        ? m_display.caught_up() && since_draw >= DISPLAY_DRAW_INTERVAL_MS
                        : since_draw >= DISPLAY_DISCONNECTED_DRAW_INTERVAL_MS;
    MonoFramebuffer &frame = m_display.frame();
    if (!m_drawing && m_dirty && can_draw)
    {
        m_dirty = false;
        m_drawing = true;
        m_draw_index = 0;
        m_last_draw = millis();
        frame.clear();
    }
    // One element each update, so drawing a frame never holds up the inputs for long. Nothing is sent
    // until the whole frame is drawn.
    if (m_drawing)
    {
        if (m_draw_index < m_elements.size())
        {
            m_elements[m_draw_index++]->draw(frame);
        }
        if (m_draw_index >= m_elements.size())
        {
            m_drawing = false;
            m_display.frame_changed();
        }
    }
    // Goes dark with the LEDs, keeping what it shows for when they come back
    m_display.set_on(!InactivityManager::instance().leds_off());
    m_display.tick();
    if (m_lastConnected != m_display.is_connected() || full_poll)
    {
        m_lastConnected = m_display.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool DisplayDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}
