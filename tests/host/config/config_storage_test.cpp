// ConfigStorage: how a config from the config tool gets into the flash cache, and how it is read back.
//
// The tool sends a ConfigInfo report (sizes, CRC32, magic), then the data in 63 byte reports. The data is
// staged at the start of the cache, and once the last byte arrives and the CRC matches it is moved to sit
// right in front of a 24 byte footer at the end of the config area. Nothing reaches flash until something
// commits the cache.
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "config/config_storage.hpp"
#include "config_test_support.hpp"

using namespace config_test;
using WriteResult = ConfigStorage::WriteResult;

namespace
{
const uint8_t *cache() { return EEPROM.writeCache; }

std::vector<uint8_t> image_bytes(const ConfigImage &image)
{
    return std::vector<uint8_t>(image.data, image.data + image.data_size);
}

class ConfigStorageTest : public StorageTest
{
};
}

// Upload ---------------------------------------------------------------------------------------------

TEST_F(ConfigStorageTest, InOrderUploadCompletesAndReadsBackFromTheCache)
{
    const auto main = pattern(200, 3);
    const auto aux = pattern(40, 90);
    EXPECT_EQ(upload(storage, main, aux), WriteResult::Done);

    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image.data_size, 240u);
    EXPECT_EQ(image.main_size, 200u);
    EXPECT_EQ(image.aux_size, 40u);
    EXPECT_EQ(image_bytes(image), concat(main, aux));
}

TEST_F(ConfigStorageTest, EveryChunkBeforeTheLastIsInProgress)
{
    const auto data = pattern(63 * 3 + 5);
    const auto info = info_report_for(data, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto reports = data_reports(data);
    ASSERT_EQ(reports.size(), 4u);
    for (size_t i = 0; i < reports.size(); i++)
    {
        const auto result = storage.write_chunk(reports[i].data(), uint16_t(reports[i].size()), uint32_t(i * REPORT_SIZE));
        EXPECT_EQ(result, i + 1 == reports.size() ? WriteResult::Done : WriteResult::InProgress) << "chunk " << i;
    }
}

TEST_F(ConfigStorageTest, SizesOnAndAroundAReportBoundaryComplete)
{
    for (uint32_t size : {1u, 62u, 63u, 64u, 126u, 127u})
    {
        SCOPED_TRACE(size);
        const auto data = pattern(size, uint8_t(size));
        EXPECT_EQ(upload(storage, data, {}), WriteResult::Done);
        ConfigImage image;
        ASSERT_TRUE(storage.read_flash(image, true));
        EXPECT_EQ(image_bytes(image), data);
    }
}

TEST_F(ConfigStorageTest, TheLargestConfigThatFitsCompletes)
{
    const auto data = pattern(MAX_DATA_SIZE, 11);
    EXPECT_EQ(upload(storage, data, {}), WriteResult::Done);
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), data);
}

TEST_F(ConfigStorageTest, FooterLayoutIsSixLittleEndianWordsAtTheEndOfTheArea)
{
    // Configs saved by older firmware have to keep loading, so this layout can't change
    const auto main = pattern(100);
    const auto aux = pattern(7);
    ASSERT_EQ(upload(storage, main, aux), WriteResult::Done);
    const Footer footer = read_raw_footer(cache());
    EXPECT_EQ(footer.data_size, 107u);
    EXPECT_EQ(footer.data_crc, crc32(concat(main, aux)));
    EXPECT_EQ(footer.main_size, 100u);
    EXPECT_EQ(footer.aux_size, 7u);
    EXPECT_EQ(footer.magic, CONFIG_MAGIC);
}

TEST_F(ConfigStorageTest, DataSitsRightBeforeTheFooterAndTheRestOfTheAreaIsCleared)
{
    const auto data = pattern(300);
    ASSERT_EQ(upload(storage, data, {}), WriteResult::Done);
    const uint8_t *data_start = cache() + EEPROM_SIZE_BYTES - FOOTER_SIZE - data.size();
    EXPECT_EQ(std::vector<uint8_t>(data_start, data_start + data.size()), data);
    for (const uint8_t *p = cache(); p < data_start; p++)
    {
        ASSERT_EQ(*p, 0) << "offset " << (p - cache());
    }
}

