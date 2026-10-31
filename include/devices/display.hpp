#pragma once
#include <vector>
#include "base.hpp"
#include "device.pb.h"
#include "libdisplay.hpp"

// Something drawn on a display, a display LED
class DisplayElement
{
public:
    virtual ~DisplayElement() {}
    virtual void draw(MonoFramebuffer &frame) = 0;
    // Every update, for elements that follow something themselves rather than their LED value
    virtual void tick() {}
};

class DisplayDevice : public Device
{
public:
    ~DisplayDevice() {}
    DisplayDevice(proto_DisplayDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    // Elements are drawn in the order they were added, later ones over the top
    void add_element(DisplayElement *element);
    void remove_element(DisplayElement *element);
    // An element changed what it shows, so the frame is drawn again
    void element_changed() { m_dirty = true; }

private:
    MonoDisplay m_display;
    proto_DisplayDevice m_device;
    std::vector<DisplayElement *> m_elements;
    bool m_dirty = true;
    // Partway through drawing a frame, up to m_draw_index
    bool m_drawing = false;
    size_t m_draw_index = 0;
    uint32_t m_last_draw = 0;
};
