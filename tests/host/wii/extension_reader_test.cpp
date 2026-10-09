// The I2C conversation WiiExtension (lib/wii_extensions/wii_extension.cpp) has with an extension
// plugged into the Santroller, against the fake I2C master in support/wii. Each test plays the
// extension: it answers the last transfer and signals the stop, then fires the 200us alarm the
// reader waits on before its next transfer.
//
// wiibrew (Wiimote/Extension_Controllers):
//  - "the new way": write 0x55 to 0xF0, then 0x00 to 0xFB; the ID is then readable, unencrypted,
//    as six bytes from 0xFA. Extensions live at I2C address 0x52
//  - 0xFE is the data format; the Classic Controller supports 1, 2 and 3, set by writing 0xFE
//  - encryption is set up by writing 0xAA to 0xF0, then the 16 byte key to 0x40 in 6, 6 and 4
//    byte blocks; with an all zero key both tables are 0x97
//  - the TaTaCon's report is the one byte at 0x05
//  - writing 0x01 / 0x00 to 0xFB turns the DJ Hero Euphoria LED on / off
#include <gtest/gtest.h>
#include <string.h>
#include <vector>
#include "fake_bus.hpp"
#include "pico/time.h"
#include "wii_extension.hpp"

namespace
{
using fake_i2c_master::transfers;

class ExtensionReader : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reset_wii_fakes();
        fake_time::set_us(50'000'000);
        ext.begin();
    }

    WiiExtension ext{nullptr, 0, 4, 5, 400000, 0};

    const fake_i2c_master::Transfer &last() { return transfers.back(); }

    // The extension finishes the last transfer (filling in what was read), and the reader moves on
    void complete(const std::vector<uint8_t> &reply = {})
    {
        ASSERT_FALSE(transfers.empty());
        auto &t = transfers.back();
        ASSERT_EQ(reply.size(), t.read_len) << "reply doesn't match the read";
        if (!reply.empty())
        {
            memcpy(t.read_into, reply.data(), reply.size());
        }
        size_t before = transfers.size();
        ext.process_data(0x52, false, false, false, true);
        fake_time::advance_us(10'000);
        ASSERT_TRUE(fake_alarm::fire()) << "no next step scheduled";
        ASSERT_EQ(transfers.size(), before + 1) << "no transfer after the alarm";
    }

    void expect_write(std::vector<uint8_t> bytes, const char *what)
    {
        ASSERT_FALSE(transfers.empty()) << what;
        EXPECT_EQ(last().addr, 0x52) << what;
        EXPECT_EQ(last().written, bytes) << what;
        EXPECT_EQ(last().read_len, 0u) << what;
    }
    void expect_read(size_t len, const char *what)
    {
        ASSERT_FALSE(transfers.empty()) << what;
        EXPECT_EQ(last().addr, 0x52) << what;
        EXPECT_TRUE(last().written.empty()) << what;
        EXPECT_EQ(last().read_len, len) << what;
    }

    // Through the new way initialisation to the ID read, answering with id
    void identify(const std::vector<uint8_t> &id)
    {
        expect_write({0xF0, 0x55}, "disable encryption");
        complete();
        expect_write({0xFB, 0x00}, "second half of the new way");
        complete();
        expect_write({0xFA}, "point at the ID");
        complete();
        expect_read(6, "read the ID");
        complete(id);
    }
};
} // namespace

TEST_F(ExtensionReader, InitialisesTheNewWayAndReadsTheId)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    EXPECT_EQ(ext.mType, WiiGuitarHeroGuitar);
    expect_write({0x00}, "point at the report");
}

TEST_F(ExtensionReader, PollsSixBytesFromZeroForAGuitar)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    complete();
    expect_read(6, "first report");
    complete({0xE0, 0xE0, 0x0F, 0x00, 0xFF, 0xEF}); // green held
    // non zero, so no encryption: straight to polling
    expect_write({0x00}, "poll");
    complete();
    expect_read(6, "report");
    complete({0xE0, 0xE0, 0x0F, 0x00, 0xFF, 0xEF});
    EXPECT_TRUE(ext.read_button(WiiButtonGuitarGreen));
    EXPECT_FALSE(ext.read_button(WiiButtonGuitarRed));
    EXPECT_EQ(ext.s_box, 0);
}

