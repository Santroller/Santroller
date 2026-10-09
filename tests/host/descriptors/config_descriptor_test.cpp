#include <gtest/gtest.h>
#include <regex>
#include "firmware_descriptors.hpp"
#include "hid_descriptor.hpp"
#include "source_check.hpp"

// The configurator interface (HIDConfigDevice, src/emulation/usb/hid_config_device.cpp; also the report
// map of the second BLE HID service). Every report is a vendor byte array, so the checks are about ids
// and lengths: feature reports go through ctrl_buf (CFG_TUD_HID_EP_BUFSIZE, report id included) and
// SET_REPORT rejects anything longer (hid_device.cpp).
//
// Pairing: handle_get_report() writes the report id into buffer[0] and returns the length including it;
// handle_set_report() skips the id byte.

using namespace hid_desc;

namespace
{
Descriptor config()
{
    auto d = parse(kConfig);
    EXPECT_TRUE(d.ok) << d.error;
    return d;
}
} // namespace

TEST(ConfigDescriptor, ReportsFitTheControlBuffer)
{
    auto d = config();
    for (const auto &[report, bits] : d.report_bits)
        EXPECT_LE(d.wire_bytes(report.first, report.second), size_t(CFG_TUD_HID_EP_BUFSIZE))
            << to_string(report.first) << " 0x" << std::hex << int(report.second);
}

TEST(ConfigDescriptor, ReportIds)
{
    auto d = config();
    // config events go out as input report 0x22, firmware uploads come in as output report 0x31
    EXPECT_EQ(d.wire_bytes(ReportType::Input, ReportIdConfig), 64u);
    EXPECT_EQ(d.wire_bytes(ReportType::Output, ReportIdUploadFirmware), 64u);
    for (uint8_t id : {ReportIdConfig, ReportIdConfigInfo, ReportIdLoaded, ReportIdKeepalive, ReportIdBootloader,
                       ReportIdCommand, ReportIdGetActiveProfiles, ReportIdGetVersion, ReportIdUpdateFirmware,
                       ReportIdUploadFirmware, ReportIdGetType})
        EXPECT_TRUE(d.has_report(ReportType::Feature, id)) << "0x" << std::hex << int(id);
    // GetActiveProfiles answers with a full 64 byte report
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdGetActiveProfiles), 64u);
}

// handle_get_report answers ReportIdConfigInfoSaved (0x33) with the same copy_config_info as
// ReportIdConfigInfo, and the configurator reads it (fetchConfigData(saved = true) in SettingsContext.ts),
// so it has to be declared or hosts that only allow declared ids (Windows' HID class driver) refuse it
TEST(ConfigDescriptor, ConfigInfoSavedIsDeclared)
{
    auto d = config();
    EXPECT_TRUE(d.has_report(ReportType::Feature, ReportIdConfigInfoSaved));
    EXPECT_EQ(d.report_bytes(ReportType::Feature, ReportIdConfigInfoSaved), d.report_bytes(ReportType::Feature, ReportIdConfigInfo));
}

// GetVersion / GetType are declared as sizeof(string) + 1 bytes after the id, and handle_get_report
// returns exactly that: the id, the string with its NUL, and one zero byte (sizeof(string) + 2 in all)
TEST(ConfigDescriptor, VersionAndTypeLengthsMatchGetReport)
{
    auto d = config();
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdGetVersion), sizeof(kConfigVersion) + 2);
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdGetType), sizeof(kConfigType) + 2);
    const std::string source = firmware_source("src/emulation/usb/hid_config_device.cpp");
    EXPECT_NE(source.find("buffer[0]=report_id;memcpy(buffer+1,version,sizeof(version));buffer[1+sizeof(version)]=0;returnsizeof(version)+2;"),
              std::string::npos);
    EXPECT_NE(source.find("buffer[0]=report_id;memcpy(buffer+1,type,sizeof(type));buffer[1+sizeof(type)]=0;returnsizeof(type)+2;"),
              std::string::npos);
}
