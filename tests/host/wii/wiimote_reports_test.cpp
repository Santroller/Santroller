// The report packing in the Bluetooth Wii Remote emulation (lib/wii_remote_emulation/wm_reports.c),
// against wiibrew's Wiimote page:
//   Core Buttons: first byte Left 0x01 Right 0x02 Down 0x04 Up 0x08 Plus 0x10, second byte Two 0x01
//     One 0x02 B 0x04 A 0x08 Minus 0x10 Home 0x80, 1 when pressed
//   Normal accelerometer reporting: XX YY ZZ are bits 9-2, and the button bytes carry X<1:0> in
//     byte 0 bits 6-5, Y<1> in byte 1 bit 5 and Z<1> in byte 1 bit 6
//   Interleaved (0x3e / 0x3f): one byte of X (0x3e) or Y (0x3f), no LSBs; Z<7:0> is spread over the
//     button bytes: 0x3e byte 0 Z<5:4>, byte 1 Z<7:6>; 0x3f byte 0 Z<1:0>, byte 1 Z<3:2>
//   IR basic: per pair X1<7:0> Y1<7:0> [Y1<9:8> X1<9:8> Y2<9:8> X2<9:8>] X2<7:0> Y2<7:0>
//   IR extended: X<7:0> Y<7:0> [Y<9:8> X<9:8> S<3:0>]
//   IR full: extended, then X min, Y min, X max, Y max (7 bits each), 0, intensity
// The rest of the Wii Remote emulation (wiimote.c, wiimote_btstack.c) is tied to BTstack and isn't
// tested here.
#include <gtest/gtest.h>
#include <string.h>
#include <memory>
#include <vector>
extern "C" {
#include "wm_reports.h"
}
#include "wm_crypto.h"

namespace
{
std::unique_ptr<wiimote_state> new_state()
{
    auto state = std::make_unique<wiimote_state>();
    memset(state.get(), 0, sizeof(wiimote_state));
    return state;
}

std::vector<uint8_t> buttons_for(void (*press)(wiimote_state &))
{
    auto state = new_state();
    press(*state);
    uint8_t buf[2] = {};
    report_append_buttons(state.get(), buf);
    return {buf[0], buf[1]};
}
} // namespace

TEST(WiimoteReports, CoreButtonBits)
{
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.left = true; }), (std::vector<uint8_t>{0x01, 0x00}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.right = true; }), (std::vector<uint8_t>{0x02, 0x00}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.down = true; }), (std::vector<uint8_t>{0x04, 0x00}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.up = true; }), (std::vector<uint8_t>{0x08, 0x00}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.plus = true; }), (std::vector<uint8_t>{0x10, 0x00}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.two = true; }), (std::vector<uint8_t>{0x00, 0x01}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.one = true; }), (std::vector<uint8_t>{0x00, 0x02}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.b = true; }), (std::vector<uint8_t>{0x00, 0x04}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.a = true; }), (std::vector<uint8_t>{0x00, 0x08}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.minus = true; }), (std::vector<uint8_t>{0x00, 0x10}));
    EXPECT_EQ(buttons_for([](wiimote_state &s) { s.usr.home = true; }), (std::vector<uint8_t>{0x00, 0x80}));
    EXPECT_EQ(buttons_for([](wiimote_state &) {}), (std::vector<uint8_t>{0x00, 0x00}));
}

TEST(WiimoteReports, AccelerometerHighBitsAndLsbs)
{
    auto state = new_state();
    state->usr.accel_x = 0x2AB; // X<1:0> = 3
    state->usr.accel_y = 0x156; // Y<1> = 1
    state->usr.accel_z = 0x1FC; // Z<1> = 0
    uint8_t buf[5] = {};
    report_append_accelerometer(state.get(), buf);
    EXPECT_EQ(buf[0], 0x60);
    EXPECT_EQ(buf[1], 0x20);
    EXPECT_EQ(buf[2], 0xAA);
    EXPECT_EQ(buf[3], 0x55);
    EXPECT_EQ(buf[4], 0x7F);

    state->usr.accel_x = 0x200; // X<1:0> = 0
    state->usr.accel_y = 0x155; // Y<1> = 0
    state->usr.accel_z = 0x3FE; // Z<1> = 1
    memset(buf, 0, sizeof(buf));
    report_append_accelerometer(state.get(), buf);
    EXPECT_EQ(buf[0], 0x00);
    EXPECT_EQ(buf[1], 0x40);
    EXPECT_EQ(buf[2], 0x80);
    EXPECT_EQ(buf[3], 0x55);
    EXPECT_EQ(buf[4], 0xFF);
}

