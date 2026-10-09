#include <gtest/gtest.h>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include "firmware_descriptors.hpp"
#include "hid_descriptor.hpp"
#include "source_check.hpp"

// Over BLE (HID over GATT) a report can only be sent or received if the GATT database has a Report
// characteristic whose Report Reference (id, type) names it: btstack's hids_device looks the report up
// by id and type (hids_device_send_input_report_for_id), and sizes it from the report map
// (btstack_hid_get_report_size_for_id). The database is src/emulation/bt/bt.gatt, which imports
// btstack's hids.gatt for the first HID service. The firmware compiles the checked in header generated from
// it (include/emulation/bt/bt_profile.h, made by lib/btstack/tool/compile_gatt.py), so the tests read the
// header and check that it is in step with bt.gatt.

using namespace hid_desc;

namespace
{
struct ReportRef
{
    uint8_t id;
    uint8_t type; // 1 input, 2 output, 3 feature (HOGP Report Reference)
    bool operator==(const ReportRef &o) const { return id == o.id && type == o.type; }
};

struct Service
{
    bool hid = false;
    std::vector<ReportRef> reports;
};

std::vector<std::string> read_lines(const std::string &path)
{
    std::ifstream file(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
        lines.push_back(line);
    return lines;
}

// The services of bt.gatt in order, with hids.gatt expanded in place. compile_gatt.py writes Report
// Reference values as hex bytes.
std::vector<Service> gatt_services()
{
    std::vector<Service> services;
    std::vector<std::string> lines = read_lines(std::string(SANTROLLER_ROOT) + "/src/emulation/bt/bt.gatt");
    EXPECT_FALSE(lines.empty()) << "can't read bt.gatt";
    std::vector<std::string> expanded;
    for (const auto &line : lines)
    {
        if (line.rfind("#import <hids.gatt>", 0) == 0)
        {
            auto hids = read_lines(std::string(SANTROLLER_ROOT) + "/lib/btstack/src/ble/gatt-service/hids.gatt");
            EXPECT_FALSE(hids.empty()) << "can't read hids.gatt";
            expanded.insert(expanded.end(), hids.begin(), hids.end());
        }
        else if (line.rfind("#import", 0) == 0)
        {
            // another standard service (battery, device information): starts a service with no reports
            expanded.push_back("PRIMARY_SERVICE, IMPORTED");
        }
        else
        {
            expanded.push_back(line);
        }
    }
    const std::regex reference(R"(^REPORT_REFERENCE\s*,\s*READ\s*,\s*([0-9A-Fa-f]+)\s*,\s*([0-9A-Fa-f]+))");
    for (const auto &line : expanded)
    {
        std::smatch m;
        if (line.rfind("PRIMARY_SERVICE", 0) == 0)
        {
            services.push_back({line.find("ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE") != std::string::npos, {}});
        }
        else if (std::regex_search(line, m, reference))
        {
            EXPECT_FALSE(services.empty());
            services.back().reports.push_back({uint8_t(std::stoul(m[1], nullptr, 16)), uint8_t(std::stoul(m[2], nullptr, 16))});
        }
    }
    return services;
}

// The same from the generated header, whose comments list each attribute as compile_gatt.py saw it
// ("// 0x0021 REPORT_REFERENCE-READ-1-1")
std::vector<Service> header_services()
{
    std::vector<Service> services;
    auto lines = read_lines(std::string(SANTROLLER_ROOT) + "/include/emulation/bt/bt_profile.h");
    EXPECT_FALSE(lines.empty()) << "can't read bt_profile.h";
    const std::regex service(R"(^\s*// 0x[0-9a-fA-F]+ PRIMARY_SERVICE-(\S+))");
    const std::regex reference(R"(^\s*// 0x[0-9a-fA-F]+ REPORT_REFERENCE-READ-([0-9A-Fa-f]+)-([0-9A-Fa-f]+))");
    for (const auto &line : lines)
    {
        std::smatch m;
        if (std::regex_search(line, m, service))
        {
            services.push_back({m[1].str() == "ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE", {}});
        }
        else if (std::regex_search(line, m, reference))
        {
            EXPECT_FALSE(services.empty());
            services.back().reports.push_back({uint8_t(std::stoul(m[1], nullptr, 16)), uint8_t(std::stoul(m[2], nullptr, 16))});
        }
    }
    return services;
}

std::vector<Service> only_hid(const std::vector<Service> &services)
{
    std::vector<Service> out;
    for (const auto &s : services)
        if (s.hid)
            out.push_back(s);
    return out;
}

// What the firmware is built with
std::vector<Service> hid_services() { return only_hid(header_services()); }

bool has_ref(const Service &s, uint8_t id, uint8_t type)
{
    for (const auto &r : s.reports)
        if (r.id == id && r.type == type)
            return true;
    return false;
}

ReportType report_type(uint8_t type)
{
    return type == 1 ? ReportType::Input : type == 2 ? ReportType::Output : ReportType::Feature;
}
} // namespace

// bt_profile.h has to be regenerated whenever bt.gatt changes:
//   lib/btstack/tool/compile_gatt.py src/emulation/bt/bt.gatt include/emulation/bt/bt_profile.h
TEST(BtGatt, GeneratedHeaderMatchesTheGattFile)
{
    auto from_gatt = only_hid(gatt_services());
    auto from_header = hid_services();
    ASSERT_EQ(from_gatt.size(), from_header.size());
    for (size_t i = 0; i < from_gatt.size(); i++)
    {
        ASSERT_EQ(from_gatt[i].reports.size(), from_header[i].reports.size()) << "HID service " << i;
        for (size_t j = 0; j < from_gatt[i].reports.size(); j++)
            EXPECT_TRUE(from_gatt[i].reports[j] == from_header[i].reports[j]) << "HID service " << i << " report " << j;
    }
}

TEST(BtGatt, HasTheGamepadAndConfigHidServices)
{
    auto services = hid_services();
    ASSERT_EQ(services.size(), 2u);
    EXPECT_FALSE(services[0].reports.empty());
    EXPECT_FALSE(services[1].reports.empty());
}

// BTGamepadDevice sends the gamepad report with hids_device_send_input_report, which uses the first input
// Report characteristic, and the capabilities report by id (bt_gamepad.cpp send_report)
TEST(BtGatt, GamepadReportsHaveCharacteristics)
{
    auto services = hid_services();
    ASSERT_GE(services.size(), 1u);
    const Service &gamepad = services[0];
    const ReportRef *first_input = nullptr;
    for (const auto &r : gamepad.reports)
        if (r.type == 1 && !first_input)
            first_input = &r;
    ASSERT_NE(first_input, nullptr);
    EXPECT_EQ(first_input->id, uint8_t(ReportIdGamepad));
    EXPECT_TRUE(has_ref(gamepad, ReportIdSantrollerCapabilities, 1));
    // rumble / commands and the capabilities request come in as output reports
    EXPECT_TRUE(has_ref(gamepad, ReportIdGamepad, 2));
    EXPECT_TRUE(has_ref(gamepad, ReportIdSantrollerCapabilities, 2));
}

// Every report the gamepad service references must be in the report map, or btstack sizes it as 0
// (apart from the keyboard profile's mouse and media key inputs, which a gamepad never sends)
TEST(BtGatt, GamepadCharacteristicsAreInTheReportMap)
{
    auto services = hid_services();
    ASSERT_GE(services.size(), 1u);
    for (const auto &desc : {parse(desc_hid_report_hat), parse(desc_hid_report_buttons)})
    {
        ASSERT_TRUE(desc.ok) << desc.error;
        for (const auto &r : services[0].reports)
        {
            if (r.type == 1 && (r.id == MOUSE_REPORT_ID || r.id == CONSUMER_REPORT_ID))
                continue;
            EXPECT_GT(desc.report_bytes(report_type(r.type), r.id), 0u) << "report " << int(r.id) << " type " << int(r.type);
        }
    }
}

// KeyboardMouse profiles share the first HID service (bt_gamepad.cpp initialises hids_device with
// desc_hid_report_keyboard), and BTGamepadDevice::process sends the keyboard, media key and mouse
// reports with hids_device_send_input_report_for_id(KEYBOARD_REPORT_ID / CONSUMER_REPORT_ID /
// MOUSE_REPORT_ID). The lock lights come back as output report KEYBOARD_REPORT_ID.
TEST(BtGatt, KeyboardReportHasCharacteristics)
{
    auto services = hid_services();
    ASSERT_GE(services.size(), 1u);
    EXPECT_TRUE(has_ref(services[0], KEYBOARD_REPORT_ID, 1));
    EXPECT_TRUE(has_ref(services[0], KEYBOARD_REPORT_ID, 2));
}

// Mouse (2) and media keys (3) need their own input Report characteristics, or
// hids_device_send_input_report_for_id finds no report and they never go out. Every report the first
// service references has to fit bt_gamepad.cpp's report_storage (6 entries), or hids_device stops there.
TEST(BtGatt, MouseAndMediaKeyReportsHaveCharacteristics)
{
    auto services = hid_services();
    ASSERT_GE(services.size(), 1u);
    EXPECT_TRUE(has_ref(services[0], MOUSE_REPORT_ID, 1));
    EXPECT_TRUE(has_ref(services[0], CONSUMER_REPORT_ID, 1));
    EXPECT_LE(services[0].reports.size(), 6u);
}

// Every report the keyboard service references beyond the shared capabilities ones is in its report map
TEST(BtGatt, KeyboardCharacteristicsAreInTheReportMap)
{
    auto services = hid_services();
    ASSERT_GE(services.size(), 1u);
    auto d = parse(desc_hid_report_keyboard, desc_hid_report_keyboard_len);
    ASSERT_TRUE(d.ok) << d.error;
    for (const auto &r : services[0].reports)
    {
        if (r.id == ReportIdSantrollerCapabilities)
            continue;
        EXPECT_GT(d.report_bytes(report_type(r.type), r.id), 0u) << "report " << int(r.id) << " type " << int(r.type);
    }
}

// The second HID service carries the config collection (bt_config_service.cpp sizes each report from
// hid_config_report_descriptor(), capped at MAX_REPORT_SIZE 64)
TEST(BtGatt, ConfigCharacteristicsAreInTheConfigDescriptor)
{
    auto services = hid_services();
    ASSERT_EQ(services.size(), 2u);
    auto d = parse(kConfig);
    ASSERT_TRUE(d.ok) << d.error;
    for (const auto &r : services[1].reports)
    {
        SCOPED_TRACE(::testing::Message() << "report 0x" << std::hex << int(r.id) << " type " << int(r.type));
        EXPECT_GT(d.report_bytes(report_type(r.type), r.id), 0u);
        EXPECT_LE(d.report_bytes(report_type(r.type), r.id), 64u);
    }
    EXPECT_TRUE(has_ref(services[1], ReportIdConfig, 1)) << "config events";
    EXPECT_TRUE(has_ref(services[1], ReportIdUploadFirmware, 2)) << "firmware upload";
}

// The Web Bluetooth config service has one characteristic per feature report id (s_web_report_ids in
// bt_config_service.cpp), sized from the config descriptor's feature reports
TEST(BtGatt, WebConfigReportIdsAreConfigFeatures)
{
    const std::string source = firmware_source("src/emulation/bt/bt_config_service.cpp");
    std::smatch m;
    ASSERT_TRUE(std::regex_search(source, m, std::regex(R"(s_web_report_ids\[\]=\{([^}]*)\})")));
    auto d = parse(kConfig);
    ASSERT_TRUE(d.ok) << d.error;
    std::stringstream ids(m[1].str());
    std::string id;
    int count = 0;
    while (std::getline(ids, id, ','))
    {
        const uint8_t report = uint8_t(std::stoul(id, nullptr, 0));
        EXPECT_GT(d.report_bytes(ReportType::Feature, report), 0u) << "report 0x" << std::hex << int(report);
        count++;
    }
    EXPECT_GT(count, 0);
}
