#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "state/base.hpp"
#include "protocols/controller_reports.hpp"
#include "protocols/hid.hpp"
#include "protocols/og_xbox.hpp"
#include "protocols/ps2.hpp"
#include "protocols/ps3.hpp"
#include "protocols/ps4.hpp"
#include "protocols/xbox_one.hpp"
#include "protocols/xinput.hpp"
#include <memory>
class Mapping
{
public:
    Mapping(uint16_t id): m_id(id) {}
    virtual ~Mapping() {}
    virtual void update(bool full_poll)=0;
    // virtual void update_hid(PCGamepad_Data_t *report, bool full_poll)=0;
    // virtual void update_ps3(san_base_t *base, bool full_poll)=0;
protected:
    uint16_t m_id;
};

class AxisMapping : public Mapping
{
public:
    ~AxisMapping() {}
    AxisMapping(proto_AxisMapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update(bool full_poll);

private:
    proto_AxisMapping m_mapping;
    std::unique_ptr<Input> m_input;
    uint32_t m_lastValue = 0;
};

class ButtonMapping : public Mapping
{
public:
    ~ButtonMapping() {}
    ButtonMapping(proto_ButtonMapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update(bool full_poll);

private:
    proto_ButtonMapping m_mapping;
    std::unique_ptr<Input> m_input;
    bool m_lastValue = false;
};