TEST(WiimoteReports, AccelerometerKeepsTheButtons)
{
    auto state = new_state();
    state->usr.home = true;
    state->usr.left = true;
    state->usr.accel_x = 0x3FF;
    state->usr.accel_y = 0x3FF;
    state->usr.accel_z = 0x3FF;
    uint8_t buf[5] = {};
    report_append_buttons(state.get(), buf);
    report_append_accelerometer(state.get(), buf);
    EXPECT_EQ(buf[0], 0x61);
    EXPECT_EQ(buf[1], 0xE0);
}

TEST(WiimoteReports, IrBasicPacksPairsIntoFiveBytes)
{
    auto state = new_state();
    const uint16_t xs[4] = {0x0A1, 0x399, 0x39E, 0x0A7};
    const uint16_t ys[4] = {0x2AA, 0x2AE, 0x078, 0x074};
    for (int i = 0; i < 4; i++)
    {
        state->usr.ir_object[i].x = xs[i];
        state->usr.ir_object[i].y = ys[i];
    }
    uint8_t buf[10] = {};
    report_append_ir_10(state.get(), buf);
    auto high = [](uint16_t y1, uint16_t x1, uint16_t y2, uint16_t x2)
    { return (uint8_t)((y1 >> 8) << 6 | (x1 >> 8) << 4 | (y2 >> 8) << 2 | (x2 >> 8)); };
    const uint8_t expected[10] = {
        0xA1, 0xAA, high(0x2AA, 0x0A1, 0x2AE, 0x399), 0x99, 0xAE,
        0x9E, 0x78, high(0x078, 0x39E, 0x074, 0x0A7), 0xA7, 0x74,
    };
    for (int i = 0; i < 10; i++)
    {
        EXPECT_EQ(buf[i], expected[i]) << i;
    }
}

TEST(WiimoteReports, IrExtendedIsThreeBytesPerObject)
{
    auto state = new_state();
    for (int i = 0; i < 4; i++)
    {
        state->usr.ir_object[i].x = (uint16_t)(0x100 * i + 0x23);
        state->usr.ir_object[i].y = (uint16_t)(0x2FF - 0x100 * (i % 3));
        state->usr.ir_object[i].size = (uint8_t)(3 + 4 * i);
    }
    uint8_t buf[12] = {};
    report_append_ir_12(state.get(), buf);
    for (int i = 0; i < 4; i++)
    {
        uint16_t x = state->usr.ir_object[i].x, y = state->usr.ir_object[i].y;
        EXPECT_EQ(buf[i * 3 + 0], x & 0xFF) << i;
        EXPECT_EQ(buf[i * 3 + 1], y & 0xFF) << i;
        EXPECT_EQ(buf[i * 3 + 2], (y >> 8) << 6 | (x >> 8) << 4 | state->usr.ir_object[i].size) << i;
    }
}

namespace
{
void fill_full_ir(wiimote_state &s)
{
    for (int i = 0; i < 4; i++)
    {
        auto &o = s.usr.ir_object[i];
        o.x = (uint16_t)(0x0E1 * (i + 1));
        o.y = (uint16_t)(0x0AB + 0x80 * i);
        o.size = (uint8_t)(i + 5);
        o.xmin = (uint8_t)(0x10 + i);
        o.ymin = (uint8_t)(0x20 + i);
        o.xmax = (uint8_t)(0x30 + i);
        o.ymax = (uint8_t)(0x40 + i);
        o.intensity = (uint8_t)(0xC0 + i);
    }
}

void expect_full_object(const uint8_t *obj, const wiimote_ir_object &o, int which)
{
    EXPECT_EQ(obj[0], o.x & 0xFF) << which;
    EXPECT_EQ(obj[1], o.y & 0xFF) << which;
    EXPECT_EQ(obj[2], (o.y >> 8) << 6 | (o.x >> 8) << 4 | o.size) << which;
    EXPECT_EQ(obj[3], o.xmin) << which << " x min";
    EXPECT_EQ(obj[4], o.ymin) << which << " y min";
    EXPECT_EQ(obj[5], o.xmax) << which << " x max";
    EXPECT_EQ(obj[6], o.ymax) << which << " y max";
    EXPECT_EQ(obj[7], 0) << which;
    EXPECT_EQ(obj[8], o.intensity) << which;
}
} // namespace

TEST(WiimoteReports, InterleavedAlternatesBetween3eAnd3f)
{
    auto state = new_state();
    state->sys.reporting_mode = 0x3e;
    uint8_t buf[21] = {};
    report_append_interleaved(state.get(), buf);
    EXPECT_EQ(state->sys.reporting_mode, 0x3f);
    report_append_interleaved(state.get(), buf);
    EXPECT_EQ(state->sys.reporting_mode, 0x3e);
}

