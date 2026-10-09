// Reading an extension plugged into the Santroller: WiiExtensionDecoder
// (lib/wii_extensions/wii_extension_decoder.cpp), against wiibrew:
//   Wiimote/Extension_Controllers                      identification bytes at 0xFA
//   .../Nunchuck                                       6 byte report
//   .../Classic_Controller (and Classic_Controller_Pro) data formats 1, 2 and 3
//   .../Guitar_Hero_(Wii)_Guitars                      6 byte report, touch bar codes
//   .../Guitar_Hero_World_Tour_(Wii)_Drums             6 byte report, inverted MIDI data
//   .../DJ_Hero_(Wii)_Turntable                        6 byte report
//   .../TaTaCon                                        1 byte report at 0x05
//   .../uDraw_GameTablet                               6 byte report
// All buttons are active low. Reports are built from the bit tables on those pages, not from the
// structs in include/protocols/wii.hpp, so a mistake shared with the emulation side still shows up.
#include <gtest/gtest.h>
#include <stdint.h>
#include <string.h>
#include <initializer_list>
#include <utility>
#include <vector>
#include "fake_bus.hpp"
#include "wii_extension_decoder.hpp"
#include "devices/midi.hpp"

namespace
{
WiiExtensionDecoder decoder_for(std::initializer_list<uint8_t> id)
{
    WiiExtensionDecoder decoder;
    std::vector<uint8_t> bytes(id);
    decoder.decode_id(bytes.data());
    return decoder;
}

void feed(WiiExtensionDecoder &decoder, std::vector<uint8_t> report, MidiDevice *midi = nullptr)
{
    decoder.update_data(report.data(), (uint8_t)report.size(), midi);
}

// A button and where wiibrew puts it: byte, bit
struct ButtonBit
{
    proto_WiiButtonType button;
    const char *name;
    uint8_t byte;
    uint8_t bit;
};

// Clears one button's bit at a time in an otherwise idle report and checks only that button reads
void expect_buttons(WiiExtensionDecoder &decoder, const std::vector<uint8_t> &idle,
                    const std::vector<ButtonBit> &buttons)
{
    feed(decoder, idle);
    for (const auto &b : buttons)
    {
        EXPECT_FALSE(decoder.read_button(b.button)) << b.name << " with nothing pressed";
    }
    for (const auto &pressed : buttons)
    {
        std::vector<uint8_t> report = idle;
        report[pressed.byte] &= ~(1 << pressed.bit);
        feed(decoder, report);
        for (const auto &b : buttons)
        {
            EXPECT_EQ(decoder.read_button(b.button), b.button == pressed.button)
                << b.name << " while pressing " << pressed.name;
        }
    }
}

proto_Output gamepad_button(GamepadButtonType button)
{
    proto_Output out;
    memset(&out, 0, sizeof(out));
    out.which_mapping = proto_Output_gamepadButton_tag;
    out.mapping.gamepadButton = button;
    return out;
}
proto_Output gamepad_axis(GamepadAxisType axis)
{
    proto_Output out;
    memset(&out, 0, sizeof(out));
    out.which_mapping = proto_Output_gamepadAxis_tag;
    out.mapping.gamepadAxis = axis;
    return out;
}

// Somewhere for the decoder to send drum MIDI; the fake process_midi_data never touches it
alignas(MidiDevice) uint8_t midi_storage[sizeof(MidiDevice)];
MidiDevice *fake_midi_device() { return reinterpret_cast<MidiDevice *>(midi_storage); }
} // namespace

// Identification

namespace
{
struct IdCase
{
    std::vector<uint8_t> id;
    WiiExtType type;
    SubType subtype;
    const char *name;
};
} // namespace

TEST(WiiExtensionId, EveryDocumentedIdDecodesToItsType)
{
    const IdCase cases[] = {
        {{0x00, 0x00, 0xA4, 0x20, 0x00, 0x00}, WiiNunchuk, SubType_Gamepad, "Nunchuk"},
        {{0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}, WiiClassicController, SubType_Gamepad, "Classic Controller"},
        {{0x00, 0x00, 0xA4, 0x20, 0x03, 0x01}, WiiClassicController, SubType_Gamepad, "Classic Controller, format 3"},
        {{0x01, 0x00, 0xA4, 0x20, 0x01, 0x01}, WiiClassicControllerPro, SubType_Gamepad, "Classic Controller Pro"},
        {{0xFF, 0x00, 0xA4, 0x20, 0x00, 0x13}, WiiUbisoftDrawsomeTablet, SubType_Gamepad, "Drawsome tablet"},
        {{0x00, 0x00, 0xA4, 0x20, 0x01, 0x03}, WiiGuitarHeroGuitar, SubType_GuitarHeroGuitar, "Guitar Hero guitar"},
        {{0x01, 0x00, 0xA4, 0x20, 0x01, 0x03}, WiiGuitarHeroDrums, SubType_GuitarHeroDrums, "GHWT drums"},
        {{0x03, 0x00, 0xA4, 0x20, 0x01, 0x03}, WiiDjHeroTurntable, SubType_DjHeroTurntable, "DJ Hero turntable"},
        {{0x00, 0x00, 0xA4, 0x20, 0x01, 0x11}, WiiTaikoNoTatsujinController, SubType_Taiko, "TaTaCon"},
        {{0xFF, 0x00, 0xA4, 0x20, 0x01, 0x12}, WiiThqUdrawTablet, SubType_Gamepad, "uDraw GameTablet"},
        {{0x00, 0x00, 0xA4, 0x20, 0x04, 0x05}, WiiMotionPlus, SubType_Gamepad, "active Wii Motion Plus"},
    };
    for (const auto &c : cases)
    {
        WiiExtensionDecoder decoder;
        decoder.decode_id(c.id.data());
        EXPECT_EQ(decoder.mType, c.type) << c.name;
        EXPECT_EQ(decoder.get_subtype(), c.subtype) << c.name;
    }
}

