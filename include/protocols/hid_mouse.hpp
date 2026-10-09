#pragma once
#include <stdint.h>
#include <string.h>
#include "class/hid/hid.h"
#include "hidparser.h"

// Mice send reports only while something changes, so movement older than this has stopped
#define MOUSE_IDLE_US 20000
// Movement per report is small, scale it up into an axis (128 counts / 4 scroll notches = full)
#define MOUSE_MOVE_SCALE 256
#define MOUSE_SCROLL_SCALE 8192

// Mouse reports, read using the mouse's report descriptor (report protocol, so the wheel is available,
// unlike in boot protocol)
class HidMouseDecoder
{
public:
    // in the order of MouseAxisType: x, y, horizontal scroll, vertical scroll
    enum Axis : uint8_t
    {
        MoveX,
        MoveY,
        ScrollX,
        ScrollY,
        AxisCount
    };
    // HID buttons 1 / 2 / 3
    enum Button : uint8_t
    {
        Left,
        Right,
        Middle,
        ButtonCount
    };

    void set_report_info(HID_ReportInfo_t *info)
    {
        memset(m_axis_items, 0, sizeof(m_axis_items));
        memset(m_button_items, 0, sizeof(m_button_items));
        if (!info)
        {
            return;
        }
        for (HID_ReportItem_t *item = info->FirstReportItem; item; item = item->Next)
        {
            const auto &usage = item->Attributes.Usage;
            if (usage.Page == HID_USAGE_PAGE_DESKTOP)
            {
                switch (usage.Usage)
                {
                case HID_USAGE_DESKTOP_X:
                    m_axis_items[MoveX] = item;
                    break;
                case HID_USAGE_DESKTOP_Y:
                    m_axis_items[MoveY] = item;
                    break;
                case HID_USAGE_DESKTOP_WHEEL:
                    m_axis_items[ScrollY] = item;
                    break;
                }
            }
            else if (usage.Page == HID_USAGE_PAGE_CONSUMER && usage.Usage == HID_USAGE_CONSUMER_AC_PAN)
            {
                m_axis_items[ScrollX] = item;
            }
            else if (usage.Page == HID_USAGE_PAGE_BUTTON && usage.Usage >= 1 && usage.Usage <= ButtonCount)
            {
                m_button_items[usage.Usage - 1] = item;
            }
        }
    }

    // report starts with its report id when the mouse uses them
    void handle_report(const uint8_t *report, uint32_t now_us)
    {
        bool updated = false;
        for (uint8_t i = 0; i < AxisCount; i++)
        {
            if (m_axis_items[i] && read_item(report, m_axis_items[i]))
            {
                m_movement[i] = signed_value(m_axis_items[i]);
                updated = true;
            }
        }
        for (uint8_t i = 0; i < ButtonCount; i++)
        {
            if (m_button_items[i] && read_item(report, m_button_items[i]))
            {
                m_buttons = m_button_items[i]->Value ? m_buttons | (1 << i) : m_buttons & ~(1 << i);
                updated = true;
            }
        }
        if (updated)
        {
            m_last_report_us = now_us;
        }
    }

    bool button(Button button) const
    {
        return button < ButtonCount && (m_buttons & (1 << button));
    }

    // centred at rest, movement since the last report scaled out from there
    uint16_t axis(Axis axis, uint32_t now_us) const
    {
        const int32_t center = UINT16_MAX / 2;
        if (axis >= AxisCount || now_us - m_last_report_us > MOUSE_IDLE_US)
        {
            return center;
        }
        int32_t value = center + m_movement[axis] * (axis < ScrollX ? MOUSE_MOVE_SCALE : MOUSE_SCROLL_SCALE);
        return value < 0 ? 0 : value > UINT16_MAX ? UINT16_MAX : value;
    }

private:
    // Read an item out of a report, which starts with its report id when the device uses them
    static bool read_item(const uint8_t *report, HID_ReportItem_t *item)
    {
        if (item->ReportID)
        {
            if (item->ReportID != report[0])
            {
                return false;
            }
            report++;
        }
        return USB_GetHIDReportItemInfo(item->ReportID, report, item);
    }

    static int32_t signed_value(const HID_ReportItem_t *item)
    {
        uint8_t bits = item->Attributes.BitSize;
        int32_t value = item->Value;
        // relative axes have a negative logical minimum, so sign extend them. The parser keeps the raw
        // item data, so a negative minimum (e.g. 0x81 for -127) shows up as larger than the maximum
        bool is_signed = item->Attributes.Logical.Minimum > item->Attributes.Logical.Maximum;
        if (is_signed && bits && bits < 32 && (value & (1 << (bits - 1))))
        {
            value -= 1 << bits;
        }
        return value;
    }

    HID_ReportItem_t *m_axis_items[AxisCount] = {nullptr};
    HID_ReportItem_t *m_button_items[ButtonCount] = {nullptr};
    // latest movement, and when it arrived
    int32_t m_movement[AxisCount] = {0};
    uint8_t m_buttons = 0;
    uint32_t m_last_report_us = 0;
};