TEST_F(ConfigStorageTest, PaddingInTheLastReportDoesNotReachTheFooter)
{
    // 64 bytes is one full report and one with a single byte of data plus 62 of padding
    const auto data = pattern(64);
    ASSERT_EQ(upload(storage, data, {}), WriteResult::Done);
    EXPECT_EQ(read_raw_footer(cache()).data_size, 64u);
    EXPECT_EQ(read_raw_footer(cache()).magic, CONFIG_MAGIC);
}

TEST_F(ConfigStorageTest, AnEmptyConfigNeedsNoChunksAndIsAValidEmptyImage)
{
    // The tool sends only the ConfigInfo for an empty buffer
    const auto info = info_report_for({}, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image.data_size, 0u);
    EXPECT_EQ(image.main_size, 0u);
    EXPECT_EQ(image.aux_size, 0u);
}

TEST_F(ConfigStorageTest, ASecondUploadReplacesTheFirst)
{
    ASSERT_EQ(upload(storage, pattern(500, 1), pattern(20, 2)), WriteResult::Done);
    const auto main = pattern(90, 5);
    ASSERT_EQ(upload(storage, main, {}), WriteResult::Done);
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), main);
    EXPECT_EQ(image.aux_size, 0u);
}

TEST_F(ConfigStorageTest, ADuplicatedChunkIsHarmless)
{
    const auto data = pattern(200);
    const auto info = info_report_for(data, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto reports = data_reports(data);
    EXPECT_EQ(storage.write_chunk(reports[0].data(), REPORT_SIZE, 0), WriteResult::InProgress);
    EXPECT_EQ(storage.write_chunk(reports[0].data(), REPORT_SIZE, 0), WriteResult::InProgress);
    EXPECT_EQ(storage.write_chunk(reports[1].data(), REPORT_SIZE, 63), WriteResult::InProgress);
    EXPECT_EQ(storage.write_chunk(reports[2].data(), REPORT_SIZE, 126), WriteResult::InProgress);
    EXPECT_EQ(storage.write_chunk(reports[3].data(), REPORT_SIZE, 189), WriteResult::Done);
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), data);
}

TEST_F(ConfigStorageTest, ChunksInReverseOrderNeverCompleteAsAValidConfig)
{
    // The chunk holding the last byte ends the upload, so anything sent before it is missing
    const auto data = pattern(250);
    const auto info = info_report_for(data, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto reports = data_reports(data);
    for (size_t i = reports.size(); i-- > 0;)
    {
        EXPECT_NE(storage.write_chunk(reports[i].data(), REPORT_SIZE, uint32_t(i * REPORT_SIZE)), WriteResult::Done);
    }
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, true));
}

TEST_F(ConfigStorageTest, AMissingChunkFailsTheCrc)
{
    const auto data = pattern(250);
    const auto info = info_report_for(data, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto reports = data_reports(data);
    for (size_t i = 0; i < reports.size(); i++)
    {
        if (i == 1)
        {
            continue;
        }
        const auto result = storage.write_chunk(reports[i].data(), REPORT_SIZE, uint32_t(i * REPORT_SIZE));
        if (i + 1 == reports.size())
        {
            EXPECT_EQ(result, WriteResult::Invalid);
        }
    }
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, true));
}

TEST_F(ConfigStorageTest, ACorruptedByteFailsTheCrc)
{
    auto data = pattern(250);
    const auto info = info_report_for(data, {});
    data[123] ^= 0x10;
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    WriteResult result = WriteResult::InProgress;
    uint32_t start = 0;
    for (const auto &report : data_reports(data))
    {
        result = storage.write_chunk(report.data(), REPORT_SIZE, start);
        start += REPORT_SIZE;
    }
    EXPECT_EQ(result, WriteResult::Invalid);
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, true));
}

