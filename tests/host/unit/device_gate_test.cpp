#include <gtest/gtest.h>
#include "config/device_gate.hpp"

namespace
{
proto_DeviceGate digital(PinMode mode)
{
    proto_DeviceGate gate = proto_DeviceGate_init_zero;
    gate.pin = 2;
    gate.pinMode = mode;
    return gate;
}

proto_DeviceGate analog(uint32_t min_mv, uint32_t max_mv)
{
    proto_DeviceGate gate = proto_DeviceGate_init_zero;
    gate.pin = 26;
    gate.pinMode = PinMode_Floating;
    gate.analog = true;
    gate.minMv = min_mv;
    gate.maxMv = max_mv;
    return gate;
}
} // namespace

TEST(DeviceGate, PullUpIsActiveLow)
{
    EXPECT_TRUE(device_gate_matches(digital(PinMode_PullUp), false, 0));
    EXPECT_FALSE(device_gate_matches(digital(PinMode_PullUp), true, 0));
}

TEST(DeviceGate, PullDownIsActiveHigh)
{
    EXPECT_TRUE(device_gate_matches(digital(PinMode_PullDown), true, 0));
    EXPECT_FALSE(device_gate_matches(digital(PinMode_PullDown), false, 0));
}

TEST(DeviceGate, AnalogRangeIsInclusive)
{
    auto gate = analog(1000, 1500);
    EXPECT_FALSE(device_gate_matches(gate, false, 999));
    EXPECT_TRUE(device_gate_matches(gate, false, 1000));
    EXPECT_TRUE(device_gate_matches(gate, false, 1500));
    EXPECT_FALSE(device_gate_matches(gate, false, 1501));
}

TEST(DeviceGate, ChangeHasToSettle)
{
    DeviceGateState state{1, analog(0, 100), false};
    EXPECT_FALSE(state.update(true, 1000));
    EXPECT_FALSE(state.update(true, 1000 + DeviceGateState::SETTLE_MS - 1));
    EXPECT_TRUE(state.update(true, 1000 + DeviceGateState::SETTLE_MS));
}

TEST(DeviceGate, BounceRestartsSettling)
{
    DeviceGateState state{1, analog(0, 100), true};
    EXPECT_FALSE(state.update(false, 0));
    EXPECT_FALSE(state.update(true, 50));
    EXPECT_FALSE(state.update(false, 60));
    EXPECT_FALSE(state.update(false, 60 + DeviceGateState::SETTLE_MS - 1));
    EXPECT_TRUE(state.update(false, 60 + DeviceGateState::SETTLE_MS));
}