TEST(WiiExtensionId, DecodingAnIdClearsPerExtensionState)
{
    WiiExtensionDecoder decoder;
    decoder.hiRes = true;
    decoder.hasTapBar = true;
    decoder.s_box = 0x97;
    const uint8_t id[] = {0x00, 0x00, 0xA4, 0x20, 0x01, 0x03};
    decoder.decode_id(id);
    EXPECT_FALSE(decoder.hiRes);
    EXPECT_FALSE(decoder.hasTapBar);
    EXPECT_EQ(decoder.s_box, 0);
}

TEST(WiiExtensionId, ResetForgetsTheExtension)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    feed(decoder, {1, 2, 3, 4, 5, 6});
    decoder.reset();
    EXPECT_EQ(decoder.mType, WiiNoExtension);
    EXPECT_EQ(decoder.mBuffer[0], 0);
}

// Old style (zero key) encryption, which the reader undoes with a single table value

TEST(WiiExtensionEncryption, ZeroKeyDecryptionWithTheFirstPartyTable)
{
    // wiibrew: with an all zero key both tables are 0x97; the encrypted Classic Controller
    // type bytes read FD FD and the Nunchuk FE FE
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    decoder.s_box = 0x97;
    feed(decoder, {0xFD, 0xFD, 0xFE, 0xFE, 0xFB, 0xEB});
    EXPECT_EQ(decoder.mBuffer[0], 0x01);
    EXPECT_EQ(decoder.mBuffer[1], 0x01);
    EXPECT_EQ(decoder.mBuffer[2], 0x00);
    EXPECT_EQ(decoder.mBuffer[3], 0x00);
    EXPECT_EQ(decoder.mBuffer[4], 0x03);
    EXPECT_EQ(decoder.mBuffer[5], 0x13);
}

TEST(WiiExtensionEncryption, NoTableLeavesDataAlone)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    feed(decoder, {0xFD, 0xFD, 0xFE, 0xFE, 0xFB, 0xEB});
    EXPECT_EQ(decoder.mBuffer[0], 0xFD);
    EXPECT_EQ(decoder.mBuffer[5], 0xEB);
}

TEST(WiiExtensionEncryption, LongReportsOnlyFillTheBuffer)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x03, 0x01});
    std::vector<uint8_t> report(21, 0x42);
    feed(decoder, report);
    for (uint8_t byte : decoder.mBuffer)
    {
        EXPECT_EQ(byte, 0x42);
    }
}

// Nunchuk: SX, SY, AX<9:2>, AY<9:2>, AZ<9:2>, then AZ<1:0> AY<1:0> AX<1:0> C Z

TEST(WiiNunchuk, SticksAreEightBit)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00});
    feed(decoder, {0x23, 0xDC, 0x80, 0x80, 0x80, 0xFF});
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukStickX), 0x23 << 8);
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukStickY), 0xDC << 8);
    proto_Output lx = gamepad_axis(Gamepad_LeftStickX), ly = gamepad_axis(Gamepad_LeftStickY);
    EXPECT_EQ(decoder.tick_analog(lx), 0x23 << 8);
    EXPECT_EQ(decoder.tick_analog(ly), 0xDC << 8);
}

TEST(WiiNunchuk, ButtonsAreActiveLowInTheBottomBits)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00});
    expect_buttons(decoder, {0x80, 0x80, 0x80, 0x80, 0x80, 0xFF},
                   {{WiiButtonNunchukC, "C", 5, 1}, {WiiButtonNunchukZ, "Z", 5, 0}});
}

TEST(WiiNunchuk, ButtonsMapToTheGamepad)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00});
    feed(decoder, {0x80, 0x80, 0x80, 0x80, 0x80, 0xFC});
    proto_Output a = gamepad_button(Gamepad_A), b = gamepad_button(Gamepad_B), lt = gamepad_axis(Gamepad_LeftTrigger);
    EXPECT_TRUE(decoder.tick_digital(a));
    EXPECT_TRUE(decoder.tick_digital(b));
    EXPECT_EQ(decoder.tick_analog(lt), 65535);
    feed(decoder, {0x80, 0x80, 0x80, 0x80, 0x80, 0xFF});
    EXPECT_FALSE(decoder.tick_digital(a));
    EXPECT_EQ(decoder.tick_analog(lt), 0);
}

TEST(WiiNunchuk, AccelerationHighBitsScaleTo16Bits)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00});
    // the low bits all zero, so this doesn't depend on where they are
    feed(decoder, {0x80, 0x80, 0x40, 0x80, 0xC0, 0x03});
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationX), (0x40 << 2) << 6);
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationY), (0x80 << 2) << 6);
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationZ), (0xC0 << 2) << 6);
}

