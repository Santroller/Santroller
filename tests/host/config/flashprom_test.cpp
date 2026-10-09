// FlashPROM: the RAM cache of the config area and how it is written back to flash. Commits wait
// EEPROM_WRITE_WAIT ms, then only erase and program the sectors that changed.
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "config/FlashPROM.h"
#include "config_test_support.hpp"
#include "pio_usb.h"

using namespace config_test;

namespace
{
class FlashPROMTest : public StorageTest
{
protected:
    bool flash_matches_cache() const { return memcmp(flash_area(), EEPROM.writeCache, EEPROM_SIZE_BYTES) == 0; }
};
}

TEST_F(FlashPROMTest, StartCopiesFlashIntoTheCache)
{
    const auto bytes = pattern(EEPROM_SIZE_BYTES, 3);
    memcpy(flash_area(), bytes.data(), bytes.size());
    EEPROM.start();
    EXPECT_TRUE(flash_matches_cache());
}

TEST_F(FlashPROMTest, ACommitWaitsForTheWriteDelay)
{
    EEPROM.writeCache[100] = 0x12;
    EEPROM.commit();
    EEPROM.tick();
    fake_time::advance_ms(EEPROM_WRITE_WAIT - 1);
    EEPROM.tick();
    EXPECT_EQ(fake_flash::erase_count, 0u);
    EXPECT_EQ(flash_area()[100], 0xFF);

    fake_time::advance_ms(1);
    EEPROM.tick();
    EXPECT_EQ(flash_area()[100], 0x12);
    EXPECT_TRUE(flash_matches_cache());
    EXPECT_EQ(EEPROM.should_commit_at, 0u);
}

TEST_F(FlashPROMTest, ANewerCommitPushesTheWriteBack)
{
    EEPROM.commit();
    fake_time::advance_ms(EEPROM_WRITE_WAIT - 10);
    EEPROM.writeCache[0] = 0;
    EEPROM.commit();
    fake_time::advance_ms(20);
    EEPROM.tick();
    EXPECT_EQ(fake_flash::erase_count, 0u);
    fake_time::advance_ms(EEPROM_WRITE_WAIT);
    EEPROM.tick();
    EXPECT_TRUE(flash_matches_cache());
}

TEST_F(FlashPROMTest, CommitNowWritesTheWholeCache)
{
    const auto bytes = pattern(EEPROM_SIZE_BYTES, 9);
    memcpy(EEPROM.writeCache, bytes.data(), bytes.size());
    EEPROM.commit_now();
    EXPECT_TRUE(flash_matches_cache());
    EXPECT_FALSE(fake_flash::program_without_erase);
    EXPECT_EQ(fake_flash::erase_count, EEPROM_SIZE_BYTES / FLASH_SECTOR_SIZE);
}

TEST_F(FlashPROMTest, NothingIsErasedWhenFlashAlreadyMatches)
{
    EEPROM.commit_now();
    EXPECT_EQ(fake_flash::erase_count, 0u);
    EXPECT_EQ(fake_flash::program_count, 0u);
}

TEST_F(FlashPROMTest, OnlyChangedSectorsAreErased)
{
    const auto bytes = pattern(EEPROM_SIZE_BYTES, 5);
    memcpy(EEPROM.writeCache, bytes.data(), bytes.size());
    EEPROM.commit_now();
    fake_flash::erase_count = 0;
    fake_flash::program_count = 0;

    EEPROM.writeCache[FLASH_SECTOR_SIZE * 3 + 17] ^= 0xFF;
    EEPROM.commit_now();
    EXPECT_EQ(fake_flash::erase_count, 1u);
    EXPECT_EQ(fake_flash::program_count, FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE);
    EXPECT_TRUE(flash_matches_cache());
    EXPECT_FALSE(fake_flash::program_without_erase);
}

TEST_F(FlashPROMTest, ErasedPagesInAChangedSectorAreLeftErased)
{
    const auto bytes = pattern(EEPROM_SIZE_BYTES, 5);
    memcpy(EEPROM.writeCache, bytes.data(), bytes.size());
    EEPROM.commit_now();
    fake_flash::program_count = 0;

    // Blank the second page of sector 0, which changes the sector
    memset(EEPROM.writeCache + FLASH_PAGE_SIZE, 0xFF, FLASH_PAGE_SIZE);
    EEPROM.commit_now();
    EXPECT_EQ(fake_flash::program_count, FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE - 1);
    EXPECT_TRUE(flash_matches_cache());
}

TEST_F(FlashPROMTest, FlashOutsideTheConfigAreaIsNeverTouched)
{
    // The sector in front of the config area holds firmware on a real device
    uint8_t *before = fake_flash::memory;
    const size_t before_size = PICO_FLASH_SIZE_BYTES - EEPROM_SIZE_BYTES;
    const auto firmware = pattern(before_size, 77);
    memcpy(before, firmware.data(), before_size);

    const auto bytes = pattern(EEPROM_SIZE_BYTES, 1);
    memcpy(EEPROM.writeCache, bytes.data(), bytes.size());
    EEPROM.commit_now();
    EXPECT_EQ(std::vector<uint8_t>(before, before + before_size), firmware);
}

TEST_F(FlashPROMTest, TheUsbHostIsHandedBackAfterAWrite)
{
    // core1 keeps the USB host's SOFs going while core0 is stalled by the write
    const uint32_t changes = fake_pio_usb::sof_only_changes;
    EEPROM.writeCache[5] = 0;
    EEPROM.commit_now();
    EXPECT_EQ(fake_pio_usb::sof_only_changes, changes + 2);
    EXPECT_FALSE(fake_pio_usb::sof_only);
}

TEST_F(FlashPROMTest, ResetLeavesFlashThatIsNotAConfig)
{
    ASSERT_TRUE(storage.initialize_empty());
    run_pending_commit();
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, false));

    EEPROM.reset();
    run_pending_commit();
    EXPECT_FALSE(storage.read_flash(image, false));
    EXPECT_TRUE(flash_matches_cache());
}