TEST(WiimoteReports, InterleavedCarriesXThenY)
{
    auto state = new_state();
    state->usr.accel_x = 0x2AB;
    state->usr.accel_y = 0x157;
    state->sys.reporting_mode = 0x3e;
    uint8_t first[21] = {}, second[21] = {};
    report_append_interleaved(state.get(), first);
    report_append_interleaved(state.get(), second);
    EXPECT_EQ(first[2], 0xAA);
    EXPECT_EQ(second[2], 0x55);
}

// report_append_interleaved (lib/wii_remote_emulation/wm_reports.c) fills X min, Y min, X max and
// Y max into bytes 3-6 of each object, as in wiibrew's Full Mode table.
TEST(WiimoteReports, InterleavedFullIrObjects)
{
    auto state = new_state();
    fill_full_ir(*state);
    state->sys.reporting_mode = 0x3e;
    uint8_t first[21], second[21];
    memset(first, 0, sizeof(first));
    memset(second, 0, sizeof(second));
    report_append_interleaved(state.get(), first);
    report_append_interleaved(state.get(), second);
    expect_full_object(first + 3, state->usr.ir_object[0], 0);
    expect_full_object(first + 12, state->usr.ir_object[1], 1);
    expect_full_object(second + 3, state->usr.ir_object[2], 2);
    expect_full_object(second + 12, state->usr.ir_object[3], 3);
}

// The interleaved Z bits (wm_reports.c report_append_interleaved) are the top 8 bits of the 10 bit
// accel_z, like X and Y (>> 2). wiibrew: in this mode "the LSBs are not available", so Z<7:0> is the
// same 8 bit value as the ZZ byte of the normal format (Z<9:2>), 0x3e carries Z<5:4> / Z<7:6> of it
// and 0x3f Z<1:0> / Z<3:2>.
TEST(WiimoteReports, InterleavedZIsTheTopEightBits)
{
    auto state = new_state();
    state->usr.accel_z = 0x2D8; // Z<7:0> of the 8 bit value = 0xB6
    state->sys.reporting_mode = 0x3e;
    uint8_t first[21] = {}, second[21] = {};
    report_append_interleaved(state.get(), first);
    report_append_interleaved(state.get(), second);
    uint8_t z = (uint8_t)(((first[1] >> 5) & 3) << 6 | ((first[0] >> 5) & 3) << 4 | ((second[1] >> 5) & 3) << 2 |
                          ((second[0] >> 5) & 3));
    EXPECT_EQ(z, 0xB6);
}

TEST(WiimoteReports, ExtensionBytesComeFromRegister8)
{
    auto state = new_state();
    state->sys.extension_stream_valid = true;
    state->sys.extension_stream_size = 6;
    for (int i = 0; i < 6; i++)
    {
        state->sys.register_a4[0x08 + i] = (uint8_t)(0x10 + i);
    }
    uint8_t buf[16];
    memset(buf, 0xEE, sizeof(buf));
    report_append_extension(state.get(), buf, 16);
    for (int i = 0; i < 6; i++)
    {
        EXPECT_EQ(buf[i], 0x10 + i) << i;
    }
    for (int i = 6; i < 16; i++)
    {
        EXPECT_EQ(buf[i], 0) << i << " padding";
    }
}

TEST(WiimoteReports, EncryptedExtensionBytesUseTheKeyTables)
{
    // Dolphin derived key: "random idx 0" in wm_crypto_test.cpp
    const uint8_t ft[8] = {0x78, 0x06, 0x54, 0x39, 0xD5, 0x78, 0xE4, 0x12};
    const uint8_t sb[8] = {0x64, 0x2F, 0x70, 0x99, 0xF5, 0xEB, 0x22, 0xCA};
    auto state = new_state();
    state->sys.extension_stream_valid = true;
    state->sys.extension_stream_size = 6;
    state->sys.extension_encrypted = true;
    memcpy(state->sys.extension_crypto_state.ft, ft, 8);
    memcpy(state->sys.extension_crypto_state.sb, sb, 8);
    const uint8_t plain[6] = {0x80, 0x7F, 0x00, 0xFF, 0x12, 0x34};
    memcpy(&state->sys.register_a4[0x08], plain, 6);
    uint8_t buf[6] = {};
    report_append_extension(state.get(), buf, 6);
    for (int i = 0; i < 6; i++)
    {
        int addr = 0x08 + i;
        EXPECT_EQ(buf[i], (uint8_t)((uint8_t)(plain[i] - ft[addr % 8]) ^ sb[addr % 8])) << i;
    }
}
