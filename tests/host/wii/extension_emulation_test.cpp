// Pretending to be an extension for a Wii Remote: lib/wii_extension_emulation, driven over the
// fake I2C bus the way a Wii Remote drives a real extension.
//
// Expected values come from wiibrew (Wiimote/Extension_Controllers and its per controller pages):
//  - the six identification bytes at 0xFA, whose 0xFE byte is the data format and is writable
//  - 0x55 to 0xF0 then 0x00 to 0xFB initialises an extension unencrypted ("the new way")
//  - 0xAA to 0xF0 and a 16 byte key at 0x40 (written as 6, 6 and 4 bytes) turn encryption on,
//    after which every byte read is encrypted as (byte - ft[addr % 8]) ^ sb[addr % 8]
//  - the TaTaCon's game reads six bytes from 0x00, with the drum hits in byte 5
//  - writing 0x01 / 0x00 to 0xFB turns the DJ Hero Euphoria LED on / off
// The encryption tables for a real key come from Dolphin's key generation (see wm_crypto_test.cpp),
// and the calibration checksum rule from Dolphin's UpdateCalibrationDataChecksum: the 15th byte is
// 0x55 plus the sum of the first 14, the 16th is the 15th plus 0x55.
#include <gtest/gtest.h>
#include <string.h>
#include "fake_bus.hpp"
#include "wiimote_bus.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "wii_extension_emulation.hpp"
#include "wii_extension_backend.h"

namespace
{
constexpr uint8_t SDA = 4, SCL = 5, DETECT = 6;

// Dolphin derived key: "random idx 0" in wm_crypto_test.cpp
const uint8_t real_key[16] = {0xD4, 0xB5, 0xFF, 0x7D, 0x87, 0xD1, 0x66, 0x9B,
                              0x5D, 0xF0, 0xF3, 0x3D, 0x2D, 0x83, 0x03, 0x4A};
const uint8_t real_ft[8] = {0x78, 0x06, 0x54, 0x39, 0xD5, 0x78, 0xE4, 0x12};
const uint8_t real_sb[8] = {0x64, 0x2F, 0x70, 0x99, 0xF5, 0xEB, 0x22, 0xCA};
// "random idx 3"
const uint8_t other_key[16] = {0x73, 0x14, 0x68, 0x75, 0xD0, 0x6D, 0xA2, 0xD4,
                               0xDA, 0xA6, 0x5A, 0x1A, 0x78, 0xC2, 0x69, 0xED};
const uint8_t other_ft[8] = {0x3D, 0x25, 0xF4, 0x87, 0xB3, 0xDD, 0x97, 0x4A};
const uint8_t other_sb[8] = {0xE3, 0x8C, 0x06, 0x3A, 0x61, 0x32, 0x56, 0xE8};

std::vector<uint8_t> encrypted(const std::vector<uint8_t> &plain, uint8_t addr, const uint8_t ft[8],
                               const uint8_t sb[8])
{
    std::vector<uint8_t> out;
    for (size_t i = 0; i < plain.size(); i++)
    {
        uint8_t a = (uint8_t)(addr + i);
        out.push_back((uint8_t)((uint8_t)(plain[i] - ft[a % 8]) ^ sb[a % 8]));
    }
    return out;
}

class ExtensionEmulation : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reset_wii_fakes();
        // The emulation's contexts are static, so move the clock well past anything an earlier
        // test left in them
        static uint64_t start_us = 10'000'000;
        start_us += 100'000'000;
        fake_time::set_us(start_us);
    }
    WiimoteBus bus;
};
} // namespace

TEST_F(ExtensionEmulation, DetectStaysLowUntilBegin)
{
    WiiExtensionEmulation emu(0, SDA, SCL, DETECT);
    EXPECT_FALSE(fake_gpio::level[DETECT]);
    EXPECT_EQ(fake_i2c::slave_handler[0], nullptr);
    emu.begin(Gamepad);
    EXPECT_TRUE(fake_gpio::level[DETECT]);
    EXPECT_EQ(fake_i2c::slave_address[0], 0x52);
    ASSERT_NE(fake_i2c::slave_handler[0], nullptr);
}

