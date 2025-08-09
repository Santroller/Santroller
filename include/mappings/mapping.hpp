#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "state/base.hpp"
class Mapping
{
public:
    Mapping(uint16_t id): m_id(id) {}
    virtual ~Mapping() {}
    virtual void update(san_base_t *base)=0;
protected:
    uint16_t m_id;
};

class AxisMapping : public Mapping
{
public:
    ~AxisMapping() {}
    AxisMapping(proto_AxisMapping mapping, Input &input, uint16_t id);
    void update(san_base_t *base);

private:
    proto_AxisMapping m_mapping;
    Input &m_input;
};

class ButtonMapping : public Mapping
{
public:
    ~ButtonMapping() {}
    ButtonMapping(proto_ButtonMapping mapping, Input &input, uint16_t id);
    void update(san_base_t *base);

private:
    proto_ButtonMapping m_mapping;
    Input &m_input;
    bool m_lastValue = false;
};