TEST(WiiNunchuk, AccelerationYLowBitsAreBits5And4)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00});
    feed(decoder, {0x80, 0x80, 0x80, 0x80, 0x80, 0b00110011});
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationY), ((0x80 << 2) | 3) << 6);
}

// wiibrew's Nunchuk table has byte 5 = AZ<1:0> AY<1:0> AX<1:0> BC BZ, so AX's low bits are bits 3-2
// and AZ's are bits 7-6 (WiiExtensionDecoder::read_axis in lib/wii_extensions/wii_extension_decoder.cpp,
// pitch and roll use the same values).
TEST(WiiNunchuk, AccelerationXAndZLowBitsComeFromTheRightPlaces)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00});
    // AX<1:0> = 3, AZ<1:0> = 0
    feed(decoder, {0x80, 0x80, 0x80, 0x80, 0x80, 0b00001111});
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationX), ((0x80 << 2) | 3) << 6);
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationZ), (0x80 << 2) << 6);
    // AX<1:0> = 0, AZ<1:0> = 2
    feed(decoder, {0x80, 0x80, 0x80, 0x80, 0x80, 0b10000011});
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationX), (0x80 << 2) << 6);
    EXPECT_EQ(decoder.read_axis(WiiAxisNunchukAccelerationZ), ((0x80 << 2) | 2) << 6);
}

TEST(WiiNunchuk, LevelNunchukHasCentredPitchAndRoll)
{
    // Lying flat: X and Y at the 512 zero point, Z at +1g (wiibrew: around 760 right side up)
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00});
    // X = Y = 511 (0x7F << 2 | 3), Z = 760 (0xBE << 2); low bits for all three set alike so the
    // test doesn't depend on their order
    feed(decoder, {0x80, 0x80, 0x7F, 0x7F, 0xBE, 0b00111111});
    // atan2(0, positive) is 0, which reads as the middle of the range
    int pitch = decoder.read_axis(WiiAxisNunchukRotationPitch);
    int roll = decoder.read_axis(WiiAxisNunchukRotationRoll);
    EXPECT_NEAR(pitch, 32767, 200);
    EXPECT_NEAR(roll, 32767, 200);
}

// Classic Controller, data format 1:
//   0: RX<4:3> LX<5:0>   1: RX<2:1> LY<5:0>   2: RX<0> LT<4:3> RY<4:0>   3: LT<2:0> RT<4:0>
//   4: BDR BDD BLT B- BH B+ BRT 1            5: BZL BB BY BA BX BZR BDL BDU

namespace
{
std::vector<uint8_t> classic_format1(uint8_t lx, uint8_t ly, uint8_t rx, uint8_t ry, uint8_t lt, uint8_t rt)
{
    return {
        (uint8_t)(((rx >> 3) & 3) << 6 | (lx & 0x3F)),
        (uint8_t)(((rx >> 1) & 3) << 6 | (ly & 0x3F)),
        (uint8_t)((rx & 1) << 7 | ((lt >> 3) & 3) << 5 | (ry & 0x1F)),
        (uint8_t)((lt & 7) << 5 | (rt & 0x1F)),
        0xFF,
        0xFF,
    };
}

const std::vector<ButtonBit> classic_buttons_low_high = {
    {WiiButtonClassicRt, "RT click", 0, 1},  {WiiButtonClassicPlus, "+", 0, 2},
    {WiiButtonClassicHome, "Home", 0, 3},    {WiiButtonClassicMinus, "-", 0, 4},
    {WiiButtonClassicLt, "LT click", 0, 5},  {WiiButtonClassicDPadDown, "Down", 0, 6},
    {WiiButtonClassicDPadRight, "Right", 0, 7}, {WiiButtonClassicDPadUp, "Up", 1, 0},
    {WiiButtonClassicDPadLeft, "Left", 1, 1}, {WiiButtonClassicZr, "ZR", 1, 2},
    {WiiButtonClassicX, "X", 1, 3},          {WiiButtonClassicA, "A", 1, 4},
    {WiiButtonClassicY, "Y", 1, 5},          {WiiButtonClassicB, "B", 1, 6},
    {WiiButtonClassicZl, "ZL", 1, 7},
};

std::vector<ButtonBit> classic_buttons_at(uint8_t first)
{
    std::vector<ButtonBit> result = classic_buttons_low_high;
    for (auto &b : result)
    {
        b.byte += first;
    }
    return result;
}
} // namespace

TEST(WiiClassicFormat1, LeftStickIsSixBit)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    feed(decoder, classic_format1(63, 0, 0, 0, 0, 0));
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickX), 63 << 10);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickY), 0);
    feed(decoder, classic_format1(0, 33, 0, 0, 0, 0));
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickX), 0);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickY), 33 << 10);
}

TEST(WiiClassicFormat1, RightStickXIsSplitOverThreeBytes)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    for (uint8_t rx : {1, 2, 4, 8, 16, 21, 31})
    {
        feed(decoder, classic_format1(0, 0, rx, 0, 0, 0));
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickX), rx << 11) << int(rx);
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickX), 0) << int(rx);
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickY), 0) << int(rx);
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickY), 0) << int(rx);
    }
}

