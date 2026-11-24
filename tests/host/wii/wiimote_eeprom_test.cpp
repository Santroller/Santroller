// Reads of the emulated Wii Remote's EEPROM (lib/wii_remote_emulation/wiimote.c), which go out through the
// report queue in wm_reports.c as 0x21 memory responses of up to 16 bytes each. Per wiibrew, a response
// carries the size minus one in the high nibble and an error in the low nibble of the byte after the
// buttons, then the big endian address of its first byte, then 16 bytes of data.
#include <gtest/gtest.h>
#include <string.h>
#include <memory>
extern "C"
{
#include "wiimote.h"
#include "wm_reports.h"
}
#include "eeprom.bin.h"

namespace
{
struct MemResponse
{
    uint8_t type;
    int size;
    int error;
    uint16_t addr;
    const uint8_t *data;
};

MemResponse peek(wiimote_state &state)
{
    report *rpt = report_queue_peek(&state);
    EXPECT_NE(rpt, nullptr);
    auto *resp = reinterpret_cast<report_mem_resp *>(rpt->data.buf);
    const uint8_t *addr = reinterpret_cast<const uint8_t *>(&resp->addr);
    return {rpt->data.type, (resp->size & 0xf) + 1, resp->error & 0xf, (uint16_t)(addr[0] << 8 | addr[1]), resp->data};
}

std::unique_ptr<wiimote_state> new_wiimote()
{
    auto state = std::make_unique<wiimote_state>();
    wiimote_init(state.get());
    // Starting up queues its power on report, which isn't what these are about
    report_queue_clear(state.get());
    return state;
}
} // namespace

TEST(WiimoteEeprom, AReadIsSentSixteenBytesAtATime)
{
    auto state = new_wiimote();
    // The accelerometer calibration and the bytes after it
    read_eeprom(state.get(), 0x16, 0x14);

    MemResponse first = peek(*state);
    EXPECT_EQ(first.type, 0x21);
    EXPECT_EQ(first.error, 0);
    EXPECT_EQ(first.size, 16);
    EXPECT_EQ(first.addr, 0x16);
    EXPECT_EQ(memcmp(first.data, &eeprom_bin_1[0x16], 16), 0);
    report_queue_pop(state.get());

    MemResponse second = peek(*state);
    EXPECT_EQ(second.size, 4);
    EXPECT_EQ(second.addr, 0x26);
    EXPECT_EQ(memcmp(second.data, &eeprom_bin_1[0x26], 4), 0);
    report_queue_pop(state.get());

    EXPECT_EQ(report_queue_peek(state.get()), nullptr);
}

TEST(WiimoteEeprom, ReadsTheEndOfTheEeprom)
{
    auto state = new_wiimote();
    read_eeprom(state.get(), LAST_DATA_BEGIN_POS, 4);
    MemResponse resp = peek(*state);
    EXPECT_EQ(resp.error, 0);
    EXPECT_EQ(resp.addr, LAST_DATA_BEGIN_POS);
    EXPECT_EQ(memcmp(resp.data, eeprom_bin_2, 4), 0);
}

TEST(WiimoteEeprom, TheLastByteIs0x16FF)
{
    auto state = new_wiimote();
    read_eeprom(state.get(), 0x16F0, 0x10);
    MemResponse resp = peek(*state);
    EXPECT_EQ(resp.error, 0);
    EXPECT_EQ(resp.size, 16);
    EXPECT_EQ(memcmp(resp.data, wiimote_eeprom_data() + 0x16F0, 16), 0);
    report_queue_pop(state.get());

    // And it can be written
    const uint8_t value = 0x5A;
    write_eeprom(state.get(), 0x16FF, 1, &value);
    report_queue_clear(state.get());
    EXPECT_EQ(wiimote_eeprom_data()[0x16FF], value);
}

TEST(WiimoteEeprom, ReadsPastTheEndAreAnError)
{
    auto state = new_wiimote();
    // One byte too many
    read_eeprom(state.get(), 0x16F0, 0x11);
    MemResponse resp = peek(*state);
    EXPECT_EQ(resp.type, 0x21);
    EXPECT_EQ(resp.error, 8);
    report_queue_pop(state.get());
    EXPECT_EQ(report_queue_peek(state.get()), nullptr);
}