TEST_F(ExtensionEmulation, EndDropsDetectAndLeavesTheBus)
{
    WiiExtensionEmulation emu(0, SDA, SCL, DETECT);
    emu.begin(Gamepad);
    emu.end();
    EXPECT_FALSE(fake_gpio::level[DETECT]);
    EXPECT_EQ(fake_i2c::slave_handler[0], nullptr);
}

TEST_F(ExtensionEmulation, SecondBlockUsesTheSecondI2C)
{
    WiiExtensionEmulation emu(1, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.block = 1;
    ASSERT_NE(fake_i2c::slave_handler[1], nullptr);
    bus.init_new_way();
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}));
}

namespace
{
struct EmulatedId
{
    SubType subtype;
    const char *name;
    std::vector<uint8_t> id;
};
} // namespace

TEST_F(ExtensionEmulation, IdentifiesAsTheExtensionForEachSubtype)
{
    const EmulatedId cases[] = {
        {Gamepad, "Classic Controller", {0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}},
        {GuitarHeroGuitar, "Guitar Hero guitar", {0x00, 0x00, 0xA4, 0x20, 0x01, 0x03}},
        {GuitarHeroDrums, "Guitar Hero World Tour drums", {0x01, 0x00, 0xA4, 0x20, 0x01, 0x03}},
        {DjHeroTurntable, "DJ Hero turntable", {0x03, 0x00, 0xA4, 0x20, 0x01, 0x03}},
        {Taiko, "TaTaCon", {0x00, 0x00, 0xA4, 0x20, 0x01, 0x11}},
    };
    for (const auto &c : cases)
    {
        reset_wii_fakes();
        WiiExtensionEmulation emu(0, SDA, SCL, -1);
        emu.begin(c.subtype);
        bus.init_new_way();
        EXPECT_EQ(bus.read(0xFA, 6), c.id) << c.name;
    }
}

TEST_F(ExtensionEmulation, OtherSubtypesLookLikeAClassicController)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(RockBandGuitar);
    bus.init_new_way();
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}));
}

TEST_F(ExtensionEmulation, BackendIdsForNunchukAndBalanceBoard)
{
    // No subtype picks these, but the backend knows them
    uint8_t registers[256];
    bool encrypted_flag = true;
    wii_extension_backend_init(registers, &encrypted_flag, WII_EXTENSION_NUNCHUK);
    EXPECT_FALSE(encrypted_flag);
    EXPECT_EQ(std::vector<uint8_t>(registers + 0xFA, registers + 0x100), bytes({0x00, 0x00, 0xA4, 0x20, 0x00, 0x00}));
    wii_extension_backend_init(registers, &encrypted_flag, WII_EXTENSION_BALANCE_BOARD);
    EXPECT_EQ(std::vector<uint8_t>(registers + 0xFA, registers + 0x100), bytes({0x00, 0x00, 0xA4, 0x20, 0x04, 0x02}));
}

TEST_F(ExtensionEmulation, CalibrationBlocksCarryValidChecksums)
{
    for (uint8_t type : {WII_EXTENSION_NUNCHUK, WII_EXTENSION_CLASSIC, WII_EXTENSION_GUITAR, WII_EXTENSION_DRUMS,
                         WII_EXTENSION_TURNTABLE})
    {
        uint8_t registers[256];
        bool encrypted_flag;
        wii_extension_backend_init(registers, &encrypted_flag, type);
        for (int block : {0x20, 0x30})
        {
            uint8_t sum = 0x55;
            for (int i = 0; i < 14; i++)
            {
                sum += registers[block + i];
            }
            EXPECT_EQ(registers[block + 14], sum) << int(type) << " " << block;
            EXPECT_EQ(registers[block + 15], (uint8_t)(sum + 0x55)) << int(type) << " " << block;
        }
    }
}