TEST(WiiClassicFormat1, RightStickYAndTriggersAreFiveBit)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    for (uint8_t value : {1, 3, 8, 16, 31})
    {
        feed(decoder, classic_format1(0, 0, 0, value, 0, 0));
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickY), value << 11) << int(value);
        feed(decoder, classic_format1(0, 0, 0, 0, value, 0));
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftTrigger), value << 11) << int(value);
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickY), 0) << int(value);
        feed(decoder, classic_format1(0, 0, 0, 0, 0, value));
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightTrigger), value << 11) << int(value);
        EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftTrigger), 0) << int(value);
    }
}

TEST(WiiClassicFormat1, CentredSticksReadNearTheMiddle)
{
    // 6 bit sticks centre on 32, 5 bit on 16
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    feed(decoder, classic_format1(32, 32, 16, 16, 0, 0));
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickX), 0x8000);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickY), 0x8000);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickX), 0x8000);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickY), 0x8000);
}

TEST(WiiClassicFormat1, ButtonsAreActiveLow)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    expect_buttons(decoder, classic_format1(32, 32, 16, 16, 0, 0), classic_buttons_at(4));
}

TEST(WiiClassicFormat1, ProControllerUsesTheSameLayout)
{
    auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x01, 0x01});
    expect_buttons(decoder, classic_format1(32, 32, 16, 16, 0, 0), classic_buttons_at(4));
    feed(decoder, classic_format1(10, 20, 30, 5, 7, 9));
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickX), 10 << 10);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickY), 20 << 10);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickX), 30 << 11);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickY), 5 << 11);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftTrigger), 7 << 11);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightTrigger), 9 << 11);
}

TEST(WiiClassicFormat1, TriggerPressureIsTheAnalogValue)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    feed(decoder, classic_format1(32, 32, 16, 16, 12, 25));
    EXPECT_EQ(decoder.read_button_pressure(WiiButtonClassicLt), 12 << 11);
    EXPECT_EQ(decoder.read_button_pressure(WiiButtonClassicRt), 25 << 11);
    // other buttons are all or nothing
    auto report = classic_format1(32, 32, 16, 16, 0, 0);
    report[5] &= ~(1 << 4);
    feed(decoder, report);
    EXPECT_EQ(decoder.read_button_pressure(WiiButtonClassicA), UINT16_MAX);
    EXPECT_EQ(decoder.read_button_pressure(WiiButtonClassicB), 0);
}

TEST(WiiClassicFormat1, GamepadMapping)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    const std::pair<GamepadButtonType, ButtonBit> mapping[] = {
        {Gamepad_A, {WiiButtonClassicA, "A", 5, 4}},
        {Gamepad_B, {WiiButtonClassicB, "B", 5, 6}},
        {Gamepad_X, {WiiButtonClassicX, "X", 5, 3}},
        {Gamepad_Y, {WiiButtonClassicY, "Y", 5, 5}},
        {Gamepad_DpadUp, {WiiButtonClassicDPadUp, "Up", 5, 0}},
        {Gamepad_DpadDown, {WiiButtonClassicDPadDown, "Down", 4, 6}},
        {Gamepad_DpadLeft, {WiiButtonClassicDPadLeft, "Left", 5, 1}},
        {Gamepad_DpadRight, {WiiButtonClassicDPadRight, "Right", 4, 7}},
        {Gamepad_LeftShoulder, {WiiButtonClassicZl, "ZL", 5, 7}},
        {Gamepad_RightShoulder, {WiiButtonClassicZr, "ZR", 5, 2}},
        {Gamepad_Start, {WiiButtonClassicPlus, "+", 4, 2}},
        {Gamepad_Back, {WiiButtonClassicMinus, "-", 4, 4}},
        {Gamepad_Guide, {WiiButtonClassicHome, "Home", 4, 3}},
    };
    for (const auto &[gamepad, bit] : mapping)
    {
        auto report = classic_format1(32, 32, 16, 16, 0, 0);
        report[bit.byte] &= ~(1 << bit.bit);
        feed(decoder, report);
        proto_Output out = gamepad_button(gamepad);
        EXPECT_TRUE(decoder.tick_digital(out)) << bit.name;
        feed(decoder, classic_format1(32, 32, 16, 16, 0, 0));
        EXPECT_FALSE(decoder.tick_digital(out)) << bit.name;
    }
    // trigger clicks drive the digital trigger, the analog value the axis
    auto report = classic_format1(32, 32, 16, 16, 31, 0);
    report[4] &= ~(1 << 5);
    feed(decoder, report);
    proto_Output lt = gamepad_axis(Gamepad_LeftTrigger), rt = gamepad_axis(Gamepad_RightTrigger);
    EXPECT_TRUE(decoder.tick_digital(lt));
    EXPECT_FALSE(decoder.tick_digital(rt));
    EXPECT_EQ(decoder.tick_analog(lt), 31 << 11);
}

// Classic Controller, data format 3 (the reader's "hi res" mode):
//   LX, RX, LY, RY, LT, RT, then the same two button bytes

TEST(WiiClassicFormat3, SticksAndTriggersAreEightBit)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x03, 0x01});
    decoder.hiRes = true;
    feed(decoder, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xFF, 0xFF});
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickX), 0x11 << 8);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickX), 0x22 << 8);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickY), 0x33 << 8);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightStickY), 0x44 << 8);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftTrigger), 0x55 << 8);
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicRightTrigger), 0x66 << 8);
}

