#include "inputs_test_support.hpp"
#include "config/input_decoder.hpp"
#include "config/input_factory.hpp"
#include "input/held.hpp"
#include "input/shifted.hpp"
#include "input/shortcut.hpp"
#include "devices/cycle.hpp"
#include "devices/toggle.hpp"
#include <pb_encode.h>

// Only the hardware leaf is faked; the protobuf decoder and every modifier are production code.
std::unique_ptr<Input> InputFactory::create_input(const proto_Input &input, std::shared_ptr<Profile>)
{
    EXPECT_EQ(input.which_input, proto_Input_gpio_tag);
    return pin(input.input.gpio.pin);
}

namespace
{
using Bytes = std::vector<uint8_t>;

void varint(Bytes &bytes, uint32_t value)
{
    do
    {
        bytes.push_back((value & 0x7f) | (value > 0x7f ? 0x80 : 0));
        value >>= 7;
    } while (value);
}

void number(Bytes &bytes, uint32_t tag, uint32_t value)
{
    varint(bytes, tag << 3);
    varint(bytes, value);
}

void message(Bytes &bytes, uint32_t tag, const Bytes &payload)
{
    varint(bytes, (tag << 3) | 2);
    varint(bytes, payload.size());
    bytes.insert(bytes.end(), payload.begin(), payload.end());
}

Bytes wrapped(uint32_t tag, const Bytes &payload)
{
    Bytes bytes;
    message(bytes, tag, payload);
    return bytes;
}

Bytes gpio(uint8_t pin)
{
    proto_Input input = proto_Input_init_default;
    input.which_input = proto_Input_gpio_tag;
    input.input.gpio.pin = pin;
    input.input.gpio.pinMode = PullUp;
    uint8_t buffer[128];
    auto stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    EXPECT_TRUE(pb_encode(&stream, proto_Input_fields, &input));
    return Bytes(buffer, buffer + stream.bytes_written);
}

Bytes shortcut(std::initializer_list<Bytes> inputs)
{
    Bytes payload;
    for (const auto &input : inputs)
    {
        message(payload, proto_ShortcutInput_inputs_tag, input);
    }
    return wrapped(proto_Input_shortcut_tag, payload);
}

Bytes held(const Bytes &input)
{
    Bytes payload;
    message(payload, proto_HeldInput_input_tag, input);
    number(payload, proto_HeldInput_time_tag, 10);
    return wrapped(proto_Input_held_tag, payload);
}

Bytes cycle(const Bytes &forward, const Bytes &reverse, bool reverse_first = false)
{
    Bytes payload;
    number(payload, proto_CycleInput_deviceid_tag, 5);
    if (reverse_first)
    {
        message(payload, proto_CycleInput_inputReverse_tag, reverse);
        message(payload, proto_CycleInput_input_tag, forward);
    }
    else
    {
        message(payload, proto_CycleInput_input_tag, forward);
        message(payload, proto_CycleInput_inputReverse_tag, reverse);
    }
    return wrapped(proto_Input_cycle_tag, payload);
}

Bytes toggle(const Bytes &input)
{
    Bytes payload;
    number(payload, proto_ToggleInput_deviceid_tag, 6);
    message(payload, proto_ToggleInput_input_tag, input);
    return wrapped(proto_Input_toggle_tag, payload);
}

class InputDecoderTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();
    proto_CycleDevice cycle_config = proto_CycleDevice_init_default;
    proto_ToggleDevice toggle_config = proto_ToggleDevice_init_default;
    std::shared_ptr<CycleDevice> cycling = std::make_shared<CycleDevice>(
        nullptr, cycle_config, 5, 0, std::vector<uint32_t>{10, 20, 30});
    std::shared_ptr<ToggleDevice> toggling = std::make_shared<ToggleDevice>(
        nullptr, toggle_config, 6, false);

    void SetUp() override
    {
        InputsTest::SetUp();
        profile->devices[5] = cycling;
        profile->devices[6] = toggling;
    }

    std::unique_ptr<Input> decode(const Bytes &bytes)
    {
        InputDecoder decoder{profile};
        proto_Input input = proto_Input_init_default;
        input.cb_input.funcs.decode = InputDecoder::prepare;
        input.cb_input.arg = &decoder;
        auto stream = pb_istream_from_buffer(bytes.data(), bytes.size());
        bool ok = pb_decode_ex(&stream, proto_Input_fields, &input, PB_DECODE_NOINIT);
        EXPECT_TRUE(ok) << PB_GET_ERROR(&stream);
        if (!ok)
        {
            return nullptr;
        }
        EXPECT_EQ(stream.bytes_left, 0u);
        return decoder.take(input);
    }
};
}

TEST_F(InputDecoderTest, HeldLeafOwnsItsChildRatherThanItself)
{
    auto input = decode(held(gpio(2)));
    ASSERT_NE(input, nullptr);
    auto *held_input = dynamic_cast<HeldInput *>(input.get());
    ASSERT_NE(held_input, nullptr);
    EXPECT_NE(held_input->get_inner_input(), held_input);
    EXPECT_EQ(input->hardware_id(), pin_id(2));
    EXPECT_TRUE(input->valid());
    input->setup();
    fake_pins::state[2] = true;
    EXPECT_FALSE(input->tick_digital());
    clock_ms::advance(11);
    EXPECT_TRUE(input->tick_digital());
}

