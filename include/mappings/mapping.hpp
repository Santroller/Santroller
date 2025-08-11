#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "state/base.hpp"
#include <memory>
class Mapping
{
public:
    Mapping(uint16_t id): m_id(id) {}
    virtual ~Mapping() {}
    virtual void update(san_base_t *base, bool resend_events)=0;
protected:
    uint16_t m_id;
};

class AxisMapping : public Mapping
{
public:
    ~AxisMapping() {}
    AxisMapping(proto_AxisMapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update(san_base_t *base, bool resend_events);

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
    void update(san_base_t *base, bool resend_events);

private:
    proto_ButtonMapping m_mapping;
    std::unique_ptr<Input> m_input;
    bool m_lastValue = false;
};