TEST(WiiClassicFormat3, ButtonsAreInBytesSixAndSeven)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x03, 0x01});
    decoder.hiRes = true;
    expect_buttons(decoder, {0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0xFF, 0xFF}, classic_buttons_at(6));
}

TEST(WiiClassicFormat3, ProControllerToo)
{
    auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x03, 0x01});
    decoder.hiRes = true;
    expect_buttons(decoder, {0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0xFF, 0xFF}, classic_buttons_at(6));
}

// Guitar Hero guitar:
//   0: GH3 GH3 SX<5:0>   1: GH3 GH3 SY<5:0>   2: 0 0 0 TB<4:0>   3: 0 0 0 WB<4:0>
//   4: 1 BD 1 B- 1 B+ 1 1                     5: BO BR BB BG BY PB 1 BU

namespace
{
std::vector<uint8_t> guitar(uint8_t sx, uint8_t sy, uint8_t tb, uint8_t wb)
{
    return {(uint8_t)(0xC0 | sx), (uint8_t)(0xC0 | sy), tb, wb, 0xFF, 0xFF};
}
const std::vector<ButtonBit> guitar_buttons = {
    {WiiButtonGuitarStrumDown, "strum down", 4, 6}, {WiiButtonGuitarMinus, "-", 4, 4},
    {WiiButtonGuitarPlus, "+", 4, 2},               {WiiButtonGuitarOrange, "orange", 5, 7},
    {WiiButtonGuitarRed, "red", 5, 6},              {WiiButtonGuitarBlue, "blue", 5, 5},
    {WiiButtonGuitarGreen, "green", 5, 4},          {WiiButtonGuitarYellow, "yellow", 5, 3},
    {WiiButtonGuitarPedal, "pedal", 5, 2},          {WiiButtonGuitarStrumUp, "strum up", 5, 0},
};
const proto_WiiButtonType taps[] = {WiiButtonGuitarTapGreen, WiiButtonGuitarTapRed, WiiButtonGuitarTapYellow,
                                    WiiButtonGuitarTapBlue, WiiButtonGuitarTapOrange};

// Which touch bar frets the decoder reports, as a bit mask (green = 1 ... orange = 16)
uint8_t taps_pressed(const WiiExtensionDecoder &decoder)
{
    uint8_t mask = 0;
    for (int i = 0; i < 5; i++)
    {
        if (decoder.read_button(taps[i]))
        {
            mask |= 1 << i;
        }
    }
    return mask;
}
} // namespace

TEST(WiiGuitar, StickAndWhammy)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, guitar(0x20, 0x3F, 0x0F, 0x1F));
    EXPECT_EQ(decoder.read_axis(WiiAxisGuitarJoystickX), 0x20 << 10);
    EXPECT_EQ(decoder.read_axis(WiiAxisGuitarJoystickY), 0x3F << 10);
    EXPECT_EQ(decoder.read_axis(WiiAxisGuitarWhammy), 0x1F << 11);
    feed(decoder, guitar(0x00, 0x00, 0x0F, 0x10));
    EXPECT_EQ(decoder.read_axis(WiiAxisGuitarJoystickX), 0);
    EXPECT_EQ(decoder.read_axis(WiiAxisGuitarWhammy), 0x10 << 11);
    proto_Output rt = gamepad_axis(Gamepad_RightTrigger);
    EXPECT_EQ(decoder.tick_analog(rt), 0x10 << 11);
}

TEST(WiiGuitar, ButtonsAreActiveLow)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    expect_buttons(decoder, guitar(0x20, 0x20, 0x0F, 0x00), guitar_buttons);
}

TEST(WiiGuitar, GamepadMapping)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    const std::pair<GamepadButtonType, ButtonBit> mapping[] = {
        {Gamepad_A, {WiiButtonGuitarGreen, "green", 5, 4}},
        {Gamepad_B, {WiiButtonGuitarRed, "red", 5, 6}},
        {Gamepad_Y, {WiiButtonGuitarYellow, "yellow", 5, 3}},
        {Gamepad_X, {WiiButtonGuitarBlue, "blue", 5, 5}},
        {Gamepad_LeftShoulder, {WiiButtonGuitarOrange, "orange", 5, 7}},
        {Gamepad_DpadUp, {WiiButtonGuitarStrumUp, "strum up", 5, 0}},
        {Gamepad_DpadDown, {WiiButtonGuitarStrumDown, "strum down", 4, 6}},
        {Gamepad_Start, {WiiButtonGuitarPlus, "+", 4, 2}},
        {Gamepad_Back, {WiiButtonGuitarMinus, "-", 4, 4}},
    };
    for (const auto &[gamepad, bit] : mapping)
    {
        auto report = guitar(0x20, 0x20, 0x0F, 0);
        report[bit.byte] &= ~(1 << bit.bit);
        feed(decoder, report);
        proto_Output out = gamepad_button(gamepad);
        EXPECT_TRUE(decoder.tick_digital(out)) << bit.name;
        feed(decoder, guitar(0x20, 0x20, 0x0F, 0));
        EXPECT_FALSE(decoder.tick_digital(out)) << bit.name;
    }
}