TEST_F(ConfigStorageTest, AWrongCrcInTheInfoFailsTheUpload)
{
    const auto data = pattern(100);
    const auto info = info_report(100, int32_t(crc32(data) ^ 1), 100, 0);
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    WriteResult result = WriteResult::InProgress;
    uint32_t start = 0;
    for (const auto &report : data_reports(data))
    {
        result = storage.write_chunk(report.data(), REPORT_SIZE, start);
        start += REPORT_SIZE;
    }
    EXPECT_EQ(result, WriteResult::Invalid);
}

TEST_F(ConfigStorageTest, AnUndecodableInfoIsRejectedAndLeavesTheStoredConfigAlone)
{
    const auto main = pattern(100);
    ASSERT_EQ(upload(storage, main, {}), WriteResult::Done);
    // A length prefix promising more than the report holds
    std::vector<uint8_t> junk(REPORT_SIZE, 0xFF);
    junk[0] = 0x7F;
    EXPECT_FALSE(storage.write_info(junk.data(), uint16_t(junk.size())));
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), main);
}

TEST_F(ConfigStorageTest, AnInfoMissingRequiredFieldsIsRejected)
{
    // Just the length prefix and dataSize = 10
    const uint8_t info[] = {0x02, 0x08, 0x0A};
    EXPECT_FALSE(storage.write_info(info, sizeof(info)));
}

TEST_F(ConfigStorageTest, UploadingDoesNotTouchFlashByItself)
{
    ASSERT_EQ(upload(storage, pattern(400), pattern(10)), WriteResult::Done);
    run_pending_commit();
    EXPECT_EQ(fake_flash::erase_count, 0u);
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, false));
}

TEST_F(ConfigStorageTest, AnUploadCutOffHalfwayLeavesTheSavedConfigInFlash)
{
    const auto old_main = pattern(300, 40);
    const auto old_aux = pattern(12, 41);
    boot_with_flash_image(old_main, old_aux);

    const auto data = pattern(1000, 50);
    const auto info = info_report_for(data, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto reports = data_reports(data);
    for (size_t i = 0; i < reports.size() / 2; i++)
    {
        ASSERT_EQ(storage.write_chunk(reports[i].data(), REPORT_SIZE, uint32_t(i * REPORT_SIZE)), WriteResult::InProgress);
    }

    // Half a config must never load from the cache...
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, true));
    // ...and flash still has the old one, so the device boots with it after a reset
    EXPECT_EQ(EEPROM.should_commit_at, 0u);
    ASSERT_TRUE(storage.read_flash(image, false));
    EXPECT_EQ(image_bytes(image), concat(old_main, old_aux));
    EEPROM.start();
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), concat(old_main, old_aux));
}

TEST_F(ConfigStorageTest, ARestartedUploadAfterAnAbortedOneCompletes)
{
    const auto info = info_report(1000, 1234, 1000, 0);
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto junk = pattern(REPORT_SIZE, 9);
    ASSERT_EQ(storage.write_chunk(junk.data(), REPORT_SIZE, 0), WriteResult::InProgress);

    const auto main = pattern(150, 60);
    EXPECT_EQ(upload(storage, main, {}), WriteResult::Done);
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), main);
}

// The tool's sizes are signed. A size that doesn't fit the config area is refused up front, as otherwise
// write_chunk would copy the data past the end of the 32KB cache.
TEST_F(ConfigStorageTest, InfoWithMoreDataThanTheAreaHoldsIsRejected)
{
    const auto too_big = info_report(int32_t(MAX_DATA_SIZE + 1), 0, int32_t(MAX_DATA_SIZE + 1), 0);
    EXPECT_FALSE(storage.write_info(too_big.data(), uint16_t(too_big.size())));
    const auto negative = info_report(-24, 0, -24, 0);
    EXPECT_FALSE(storage.write_info(negative.data(), uint16_t(negative.size())));
}