TEST_F(ExtensionEmulation, CalibrationReadsBackOverTheBus)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    auto cal = bus.read(0x20, 16);
    ASSERT_EQ(cal.size(), 16u);
    uint8_t sum = 0x55;
    for (int i = 0; i < 14; i++)
    {
        sum += cal[i];
    }
    EXPECT_EQ(cal[14], sum);
    EXPECT_EQ(cal[15], (uint8_t)(sum + 0x55));
    EXPECT_EQ(bus.read(0x30, 16), cal);
}

TEST_F(ExtensionEmulation, InputsAreReadFromRegisterZero)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    uint8_t report[6] = {0x20, 0x20, 0x90, 0x00, 0xFF, 0xEF};
    emu.set_inputs(report, sizeof(report));
    EXPECT_EQ(bus.read(0x00, 6), std::vector<uint8_t>(report, report + 6));
}

TEST_F(ExtensionEmulation, TaikoReportMatchesTheTaTaConLayout)
{
    // The game reads six bytes from 0x00; byte 5 is 1 CL RL CR RR 1 1 1, with hits clearing bits
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Taiko);
    bus.init_new_way();
    auto report = bus.read(0x00, 6);
    ASSERT_EQ(report.size(), 6u);
    EXPECT_EQ(report[5], 0xFF);
}

TEST_F(ExtensionEmulation, DataFormatRegisterIsWritable)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    EXPECT_EQ(emu.wii_data_format(), 1);
    bus.write(0xFE, {0x03});
    EXPECT_EQ(emu.wii_data_format(), 3);
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x00, 0x00, 0xA4, 0x20, 0x03, 0x01}));
    bus.write(0xFE, {0x02});
    EXPECT_EQ(emu.wii_data_format(), 2);
}

TEST_F(ExtensionEmulation, WritesAutoIncrementTheRegisterPointer)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    bus.write(0x60, {0x01, 0x02, 0x03});
    EXPECT_EQ(bus.read(0x60, 3), bytes({0x01, 0x02, 0x03}));
}

TEST_F(ExtensionEmulation, EuphoriaLedFollowsWritesTo0xFB)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(DjHeroTurntable);
    bus.init_new_way();
    EXPECT_FALSE(emu.get_djh_euphoria_led_state());
    bus.write(0xFB, {0x01});
    EXPECT_TRUE(emu.get_djh_euphoria_led_state());
    bus.write(0xFB, {0x00});
    EXPECT_FALSE(emu.get_djh_euphoria_led_state());
}

// The key is written before 0xAA here, RekeyingUsesTheNewKey covers the documented order (0xAA first)
TEST_F(ExtensionEmulation, EncryptsReadsWithTheTablesForTheWiimotesKey)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    bus.write_key(real_key);
    bus.write(0xF0, {0xAA});
    auto id = bus.read(0xFA, 6);
    EXPECT_EQ(id, encrypted(bytes({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}), 0xFA, real_ft, real_sb));

    uint8_t report[6] = {0x21, 0x1F, 0x8F, 0x00, 0xFF, 0xFF};
    emu.set_inputs(report, sizeof(report));
    EXPECT_EQ(bus.read(0x00, 6), encrypted(std::vector<uint8_t>(report, report + 6), 0x00, real_ft, real_sb));
}

TEST_F(ExtensionEmulation, NewWayInitTurnsEncryptionBackOff)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(GuitarHeroGuitar);
    bus.init_new_way();
    bus.write_key(real_key);
    bus.write(0xF0, {0xAA});
    bus.init_new_way();
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03}));
}

TEST_F(ExtensionEmulation, KeyWrittenInOneBlockAlsoWorks)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    bus.write(0x40, std::vector<uint8_t>(real_key, real_key + 16));
    EXPECT_EQ(bus.read(0xFE, 2), encrypted(bytes({0x01, 0x01}), 0xFE, real_ft, real_sb));
}

