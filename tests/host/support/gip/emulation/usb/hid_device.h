#pragma once
// Fake HID config device: the config tool is never open here, so forced modes apply straight away

class HIDConfigDevice
{
public:
    static bool tool_may_be_open() { return false; }
};