// The loader decodes main_size bytes and then aux_size more from the start of the data, so sections that
// don't add up to dataSize would make it read past the image
TEST_F(ConfigStorageTest, InfoWhoseSectionsDontAddUpToTheDataIsRejected)
{
    const auto info = info_report(100, 0, 200, 50);
    EXPECT_FALSE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto short_sections = info_report(100, 0, 50, 10);
    EXPECT_FALSE(storage.write_info(short_sections.data(), uint16_t(short_sections.size())));
}

TEST_F(ConfigStorageTest, InfoWithTheWrongMagicIsRejected)
{
    const auto info = info_report(100, 0, 100, 0, 0x12345678);
    EXPECT_FALSE(storage.write_info(info.data(), uint16_t(info.size())));
}

TEST_F(ConfigStorageTest, ARejectedInfoLeavesTheStoredConfigAndIgnoresTheChunksAfterIt)
{
    // A large config, so chunks written at the start of the cache would land on top of it
    const auto main = pattern(20000, 12);
    ASSERT_EQ(upload(storage, main, {}), WriteResult::Done);
    const auto bad = info_report(MAX_DATA_SIZE + 1, 0, MAX_DATA_SIZE + 1, 0);
    ASSERT_FALSE(storage.write_info(bad.data(), uint16_t(bad.size())));
    const auto chunk = pattern(REPORT_SIZE, 99);
    for (uint32_t start = 0; start < 20000; start += REPORT_SIZE)
    {
        ASSERT_EQ(storage.write_chunk(chunk.data(), REPORT_SIZE, start), WriteResult::Invalid);
    }
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), main);
}

TEST_F(ConfigStorageTest, ChunksWithoutAnInfoAreIgnored)
{
    const auto chunk = pattern(REPORT_SIZE);
    EXPECT_EQ(storage.write_chunk(chunk.data(), REPORT_SIZE, 0), WriteResult::Invalid);
    ASSERT_EQ(upload(storage, pattern(30), {}), WriteResult::Done);
    // nor once the upload is done
    EXPECT_EQ(storage.write_chunk(chunk.data(), REPORT_SIZE, 0), WriteResult::Invalid);
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), pattern(30));
}

// A chunk starting past the declared size (the tool and device disagreeing on the size) used to make
// dataSize - start wrap to ~65K bytes, which write_chunk copied out of the 63 byte report and past the cache
TEST_F(ConfigStorageTest, AChunkPastTheDeclaredEndIsRejectedWithoutWriting)
{
    const auto main = pattern(10);
    const auto info = info_report_for(main, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto chunk = pattern(REPORT_SIZE);
    EXPECT_NE(storage.write_chunk(chunk.data(), REPORT_SIZE, 63), WriteResult::Done);
    EXPECT_EQ(read_raw_footer(cache()).data_size, 10u);
}

// Reading ---------------------------------------------------------------------------------------------

TEST_F(ConfigStorageTest, ErasedFlashIsNotAConfig)
{
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, false));
    EXPECT_FALSE(storage.read_flash(image, true));
    EXPECT_NE(storage.read_metadata(false).magic, CONFIG_MAGIC);
}

TEST_F(ConfigStorageTest, ZeroedFlashIsNotAConfig)
{
    // What FlashPROM::reset leaves behind
    memset(flash_area(), 0, EEPROM_SIZE_BYTES);
    EEPROM.start();
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, true));
}

TEST_F(ConfigStorageTest, AnImageWithTheWrongMagicIsNotAConfig)
{
    const auto data = pattern(100);
    Footer footer = valid_footer(data, {});
    footer.magic ^= 0x01000000;
    write_raw_image(flash_area(), data, footer);
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, false));
}

TEST_F(ConfigStorageTest, ACorruptedByteInFlashIsNotAConfig)
{
    auto data = pattern(100);
    const Footer footer = valid_footer(data, {});
    data[0] ^= 0x80;
    write_raw_image(flash_area(), data, footer);
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, false));
}

TEST_F(ConfigStorageTest, ACorruptedCrcIsNotAConfig)
{
    const auto data = pattern(100);
    Footer footer = valid_footer(data, {});
    footer.data_crc ^= 0x00010000;
    write_raw_image(flash_area(), data, footer);
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, false));
}

