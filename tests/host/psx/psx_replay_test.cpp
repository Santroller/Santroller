// Replays PS2 controller bus captures (data/ps2_captures.csv) through the emulated controller's command
// state machine. Each column pair in the capture is what a console sent (command) and what a real
// controller answered (data), one byte per row as "<capture index>: <hex byte>".
//
// Every transaction's header byte encodes whether the controller is in config mode, digital or analog,
// and how long its report is, so it checks the state machine against the real controller. The config
// mode answers (0x41 - 0x4F) are only compared for controllers the emulation imitates, other models
// identify themselves differently.
#include <gtest/gtest.h>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "enums.pb.h"
#include "psx_spi_protocol.h"

namespace
{
struct CapturedByte
{
    unsigned capture_id;
    uint8_t tx;
    uint8_t rx;
};

struct Capture
{
    const char *name;
    unsigned command_column;
    unsigned data_column;
    SubType emulated;
    // whether the emulation answers config commands exactly like this controller
    bool compare_config;
};

const Capture kCaptures[] = {
    {"GuitarHero", 4, 5, SubType_GuitarHeroGuitar, true},
    {"WirelessDualShock2", 6, 7, SubType_Gamepad, true},
    {"DualShock", 0, 1, SubType_Gamepad, false},
    {"ChinaDualShock", 8, 9, SubType_Gamepad, false},
    {"DualShockWithVibration", 11, 12, SubType_Gamepad, false},
};

constexpr size_t kMaxTransaction = 21;

std::vector<std::string> split_csv(const std::string &line)
{
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;
    while (std::getline(stream, field, ','))
    {
        fields.push_back(field);
    }
    return fields;
}

// "<id>: <hex>", returns false for headers, notes and empty cells
bool parse_cell(const std::string &cell, unsigned &id, uint8_t &byte)
{
    unsigned value;
    if (sscanf(cell.c_str(), "%u: %2x", &id, &value) != 2)
    {
        return false;
    }
    byte = value;
    return true;
}

std::vector<CapturedByte> load_capture(const Capture &capture)
{
    std::ifstream file(PSX_CAPTURE_CSV);
    EXPECT_TRUE(file.is_open()) << PSX_CAPTURE_CSV;
    std::vector<CapturedByte> stream;
    std::string line;
    while (std::getline(file, line))
    {
        auto fields = split_csv(line);
        if (fields.size() <= std::max(capture.command_column, capture.data_column))
        {
            continue;
        }
        unsigned command_id, data_id;
        uint8_t command, data;
        if (parse_cell(fields[capture.command_column], command_id, command) &&
            parse_cell(fields[capture.data_column], data_id, data) && command_id == data_id)
        {
            stream.push_back({command_id, command, data});
        }
    }
    return stream;
}

bool is_ps2_command(uint8_t b)
{
    switch (b)
    {
    case 0x40: case 0x41: case 0x42: case 0x43:
    case 0x44: case 0x45: case 0x46: case 0x47:
    case 0x4C: case 0x4D: case 0x4F:
        return true;
    default:
        return false;
    }
}

// a transaction starts with the 0x01 address byte followed by a command
bool starts_transaction(const std::vector<CapturedByte> &stream, size_t i)
{
    return i + 1 < stream.size() && stream[i].tx == 0x01 && is_ps2_command(stream[i + 1].tx);
}

std::string hex(const uint8_t *bytes, size_t len)
{
    std::string out;
    char buf[4];
    for (size_t i = 0; i < len; i++)
    {
        snprintf(buf, sizeof(buf), i ? " %02X" : "%02X", bytes[i]);
        out += buf;
    }
    return out;
}

class PsxReplay : public ::testing::TestWithParam<Capture>
{
};
} // namespace

TEST_P(PsxReplay, EmulationAnswersLikeTheCapturedController)
{
    const Capture &capture = GetParam();
    auto stream = load_capture(capture);
    ASSERT_FALSE(stream.empty());

    psx_spi_protocol_state_t s;
    PSX_SPI_PROTOCOL_INIT(&s, capture.emulated);

    // The header and config answers for a transaction are queued before its command is seen
    uint8_t header = PSX_SPI_RESPONSE_HEADER(&s, psx_spi_current_report_len(&s));
    uint8_t config[0x10][6];
    memcpy(config, s.config_responses, sizeof(config));

    unsigned transactions = 0;
    size_t i = 0;
    while (i < stream.size())
    {
        if (!starts_transaction(stream, i))
        {
            i++;
            continue;
        }
        size_t next = i + 1;
        while (next < stream.size() && !starts_transaction(stream, next))
        {
            next++;
        }
        size_t len = next - i;
        ASSERT_LE(len, kMaxTransaction) << "capture id " << stream[i].capture_id;

        uint8_t tx[kMaxTransaction] = {0};
        uint8_t rx[kMaxTransaction] = {0};
        for (size_t k = 0; k < len; k++)
        {
            tx[k] = stream[i + k].tx;
            rx[k] = stream[i + k].rx;
        }
        transactions++;
        SCOPED_TRACE(testing::Message() << "capture id " << stream[i].capture_id << " TX " << hex(tx, len)
                                        << " RX " << hex(rx, len));

        EXPECT_EQ(rx[1], header);

        uint8_t reg = tx[1] - 0x40;
        if (capture.compare_config && s.configMode && len >= 9 && reg < 0x10)
        {
            uint8_t expected[6];
            memcpy(expected, config[reg], 6);
            if (reg == 0 && tx[3] < sizeof(s.button_attr))
            {
                // 0x40 answers with the attribute of the button it asks about
                expected[2] = s.button_attr[tx[3]];
            }
            EXPECT_EQ(hex(rx + 3, 6), hex(expected, 6)) << "config answer to " << hex(tx + 1, 1);
        }

        PSX_SPI_PROCESS_COMMAND(&s, tx);
        header = PSX_SPI_RESPONSE_HEADER(&s, psx_spi_current_report_len(&s));
        memcpy(config, s.config_responses, sizeof(config));
        i = next;
    }

    // each capture covers a game setting the controller up, not just polling
    EXPECT_GT(transactions, 50u);
}

INSTANTIATE_TEST_SUITE_P(Captures, PsxReplay, ::testing::ValuesIn(kCaptures),
                         [](const ::testing::TestParamInfo<Capture> &info)
                         { return std::string(info.param.name); });