TEST(WiiGuitar, TouchBarSingleFretCodes)
{
    // wiibrew: not touching 0F, 1st (top) fret 04, 2nd 0A, 3rd 12/13, 4th 17/18, 5th 1F
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, guitar(0x20, 0x20, 0x0F, 0));
    ASSERT_TRUE(decoder.hasTapBar);
    const std::pair<uint8_t, uint8_t> codes[] = {
        {0x0F, 0}, {0x04, 1}, {0x0A, 2}, {0x12, 4}, {0x13, 4}, {0x17, 8}, {0x18, 8}, {0x1F, 16},
    };
    for (const auto &[code, mask] : codes)
    {
        feed(decoder, guitar(0x20, 0x20, code, 0));
        EXPECT_EQ(taps_pressed(decoder), mask) << std::hex << int(code);
    }
}

// wiibrew lists values for two adjacent frets held together: 07 the 1st and 2nd, 0C/0D the 2nd and
// 3rd, 14/15 the 3rd and 4th, 1A the 4th and 5th
TEST(WiiGuitar, TouchBarChordCodesPressBothFrets)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, guitar(0x20, 0x20, 0x0F, 0));
    const std::pair<uint8_t, uint8_t> codes[] = {
        {0x07, 1 | 2}, {0x0C, 2 | 4}, {0x0D, 2 | 4}, {0x14, 4 | 8}, {0x15, 4 | 8}, {0x1A, 8 | 16},
    };
    for (const auto &[code, mask] : codes)
    {
        feed(decoder, guitar(0x20, 0x20, code, 0));
        EXPECT_EQ(taps_pressed(decoder), mask) << std::hex << int(code);
    }
}

TEST(WiiGuitar, NoTouchBarUntilTheGuitarReportsOneAtRest)
{
    // GH3 guitars have no touch bar; the decoder only trusts TB once it has seen the 0F rest value
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, guitar(0x20, 0x20, 0x04, 0));
    EXPECT_FALSE(decoder.hasTapBar);
    EXPECT_EQ(taps_pressed(decoder), 0);
    feed(decoder, guitar(0x20, 0x20, 0x0F, 0));
    EXPECT_TRUE(decoder.hasTapBar);
    feed(decoder, guitar(0x20, 0x20, 0x04, 0));
    EXPECT_EQ(taps_pressed(decoder), 1);
}

TEST(WiiGuitar, TouchBarAlsoPressesTheFretOutputs)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, guitar(0x20, 0x20, 0x0F, 0));
    feed(decoder, guitar(0x20, 0x20, 0x12, 0));
    proto_Output yellow;
    memset(&yellow, 0, sizeof(yellow));
    yellow.which_mapping = proto_Output_ghButton_tag;
    yellow.mapping.ghButton = GuitarHeroGuitar_Yellow;
    EXPECT_TRUE(decoder.tick_digital(yellow));
    yellow.mapping.ghButton = GuitarHeroGuitar_TapYellow;
    EXPECT_TRUE(decoder.tick_digital(yellow));
    yellow.mapping.ghButton = GuitarHeroGuitar_Green;
    EXPECT_FALSE(decoder.tick_digital(yellow));
}

// Guitar Hero World Tour drums, everything inverted:
//   0: 0 0 SX   1: 0 0 SY   2: note<6:0> vel<3>   3: vel<6:4> channel<3:0> vel<2>
//   4: vel<1> 1 1 B- 1 B+ 1 vel<0>                5: O R Y G B Bass 1 1

namespace
{
std::vector<uint8_t> drums(uint8_t note, uint8_t channel, uint8_t velocity, uint8_t buttons4 = 0x00)
{
    // every MIDI field is sent inverted
    uint8_t n = ~note & 0x7F, c = ~channel & 0x0F, v = ~velocity & 0x7F;
    return {
        0x20,
        0x20,
        (uint8_t)(n << 1 | ((v >> 3) & 1)),
        (uint8_t)(((v >> 4) & 7) << 5 | c << 1 | ((v >> 2) & 1)),
        (uint8_t)(((v >> 1) & 1) << 7 | 0x7E | (v & 1)),
        0xFF,
    };
}
} // namespace

TEST(WiiDrums, IdleReportSendsNoMidi)
{
    reset_wii_fakes();
    auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03});
    // wiibrew: bytes 2 and 3 are FF FF when there is no data
    feed(decoder, {0x20, 0x20, 0xFF, 0xFF, 0xFF, 0xFF}, fake_midi_device());
    EXPECT_TRUE(fake_midi::packets.empty());
}

TEST(WiiDrums, PadHitsBecomeNoteOnsOnChannel10)
{
    // wiibrew's pad table: channel 10, notes 36 kick, 38 red, 45 green, 46 yellow, 48 blue, 49 orange
    // with raw note values 5B 59 52 51 4F 4E
    const std::pair<uint8_t, uint8_t> pads[] = {{36, 0x5B}, {38, 0x59}, {45, 0x52}, {46, 0x51}, {48, 0x4F}, {49, 0x4E}};
    for (const auto &[note, raw] : pads)
    {
        reset_wii_fakes();
        auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03});
        auto report = drums(note, 9, 100);
        ASSERT_EQ(report[2] >> 1, raw) << int(note);
        feed(decoder, report, fake_midi_device());
        ASSERT_EQ(fake_midi::packets.size(), 1u) << int(note);
        const auto &packet = fake_midi::packets[0];
        ASSERT_EQ(packet.size(), 4u);
        EXPECT_EQ(packet[1], 0x99) << int(note); // note on, channel 10
        EXPECT_EQ(packet[2], note);
        EXPECT_EQ(packet[3], 100);
    }
}