// Writes to an extension aren't encrypted: wiibrew only describes encrypting data read from the
// extension, and Dolphin's EncryptedExtension::BusWrite stores writes as they are.
// wii_extension_backend_write (lib/wii_extension_emulation/wii_extension_backend.c) does the same,
// so a new key written after 0xAA (a new game, or a re-plug) isn't mangled by the old tables.
TEST_F(ExtensionEmulation, RekeyingUsesTheNewKey)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    // first session, in wiibrew's order
    bus.init_new_way();
    bus.write(0xF0, {0xAA});
    bus.write_key(real_key);
    ASSERT_EQ(bus.read(0xFE, 2), encrypted(bytes({0x01, 0x01}), 0xFE, real_ft, real_sb));
    // the next session sets up a different key the same way
    bus.init_new_way();
    bus.write(0xF0, {0xAA});
    bus.write_key(other_key);
    EXPECT_EQ(bus.read(0xFE, 2), encrypted(bytes({0x01, 0x01}), 0xFE, other_ft, other_sb));
}

// Same as RekeyingUsesTheNewKey: with encryption on, a plain write of the data format byte is
// stored as written, so the Wii Remote's 0x03 selects format 3
TEST_F(ExtensionEmulation, DataFormatWriteWhileEncryptedIsStoredAsWritten)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    bus.write_key(real_key);
    bus.write(0xF0, {0xAA});
    bus.write(0xFE, {0x03});
    EXPECT_EQ(emu.wii_data_format(), 3);
}

TEST_F(ExtensionEmulation, IsCommunicatingWhileTheWiimoteKeepsPolling)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    EXPECT_FALSE(emu.is_communicating());
    emu.begin(Gamepad);
    EXPECT_FALSE(emu.is_communicating());
    bus.read(0x00, 6);
    EXPECT_TRUE(emu.is_communicating());
    fake_time::advance_us(900'000);
    EXPECT_TRUE(emu.is_communicating());
    fake_time::advance_us(200'000);
    EXPECT_FALSE(emu.is_communicating());
}

TEST_F(ExtensionEmulation, ChangingSubtypeWithDetectSimulatesAReplug)
{
    WiiExtensionEmulation emu(0, SDA, SCL, DETECT);
    emu.begin(Gamepad);
    emu.begin(GuitarHeroGuitar);
    EXPECT_FALSE(fake_gpio::level[DETECT]);
    EXPECT_EQ(fake_i2c::slave_handler[0], nullptr);
    EXPECT_TRUE(emu.is_communicating());
    fake_time::advance_us((WII_SWAP_DISCONNECT_MS - 1) * 1000);
    emu.update();
    EXPECT_FALSE(fake_gpio::level[DETECT]);
    fake_time::advance_us(1000);
    emu.update();
    EXPECT_TRUE(fake_gpio::level[DETECT]);
    bus.init_new_way();
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x00, 0x00, 0xA4, 0x20, 0x01, 0x03}));
}

TEST_F(ExtensionEmulation, ChangingSubtypeWithoutDetectSwapsTheIdInPlace)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    bus.init_new_way();
    emu.begin(DjHeroTurntable);
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x03, 0x00, 0xA4, 0x20, 0x01, 0x03}));
}

TEST_F(ExtensionEmulation, ReleasingGoesBackToTheIdleSubtypeAfterTheGracePeriod)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    emu.acquire();
    emu.begin(GuitarHeroDrums);
    bus.init_new_way();
    emu.release(Gamepad);
    fake_time::advance_us((WII_RELEASE_GRACE_MS - 1) * 1000);
    emu.update();
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03}));
    fake_time::advance_us(1000);
    emu.update();
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}));
}

TEST_F(ExtensionEmulation, ReacquiringWithinTheGracePeriodKeepsTheSubtype)
{
    WiiExtensionEmulation emu(0, SDA, SCL, -1);
    emu.begin(Gamepad);
    emu.acquire();
    emu.begin(GuitarHeroDrums);
    bus.init_new_way();
    emu.release(Gamepad);
    emu.acquire();
    fake_time::advance_us(WII_RELEASE_GRACE_MS * 2000);
    emu.update();
    EXPECT_EQ(bus.read(0xFA, 6), bytes({0x01, 0x00, 0xA4, 0x20, 0x01, 0x03}));
}