TEST_F(ExtensionReader, AsksAClassicControllerForDataFormat3)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01});
    EXPECT_EQ(ext.mType, WiiClassicController);
    for (int i = 0; i < 3; i++)
    {
        expect_write({0xFE, 0x03}, "data format 3");
        complete();
    }
    expect_write({0xFA}, "re-read the ID");
    complete();
    expect_read(6, "ID with the data format");
    complete({0x00, 0x00, 0xA4, 0x20, 0x03, 0x01});
    EXPECT_TRUE(ext.hiRes);
    expect_write({0x00}, "point at the report");
    complete();
    expect_read(8, "format 3 reports are 8 bytes");
    complete({0x80, 0x80, 0x80, 0x80, 0x10, 0x20, 0xFF, 0xEF}); // A held
    complete();
    complete({0x80, 0x80, 0x80, 0x80, 0x10, 0x20, 0xFF, 0xEF});
    EXPECT_TRUE(ext.read_button(WiiButtonClassicA));
    EXPECT_EQ(ext.read_axis(WiiAxisClassicLeftTrigger), 0x10 << 8);
}

TEST_F(ExtensionReader, StaysInFormat1WhenTheControllerRefuses)
{
    // Most old third party Classic Controllers only support format 1
    identify({0x01, 0x00, 0xA4, 0x20, 0x01, 0x01});
    EXPECT_EQ(ext.mType, WiiClassicControllerPro);
    for (int i = 0; i < 4; i++)
    {
        complete();
    }
    expect_read(6, "ID");
    complete({0x01, 0x00, 0xA4, 0x20, 0x01, 0x01});
    EXPECT_FALSE(ext.hiRes);
    complete();
    expect_read(6, "format 1 reports are 6 bytes");
}

TEST_F(ExtensionReader, TaikoReadsOnlyByteFive)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x11});
    EXPECT_EQ(ext.mType, WiiTaikoNoTatsujinController);
    expect_write({0x05}, "point at the drum byte");
    complete();
    expect_read(1, "one byte");
    complete({0xBF}); // left centre hit
    complete();
    complete({0xBF});
    EXPECT_TRUE(ext.read_button(WiiButtonTaTaConLeftDrumCenter));
    EXPECT_FALSE(ext.read_button(WiiButtonTaTaConRightDrumCenter));
}