TEST_F(InputDecoderTest, ShortcutKeepsNestedModifiersAndFollowingMembers)
{
    auto input = decode(shortcut({held(gpio(2)), shortcut({gpio(3), gpio(4)}), gpio(7)}));
    ASSERT_NE(input, nullptr);
    auto *chord = input->as_shortcut();
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->inputs.size(), 3u);
    EXPECT_NE(dynamic_cast<HeldInput *>(chord->inputs[0].get()), nullptr);
    ASSERT_NE(chord->inputs[1]->as_shortcut(), nullptr);
    EXPECT_EQ(chord->inputs[1]->as_shortcut()->inputs.size(), 2u);
    EXPECT_EQ(chord->inputs[2]->hardware_id(), pin_id(7));
    input->setup();
    fake_pins::state[2] = fake_pins::state[3] = fake_pins::state[4] = true;
    EXPECT_FALSE(input->tick_digital());
    clock_ms::advance(11);
    EXPECT_FALSE(input->tick_digital());
    fake_pins::state[7] = true;
    EXPECT_TRUE(input->tick_digital());
}

TEST_F(InputDecoderTest, CyclingShortcutsKeepBothDirectionsInEitherWireOrder)
{
    for (bool reverse_first : {false, true})
    {
        auto input = decode(cycle(shortcut({gpio(2), gpio(3)}),
                                  shortcut({gpio(4), gpio(7)}), reverse_first));
        ASSERT_NE(input, nullptr);
        EXPECT_NE(input->as_shortcut(), input.get());
        EXPECT_TRUE(input->valid());
        input->setup();
        EXPECT_EQ(input->tick_analog(), 10);
        fake_pins::state[2] = true;
        EXPECT_EQ(input->tick_analog(), 10);
        fake_pins::state[3] = true;
        EXPECT_EQ(input->tick_analog(), 20);
        clock_ms::advance(150);
        EXPECT_EQ(input->tick_analog(), 20);
        fake_pins::state[2] = fake_pins::state[3] = false;
        input->tick_analog();
        fake_pins::state[4] = fake_pins::state[7] = true;
        EXPECT_EQ(input->tick_analog(), 10);
        fake_pins::reset();
        input->tick_analog();
        clock_ms::advance(150);
    }
}

TEST_F(InputDecoderTest, ShortcutCanContainACycleAndAToggle)
{
    auto input = decode(shortcut({cycle(gpio(2), gpio(3)), toggle(shortcut({gpio(4), gpio(7)})), gpio(8)}));
    ASSERT_NE(input, nullptr);
    ASSERT_NE(input->as_shortcut(), nullptr);
    ASSERT_EQ(input->as_shortcut()->inputs.size(), 3u);
    input->setup();
    fake_pins::state[2] = fake_pins::state[4] = fake_pins::state[7] = fake_pins::state[8] = true;
    EXPECT_TRUE(input->tick_digital());
    EXPECT_EQ(cycling->get_value(), 20u);
    EXPECT_TRUE(toggling->get_value());
}

TEST_F(InputDecoderTest, ShiftedInputKeepsSeparateNestedValueAndShift)
{
    for (bool shift_first : {false, true})
    {
        Bytes payload;
        if (shift_first)
        {
            message(payload, proto_ShiftedInput_shift_tag, shortcut({gpio(3), gpio(4)}));
            message(payload, proto_ShiftedInput_input_tag, held(gpio(2)));
        }
        else
        {
            message(payload, proto_ShiftedInput_input_tag, held(gpio(2)));
            message(payload, proto_ShiftedInput_shift_tag, shortcut({gpio(3), gpio(4)}));
        }
        auto input = decode(wrapped(proto_Input_shifted_tag, payload));
        ASSERT_NE(input, nullptr);
        auto *shifted = dynamic_cast<ShiftedInput *>(input.get());
        ASSERT_NE(shifted, nullptr);
        EXPECT_NE(dynamic_cast<HeldInput *>(shifted->get_inner_input()), nullptr);
        ASSERT_NE(shifted->get_shift_input()->as_shortcut(), nullptr);
        EXPECT_TRUE(input->valid());
        input->setup();
        fake_pins::state[2] = true;
        EXPECT_FALSE(input->tick_digital());
        clock_ms::advance(11);
        EXPECT_FALSE(input->tick_digital());
        fake_pins::state[3] = fake_pins::state[4] = true;
        EXPECT_TRUE(input->tick_digital());
        fake_pins::reset();
    }
}

TEST_F(InputDecoderTest, UnclaimedDevicesLeaveInertInputsAndReleaseNestedChildren)
{
    profile->devices.clear();
    auto input = decode(cycle(shortcut({gpio(2), gpio(3)}), held(gpio(4))));
    ASSERT_NE(input, nullptr);
    EXPECT_FALSE(input->valid());
    input->setup();
    EXPECT_EQ(input->hardware_id(), 0u);
    EXPECT_EQ(input->tick_analog(), 0);
    input = decode(toggle(shortcut({gpio(2), gpio(3)})));
    ASSERT_NE(input, nullptr);
    EXPECT_FALSE(input->valid());
    input->setup();
    EXPECT_FALSE(input->tick_digital());
}

TEST_F(InputDecoderTest, FailedNestedDecodeReleasesAlreadyDecodedChildren)
{
    Bytes invalid_held;
    message(invalid_held, proto_HeldInput_input_tag, shortcut({gpio(2), gpio(3)}));
    // The required hold time is missing, after all the nested children have been allocated.
    auto bytes = shortcut({gpio(4), wrapped(proto_Input_held_tag, invalid_held)});
    InputDecoder decoder{profile};
    proto_Input input = proto_Input_init_default;
    input.cb_input.funcs.decode = InputDecoder::prepare;
    input.cb_input.arg = &decoder;
    auto stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    EXPECT_FALSE(pb_decode_ex(&stream, proto_Input_fields, &input, PB_DECODE_NOINIT));
    EXPECT_STREQ(PB_GET_ERROR(&stream), "missing required field");
    EXPECT_EQ(decoder.special, nullptr);
}
