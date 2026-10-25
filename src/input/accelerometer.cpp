#include "input/accelerometer.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"
#include "protocols/motion.hpp"

AccelerometerInput::AccelerometerInput(proto_AccelerometerInput input, std::shared_ptr<AccelerometerDevice> device, Profile* profile) : m_input(input), m_device(device)
{
}
bool AccelerometerInput::tick_digital()
{
    return false;
}
// ±2g centred on the middle of the range, clamped as anything past 2g would wrap
static uint16_t raw_axis(int32_t value)
{
    value += INT16_MAX;
    if (value < 0)
        return 0;
    if (value > UINT16_MAX)
        return UINT16_MAX;
    return value;
}
uint16_t AccelerometerInput::tick_analog()
{
    const Accelerometer &accelerometer = m_device->m_accelerometer;
    switch (m_input.type)
    {
    case AccelerometerX:
        return raw_axis(accelerometer.accel[0]);
    case AccelerometerY:
        return raw_axis(accelerometer.accel[1]);
    case AccelerometerZ:
        return raw_axis(accelerometer.accel[2]);
    case AccelerometerMotionX:
    case AccelerometerMotionY:
    case AccelerometerMotionZ:
        return motion_accel_axis(accelerometer.accel[m_input.type - AccelerometerMotionX] / Accelerometer::ONE_G);
    case GyroscopeX:
    case GyroscopeY:
    case GyroscopeZ:
        return motion_gyro_axis(accelerometer.gyro[m_input.type - GyroscopeX] / Accelerometer::GYRO_PER_DPS);
    case AccelerometerAdc1:
        return accelerometer.lis3dhAdc[0];
    case AccelerometerAdc2:
        return accelerometer.lis3dhAdc[1];
    case AccelerometerAdc3:
        return accelerometer.lis3dhAdc[2];
    
    default:
        break;
    }
    return 0;
}
void AccelerometerInput::setup()
{
}