TEST_F(ExtensionReader, RejectsAnAllFFId)
{
    // wiibrew: FF FF FF FF FF FF is what an extension that wasn't initialised properly returns
    identify({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
    EXPECT_NE(ext.mType, WiiGuitarHeroGuitar);
    expect_read(6, "the ID is read again");
}

TEST_F(ExtensionReader, AllZeroDataTurnsOnZeroKeyEncryption)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    complete();
    complete({0, 0, 0, 0, 0, 0});
    expect_write({0xF0, 0xAA}, "enable encryption");
    complete();
    expect_write({0x40, 0, 0, 0, 0, 0, 0}, "key bytes 0-5");
    complete();
    expect_write({0x46, 0, 0, 0, 0, 0, 0}, "key bytes 6-11");
    complete();
    expect_write({0x4C, 0, 0, 0, 0}, "key bytes 12-15");
    complete();
    expect_write({0xFA}, "point at the ID");
    complete();
    expect_read(6, "ID");
    complete({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    EXPECT_NE(ext.s_box, 0);
    expect_write({0x00}, "then poll");
}

TEST_F(ExtensionReader, EncryptedReportsAreDecrypted)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    complete();
    complete({0, 0, 0, 0, 0, 0});
    for (int i = 0; i < 5; i++)
    {
        complete();
    }
    complete({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    ASSERT_NE(ext.s_box, 0);
    uint8_t s = ext.s_box;
    // encrypt a report the way an extension with that single valued table would
    const uint8_t plain[6] = {0xE0, 0xE0, 0x0F, 0x00, 0xFF, 0xBF}; // red held
    std::vector<uint8_t> cipher;
    for (uint8_t p : plain)
    {
        cipher.push_back((uint8_t)((uint8_t)(p - s) ^ s));
    }
    complete();
    complete(cipher);
    EXPECT_EQ(memcmp(ext.mBuffer, plain, 6), 0);
    EXPECT_TRUE(ext.read_button(WiiButtonGuitarRed));
}

TEST_F(ExtensionReader, GarbageReportsAreIgnored)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    complete();
    complete({0xE0, 0xE0, 0x0F, 0x00, 0xFF, 0xEF});
    complete();
    complete({0xE0, 0xE0, 0x0F, 0x00, 0xFF, 0xEF});
    ASSERT_TRUE(ext.read_button(WiiButtonGuitarGreen));
    complete();
    complete({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
    EXPECT_TRUE(ext.read_button(WiiButtonGuitarGreen)) << "an all FF read shouldn't replace the last report";
    complete();
    complete({0, 0, 0, 0, 0, 0});
    EXPECT_TRUE(ext.read_button(WiiButtonGuitarGreen)) << "an all zero read shouldn't replace the last report";
}

TEST_F(ExtensionReader, DrawsomeTabletGetsSwitchedOn)
{
    identify({0xFF, 0x00, 0xA4, 0x20, 0x00, 0x13});
    EXPECT_EQ(ext.mType, WiiUbisoftDrawsomeTablet);
    expect_write({0xFB, 0x01}, "drawsome enable");
}

TEST_F(ExtensionReader, EuphoriaLedIsWrittenTo0xFB)
{
    identify({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03});
    EXPECT_EQ(ext.mType, WiiDjHeroTurntable);
    complete();
    complete({0xE0, 0xE0, 0x00, 0x00, 0xFE, 0xFF});
    ext.setEuphoriaLed(true);
    complete();
    complete({0xE0, 0xE0, 0x00, 0x00, 0xFE, 0xFF});
    expect_write({0xFB, 0x01}, "LED on");
    complete();
    ext.setEuphoriaLed(false);
    complete();
    complete({0xE0, 0xE0, 0x00, 0x00, 0xFE, 0xFF});
    expect_write({0xFB, 0x00}, "LED off");
}

// Only reads from an extension are encrypted, writes are stored as written (Dolphin's
// EncryptedExtension::BusWrite), so the LED byte goes out as is even with encryption on
TEST_F(ExtensionReader, EuphoriaLedIsWrittenUnencryptedWithEncryptionOn)
{
    identify({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03});
    complete();
    complete({0, 0, 0, 0, 0, 0});
    for (int i = 0; i < 5; i++)
    {
        complete();
    }
    complete({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03});
    ASSERT_NE(ext.s_box, 0);
    uint8_t s = ext.s_box;
    std::vector<uint8_t> cipher;
    for (uint8_t p : {0xE0, 0xE0, 0x00, 0x00, 0xFE, 0xFF})
    {
        cipher.push_back((uint8_t)((uint8_t)(p - s) ^ s));
    }
    ext.setEuphoriaLed(true);
    complete();
    complete(cipher);
    expect_write({0xFB, 0x01}, "LED on");
}

TEST_F(ExtensionReader, AFewDroppedTransfersAreTolerated)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    complete();
    complete({0xE0, 0xE0, 0x0F, 0x00, 0xFF, 0xEF});
    for (int i = 0; i < 10; i++)
    {
        ext.process_data(0x52, false, true, false, false);
    }
    EXPECT_EQ(ext.mType, WiiGuitarHeroGuitar);
}

TEST_F(ExtensionReader, LosingTheExtensionStartsOverAfterHalfASecond)
{
    identify({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03});
    complete();
    complete({0xE0, 0xE0, 0x0F, 0x00, 0xFF, 0xEF});
    for (int i = 0; i < 11; i++)
    {
        ext.process_data(0x52, false, true, false, false);
    }
    EXPECT_EQ(ext.mType, WiiNoExtension);
    EXPECT_EQ(fake_alarm::pending.delay_us, 500'000u);
    ASSERT_TRUE(fake_alarm::fire());
    expect_write({0xF0, 0x55}, "initialise again");
}

TEST_F(ExtensionReader, NothingPluggedInRetriesEveryHalfSecond)
{
    expect_write({0xF0, 0x55}, "first attempt");
    ext.process_data(0x52, false, false, true, false);
    EXPECT_EQ(fake_alarm::pending.delay_us, 500'000u);
    ASSERT_TRUE(fake_alarm::fire());
    expect_write({0xF0, 0x55}, "second attempt");
}
