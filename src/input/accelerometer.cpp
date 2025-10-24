#include "input/accelerometer.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

AccelerometerInput::AccelerometerInput(proto_AccelerometerInput input, std::shared_ptr<AccelerometerDevice> device) : m_input(input), m_device(device)
{
}
bool AccelerometerInput::tickDigital()
{
    return false;
}
uint16_t AccelerometerInput::tickAnalog()
{
    switch (m_input.type)
    {
    case AccelerometerX:
        return m_device->m_accelerometer.accel[0] + INT16_MAX;
    case AccelerometerY:
        return m_device->m_accelerometer.accel[1] + INT16_MAX;
    case AccelerometerZ:
        return m_device->m_accelerometer.accel[2] + INT16_MAX;
    case AccelerometerAdc1:
        return m_device->m_accelerometer.lis3dhAdc[0];
    case AccelerometerAdc2:
        return m_device->m_accelerometer.lis3dhAdc[1];
    case AccelerometerAdc3:
        return m_device->m_accelerometer.lis3dhAdc[2];
    
    default:
        break;
    }
    return 0;
}
void AccelerometerInput::setup()
{
}