TEST_F(ConfigStorageTest, ATruncatedImageIsNotAConfig)
{
    // The footer of a 200 byte config over only 120 bytes of it
    const auto data = pattern(200);
    const Footer footer = valid_footer(data, {});
    write_raw_image(flash_area(), std::vector<uint8_t>(data.begin() + 80, data.end()), footer);
    memset(flash_area() + EEPROM_SIZE_BYTES - FOOTER_SIZE - 200, 0xFF, 80);
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, false));
}

TEST_F(ConfigStorageTest, DataSizesLargerThanTheAreaAreRejectedWithoutReadingPastIt)
{
    for (uint32_t size : {MAX_DATA_SIZE + 1, uint32_t(EEPROM_SIZE_BYTES), 0x10000u, 0x7FFFFFFFu, 0xFFFFFFFFu})
    {
        SCOPED_TRACE(size);
        write_raw_image(flash_area(), {}, {size, 0, size, 0, CONFIG_MAGIC, 0});
        ConfigImage image;
        EXPECT_FALSE(storage.read_flash(image, false));
    }
}

TEST_F(ConfigStorageTest, AValidImageInFlashReadsBackUncached)
{
    const auto main = pattern(321, 4);
    const auto aux = pattern(17, 8);
    write_raw_image(flash_area(), concat(main, aux), valid_footer(main, aux));
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, false));
    EXPECT_EQ(image.main_size, 321u);
    EXPECT_EQ(image.aux_size, 17u);
    EXPECT_EQ(image_bytes(image), concat(main, aux));
    EXPECT_EQ(image.data, flash_area() + EEPROM_SIZE_BYTES - FOOTER_SIZE - 338);
}

TEST_F(ConfigStorageTest, MetadataReportsTheFooter)
{
    const auto main = pattern(50);
    const auto aux = pattern(5);
    ASSERT_EQ(upload(storage, main, aux), WriteResult::Done);
    const ConfigMetadata metadata = storage.read_metadata(true);
    EXPECT_EQ(metadata.data_size, 55u);
    EXPECT_EQ(metadata.data_crc, crc32(concat(main, aux)));
    EXPECT_EQ(metadata.main_size, 50u);
    EXPECT_EQ(metadata.aux_size, 5u);
    EXPECT_EQ(metadata.magic, CONFIG_MAGIC);
}

// A footer can have a good CRC and still claim sections bigger than the data: the CRC only covers the
// data, and ConfigLoader reads main_size / aux_size bytes from the image
TEST_F(ConfigStorageTest, AnImageWhoseSectionsDontFitItsDataIsNotAConfig)
{
    const auto data = pattern(100);
    Footer footer = valid_footer(data, {});
    footer.main_size = 100;
    footer.aux_size = 4000;
    write_raw_image(flash_area(), data, footer);
    ConfigImage image;
    EXPECT_FALSE(storage.read_flash(image, false));

    footer.main_size = 5000;
    footer.aux_size = 0;
    write_raw_image(flash_area(), data, footer);
    EXPECT_FALSE(storage.read_flash(image, false));
}

// Reading the config back to the tool -----------------------------------------------------------------

TEST_F(ConfigStorageTest, ReadingInReportSizedChunksReturnsTheWholeImage)
{
    // Like fetchConfigData in the tool
    const auto main = pattern(200, 21);
    const auto aux = pattern(30, 22);
    ASSERT_EQ(upload(storage, main, aux), WriteResult::Done);
    std::vector<uint8_t> out;
    uint32_t start = 0;
    while (true)
    {
        uint8_t buffer[REPORT_SIZE];
        const uint32_t read = storage.read_chunk(buffer, start, REPORT_SIZE, true);
        if (read == 0)
        {
            break;
        }
        ASSERT_LE(read, REPORT_SIZE);
        out.insert(out.end(), buffer, buffer + read);
        start += read;
    }
    EXPECT_EQ(out, concat(main, aux));
}

TEST_F(ConfigStorageTest, ReadingAnInvalidImageReturnsNothing)
{
    uint8_t buffer[REPORT_SIZE];
    EXPECT_EQ(storage.read_chunk(buffer, 0, REPORT_SIZE, true), 0u);
}