TEST(WiiDrums, EveryVelocityBitIsReassembled)
{
    for (uint8_t velocity : {1, 2, 4, 8, 16, 32, 64, 127, 0x55, 0x2A})
    {
        reset_wii_fakes();
        auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03});
        feed(decoder, drums(38, 9, velocity), fake_midi_device());
        ASSERT_EQ(fake_midi::packets.size(), 1u) << int(velocity);
        EXPECT_EQ(fake_midi::packets[0][3], velocity);
        EXPECT_EQ(fake_midi::packets[0][2], 38);
    }
}

TEST(WiiDrums, ChannelComesFromByteThree)
{
    // the hi-hat pedal input and the MIDI in port can use other channels
    for (uint8_t channel : {0, 1, 5, 15})
    {
        reset_wii_fakes();
        auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03});
        feed(decoder, drums(100, channel, 64), fake_midi_device());
        ASSERT_EQ(fake_midi::packets.size(), 1u);
        EXPECT_EQ(fake_midi::packets[0][1], 0x90 | channel);
        EXPECT_EQ(fake_midi::packets[0][2], 100);
    }
}

TEST(WiiDrums, NoMidiWithoutADevice)
{
    reset_wii_fakes();
    auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, drums(38, 9, 100));
    EXPECT_TRUE(fake_midi::packets.empty());
}

