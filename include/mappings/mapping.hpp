#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "state/base.hpp"
class Mapping
{
public:
    Mapping() {}
    virtual ~Mapping() {}
    virtual void update(san_base_t *base);
};

class AxisMapping : public Mapping
{
public:
    AxisMapping(proto_AxisMapping mapping, Input &input);
    void update(san_base_t *base);

private:
    proto_AxisMapping m_mapping;
    Input &m_input;
};

class ButtonMapping : public Mapping
{
public:
    ButtonMapping(proto_ButtonMapping mapping, Input &input);
    void update(san_base_t *base);

private:
    proto_ButtonMapping m_mapping;
    Input &m_input;
};