// copy_config hands the tool whatever offset its report counter has reached, which can be past the end
TEST_F(ConfigStorageTest, ReadingPastTheEndReturnsNothing)
{
    ASSERT_EQ(upload(storage, pattern(10), {}), WriteResult::Done);
    uint8_t buffer[REPORT_SIZE];
    EXPECT_EQ(storage.read_chunk(buffer, 63, REPORT_SIZE, true), 0u);
    EXPECT_EQ(storage.read_chunk(buffer, 10, REPORT_SIZE, true), 0u);
    EXPECT_EQ(storage.read_chunk(buffer, 0xFFFFFFFF, REPORT_SIZE, true), 0u);
}

// Empty config ----------------------------------------------------------------------------------------

TEST_F(ConfigStorageTest, InitializeEmptyWritesAValidEmptyImageToFlash)
{
    ASSERT_TRUE(storage.initialize_empty());
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image.data_size, 0u);
    EXPECT_FALSE(storage.read_flash(image, false)) << "committed before the write delay";
    run_pending_commit();
    ASSERT_TRUE(storage.read_flash(image, false));
    EXPECT_EQ(image.data_size, 0u);
    EXPECT_EQ(image.main_size, 0u);
    EXPECT_EQ(image.aux_size, 0u);
    EXPECT_EQ(read_raw_footer(flash_area()).current_profile, 0u);
}

// Auxiliary block -------------------------------------------------------------------------------------

namespace
{
struct AuxWrite
{
    std::vector<uint8_t> bytes;
    bool succeed = true;
    uint32_t seen_capacity = 0;
};

bool write_aux(uint8_t *buffer, uint32_t capacity, uint32_t &written, void *context)
{
    auto *aux = static_cast<AuxWrite *>(context);
    aux->seen_capacity = capacity;
    if (!aux->succeed || aux->bytes.size() > capacity)
    {
        return false;
    }
    if (!aux->bytes.empty())
    {
        memcpy(buffer, aux->bytes.data(), aux->bytes.size());
    }
    written = uint32_t(aux->bytes.size());
    return true;
}
}

TEST_F(ConfigStorageTest, UpdatingTheAuxBlockKeepsTheMainConfigAndCommits)
{
    const auto main = pattern(400, 70);
    ASSERT_EQ(upload(storage, main, pattern(30, 71)), WriteResult::Done);
    AuxWrite aux{pattern(55, 72)};
    ASSERT_TRUE(storage.update_auxiliary(write_aux, &aux));
    // The new block is built in the free space in front of the stored config, so it can't be bigger
    // than that
    EXPECT_EQ(aux.seen_capacity, MAX_DATA_SIZE - 430);

    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image.main_size, 400u);
    EXPECT_EQ(image.aux_size, 55u);
    EXPECT_EQ(image_bytes(image), concat(main, aux.bytes));

    run_pending_commit();
    ASSERT_TRUE(storage.read_flash(image, false));
    EXPECT_EQ(image_bytes(image), concat(main, aux.bytes));
}

TEST_F(ConfigStorageTest, AnAuxBlockCanShrinkToNothing)
{
    const auto main = pattern(100);
    ASSERT_EQ(upload(storage, main, pattern(60)), WriteResult::Done);
    AuxWrite aux{{}};
    ASSERT_TRUE(storage.update_auxiliary(write_aux, &aux));
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), main);
    EXPECT_EQ(image.aux_size, 0u);
}

TEST_F(ConfigStorageTest, AFailedAuxUpdateOfASmallConfigKeepsTheImageAndDoesNotCommit)
{
    const auto main = pattern(100);
    const auto aux = pattern(10);
    ASSERT_EQ(upload(storage, main, aux), WriteResult::Done);
    AuxWrite failing{pattern(5), false};
    EXPECT_FALSE(storage.update_auxiliary(write_aux, &failing));
    EXPECT_FALSE(storage.update_auxiliary(nullptr));
    EXPECT_EQ(EEPROM.should_commit_at, 0u);
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), concat(main, aux));
}