TEST(WiiDrums, StickAndPlusMinus)
{
    auto decoder = decoder_for({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, {0x3F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF});
    EXPECT_EQ(decoder.read_axis(WiiAxisDrumJoystickX), 0x3F << 10);
    EXPECT_EQ(decoder.read_axis(WiiAxisDrumJoystickY), 0);
    expect_buttons(decoder, {0x20, 0x20, 0xFF, 0xFF, 0xFF, 0xFF},
                   {{WiiButtonDrumMinus, "-", 4, 4}, {WiiButtonDrumPlus, "+", 4, 2}});
}

// DJ Hero turntable:
//   0: RTT<4:3> SX<5:0>   1: RTT<2:1> SY<5:0>   2: RTT<0> ED<4:3> CS<3:0> RTT<5>
//   3: ED<2:0> LTT<4:0>   4: 0 0 LBR B- 0 B+ RBR LTT<5>   5: LBB 0 RBG BE LBG RBB 0 0
// Turntables are 6 bit signed (positive clockwise)

namespace
{
std::vector<uint8_t> turntable(uint8_t sx, uint8_t sy, int ltt, int rtt, uint8_t cs, uint8_t ed)
{
    uint8_t l = (uint8_t)ltt & 0x3F, r = (uint8_t)rtt & 0x3F;
    return {
        (uint8_t)(((r >> 3) & 3) << 6 | sx),
        (uint8_t)(((r >> 1) & 3) << 6 | sy),
        (uint8_t)((r & 1) << 7 | ((ed >> 3) & 3) << 5 | (cs & 0xF) << 1 | (r >> 5)),
        (uint8_t)((ed & 7) << 5 | (l & 0x1F)),
        (uint8_t)(0xFE | (l >> 5)),
        0xFF,
    };
}
} // namespace

TEST(WiiTurntable, VelocitiesAreSignedSixBitCentredAt32768)
{
    auto decoder = decoder_for({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03});
    const int velocities[] = {0, 1, -1, 31, -32, 17, -9};
    for (int v : velocities)
    {
        feed(decoder, turntable(0x20, 0x20, v, 0, 0, 0));
        EXPECT_EQ(decoder.read_axis(WiiAxisDjTurntableLeft), (32 + v) << 10) << "left " << v;
        EXPECT_EQ(decoder.read_axis(WiiAxisDjTurntableRight), 32 << 10) << "left " << v;
        feed(decoder, turntable(0x20, 0x20, 0, v, 0, 0));
        EXPECT_EQ(decoder.read_axis(WiiAxisDjTurntableRight), (32 + v) << 10) << "right " << v;
        EXPECT_EQ(decoder.read_axis(WiiAxisDjTurntableLeft), 32 << 10) << "right " << v;
    }
}

TEST(WiiTurntable, CrossfaderIsFourBitAndEffectsDialFiveBit)
{
    auto decoder = decoder_for({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03});
    for (uint8_t cs : {0, 1, 8, 15})
    {
        feed(decoder, turntable(0x20, 0x20, 0, 0, cs, 0));
        EXPECT_EQ(decoder.read_axis(WiiAxisDjCrossfadeSlider), cs << 12) << int(cs);
        EXPECT_EQ(decoder.read_axis(WiiAxisDjEffectDial), 0) << int(cs);
    }
    for (uint8_t ed : {1, 4, 8, 16, 31})
    {
        feed(decoder, turntable(0x20, 0x20, 0, 0, 0, ed));
        EXPECT_EQ(decoder.read_axis(WiiAxisDjEffectDial), ed << 11) << int(ed);
        EXPECT_EQ(decoder.read_axis(WiiAxisDjCrossfadeSlider), 0) << int(ed);
        EXPECT_EQ(decoder.read_axis(WiiAxisDjTurntableLeft), 32 << 10) << int(ed);
    }
}

TEST(WiiTurntable, Stick)
{
    auto decoder = decoder_for({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03});
    feed(decoder, turntable(0x3F, 0x01, -1, -1, 0, 0));
    EXPECT_EQ(decoder.read_axis(WiiAxisDjStickX), 0x3F << 10);
    EXPECT_EQ(decoder.read_axis(WiiAxisDjStickY), 0x01 << 10);
}

TEST(WiiTurntable, ButtonsAreActiveLow)
{
    auto decoder = decoder_for({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03});
    expect_buttons(decoder, turntable(0x20, 0x20, 0, 0, 0, 0),
                   {
                       {WiiButtonDjHeroLeftRed, "left red", 4, 5},
                       {WiiButtonDjHeroMinus, "-", 4, 4},
                       {WiiButtonDjHeroPlus, "+", 4, 2},
                       {WiiButtonDjHeroRightRed, "right red", 4, 1},
                       {WiiButtonDjHeroLeftBlue, "left blue", 5, 7},
                       {WiiButtonDjHeroRightGreen, "right green", 5, 5},
                       {WiiButtonDjHeroEuphoria, "euphoria", 5, 4},
                       {WiiButtonDjHeroLeftGreen, "left green", 5, 3},
                       {WiiButtonDjHeroRightBlue, "right blue", 5, 2},
                   });
}

// TaTaCon: the reader fetches the one byte at 0x05: 1 CL RL CR RR 1 1 1, hits clear their bit

TEST(WiiTaiko, DrumHitsAreActiveLow)
{
    auto decoder = decoder_for({0x00, 0x00, 0xA4, 0x20, 0x01, 0x11});
    expect_buttons(decoder, {0xFF},
                   {
                       {WiiButtonTaTaConLeftDrumCenter, "left centre", 0, 6},
                       {WiiButtonTaTaConLeftDrumRim, "left rim", 0, 5},
                       {WiiButtonTaTaConRightDrumCenter, "right centre", 0, 4},
                       {WiiButtonTaTaConRightDrumRim, "right rim", 0, 3},
                   });
}

// uDraw GameTablet:
//   0: X<7:0>  1: Y<7:0>  2: Y<11:8> X<11:8>  3: P<7:0>  4: FF  5: 1 1 1 1 1 P<8> BL BU

TEST(WiiUDraw, PenPositionIsTwelveBit)
{
    auto decoder = decoder_for({0xFF, 0x00, 0xA4, 0x20, 0x01, 0x12});
    feed(decoder, {0x34, 0x78, 0x56, 0x10, 0xFF, 0xFF});
    EXPECT_EQ(decoder.read_axis(WiiAxisUDrawPenX), 0x634);
    EXPECT_EQ(decoder.read_axis(WiiAxisUDrawPenY), 0x578);
    // 0xFFF when the pen is away from the surface
    feed(decoder, {0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0xFB});
    EXPECT_EQ(decoder.read_axis(WiiAxisUDrawPenX), 0xFFF);
    EXPECT_EQ(decoder.read_axis(WiiAxisUDrawPenY), 0xFFF);
}

TEST(WiiUDraw, PenPressureLowByte)
{
    auto decoder = decoder_for({0xFF, 0x00, 0xA4, 0x20, 0x01, 0x12});
    feed(decoder, {0x00, 0x00, 0x00, 0x42, 0xFF, 0xFB});
    EXPECT_EQ(decoder.read_axis(WiiAxisUDrawPenPressure), 0x42);
}

// wiibrew: uDraw pressure is a nine bit value (about 8 to 505) with P<8> in byte 5 bit 2, which
// read_axis (wii_extension_decoder.cpp) combines with P<7:0> from byte 3.
TEST(WiiUDraw, PenPressureIsNineBit)
{
    auto decoder = decoder_for({0xFF, 0x00, 0xA4, 0x20, 0x01, 0x12});
    feed(decoder, {0x00, 0x00, 0x00, 0xF9, 0xFF, 0xFF}); // P = 0x1F9 = 505
    EXPECT_EQ(decoder.read_axis(WiiAxisUDrawPenPressure), 0x1F9);
}

// wiibrew: uDraw byte 5 bit 1 is the lower stylus button and bit 0 the upper one, both active low
// (read_button in wii_extension_decoder.cpp, under WiiThqUdrawTablet).
TEST(WiiUDraw, StylusButtonsAreActiveLow)
{
    auto decoder = decoder_for({0xFF, 0x00, 0xA4, 0x20, 0x01, 0x12});
    feed(decoder, {0x00, 0x00, 0x00, 0x08, 0xFF, 0xFE}); // upper pressed
    bool upper_1 = decoder.read_button(WiiButtonUDrawPenButton1);
    bool upper_2 = decoder.read_button(WiiButtonUDrawPenButton2);
    EXPECT_NE(upper_1, upper_2);
    feed(decoder, {0x00, 0x00, 0x00, 0x08, 0xFF, 0xFD}); // lower pressed
    EXPECT_EQ(decoder.read_button(WiiButtonUDrawPenButton1), upper_2);
    EXPECT_EQ(decoder.read_button(WiiButtonUDrawPenButton2), upper_1);
}

TEST(WiiNoExtension, ReadsNothing)
{
    WiiExtensionDecoder decoder;
    feed(decoder, {0, 0, 0, 0, 0, 0});
    EXPECT_FALSE(decoder.read_button(WiiButtonClassicA));
    EXPECT_EQ(decoder.read_axis(WiiAxisClassicLeftStickX), 0);
    EXPECT_EQ(decoder.get_subtype(), SubType_Gamepad);
}