// update_auxiliary used to move the data to the start of the cache before calling the writer. When the
// data is over half the area that overwrote the stored copy, so a failing writer left it corrupted.
TEST_F(ConfigStorageTest, AFailedAuxUpdateOfALargeConfigKeepsTheImage)
{
    const auto main = pattern(20000, 30);
    const auto aux = pattern(10, 31);
    ASSERT_EQ(upload(storage, main, aux), WriteResult::Done);
    AuxWrite failing{pattern(5), false};
    EXPECT_FALSE(storage.update_auxiliary(write_aux, &failing));
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), concat(main, aux));

    // and one that doesn't fit at all
    AuxWrite too_big{pattern(MAX_DATA_SIZE, 32)};
    EXPECT_FALSE(storage.update_auxiliary(write_aux, &too_big));
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_EQ(image_bytes(image), concat(main, aux));
    EXPECT_EQ(EEPROM.should_commit_at, 0u);
}

TEST_F(ConfigStorageTest, ALargeConfigsAuxBlockCanGrowAndShrink)
{
    const auto main = pattern(20000, 33);
    ASSERT_EQ(upload(storage, main, pattern(100, 34)), WriteResult::Done);
    for (size_t size : {size_t(5000), size_t(10), size_t(0), size_t(MAX_DATA_SIZE - 20000)})
    {
        SCOPED_TRACE(size);
        AuxWrite aux{pattern(size, uint8_t(size))};
        ASSERT_TRUE(storage.update_auxiliary(write_aux, &aux));
        ConfigImage image;
        ASSERT_TRUE(storage.read_flash(image, true));
        EXPECT_EQ(image.main_size, 20000u);
        EXPECT_EQ(image_bytes(image), concat(main, aux.bytes));
    }
}

TEST_F(ConfigStorageTest, AnAuxUpdateDuringAnUploadIsRefused)
{
    ASSERT_EQ(upload(storage, pattern(100, 1), {}), WriteResult::Done);
    const auto data = pattern(300, 2);
    const auto info = info_report_for(data, {});
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    ASSERT_EQ(storage.write_chunk(data.data(), REPORT_SIZE, 0), WriteResult::InProgress);
    AuxWrite aux{pattern(4)};
    EXPECT_FALSE(storage.update_auxiliary(write_aux, &aux));
    EXPECT_EQ(EEPROM.should_commit_at, 0u);
    // and the upload still completes
    uint32_t start = REPORT_SIZE;
    WriteResult result = WriteResult::InProgress;
    for (size_t i = 1; i < data_reports(data).size(); i++)
    {
        result = storage.write_chunk(data_reports(data)[i].data(), REPORT_SIZE, start);
        start += REPORT_SIZE;
    }
    EXPECT_EQ(result, WriteResult::Done);
}

// An aux update (a cycle / toggle input changing, a bluetooth pairing) used to re-sign whatever the cache
// held. After an upload that failed its CRC check, that turned stale bytes into an image that passed every
// check, and saved it to flash in place of the good config.
TEST_F(ConfigStorageTest, AnAuxUpdateDoesNotResignAnInvalidImage)
{
    const auto old_main = pattern(300, 80);
    boot_with_flash_image(old_main, {});

    auto data = pattern(500, 81);
    const auto info = info_report_for(data, {});
    data[10] ^= 1;
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    uint32_t start = 0;
    for (const auto &report : data_reports(data))
    {
        storage.write_chunk(report.data(), REPORT_SIZE, start);
        start += REPORT_SIZE;
    }
    ConfigImage image;
    ASSERT_FALSE(storage.read_flash(image, true));

    AuxWrite aux{pattern(4, 82)};
    storage.update_auxiliary(write_aux, &aux);
    run_pending_commit();

    // Either the update was refused, or what it saved is still a config the tool wrote
    if (storage.read_flash(image, false))
    {
        EXPECT_EQ(std::vector<uint8_t>(image.data, image.data + image.main_size), old_main);
    }
}
