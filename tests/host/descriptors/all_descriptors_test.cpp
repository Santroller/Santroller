#include <gtest/gtest.h>
#include <string>
#include "firmware_descriptors.hpp"
#include "hid_descriptor.hpp"
#include "hidparser.h"
#include "source_check.hpp"

// Checks every HID report descriptor the firmware advertises gets, whatever device it belongs to.

using namespace hid_desc;

namespace
{
class AllDescriptors : public ::testing::TestWithParam<NamedDescriptor>
{
};

std::string descriptor_name(const ::testing::TestParamInfo<NamedDescriptor> &info) { return info.param.name; }
} // namespace

TEST_P(AllDescriptors, ParsesCleanly)
{
    auto d = parse(GetParam().bytes);
    // balanced collections and push / pop, no truncated or unknown items, sane report ids and sizes
    EXPECT_TRUE(d.ok) << d.error;
    EXPECT_FALSE(d.elements.empty());
}

TEST_P(AllDescriptors, NoElementsOverlap)
{
    auto d = parse(GetParam().bytes);
    ASSERT_TRUE(d.ok) << d.error;
    for (const auto &o : overlaps(d))
        ADD_FAILURE() << o;
}

// An input the host interprets must have a logical range its bits can hold: a negative minimum means a
// two's complement field (HID 1.11 6.2.2.7), otherwise the field is unsigned.
TEST_P(AllDescriptors, InputLogicalRangesFitTheirSize)
{
    auto d = parse(GetParam().bytes);
    ASSERT_TRUE(d.ok) << d.error;
    for (const auto &e : d.elements)
    {
        if (e.type != ReportType::Input || e.constant() || e.bit_size >= 32)
            continue;
        SCOPED_TRACE(::testing::Message() << "report " << int(e.report_id) << " bit " << e.bit_offset << " usage 0x" << std::hex << e.usage);
        if (e.logical_min < 0)
        {
            EXPECT_GE(e.logical_min, -(int64_t(1) << (e.bit_size - 1)));
            EXPECT_LE(e.logical_max, (int64_t(1) << (e.bit_size - 1)) - 1);
        }
        else
        {
            EXPECT_LE(e.logical_max, (int64_t(1) << e.bit_size) - 1);
        }
    }
}

// Input and output reports go over the interrupt endpoints, whose buffers (epin_buf / epout_buf) and
// max packet size are CFG_TUD_HID_EP_BUFSIZE, report id included.
TEST_P(AllDescriptors, UsbInterruptReportsFitTheEndpoint)
{
    if (GetParam().transport != Transport::Usb)
        GTEST_SKIP() << "bluetooth descriptor";
    auto d = parse(GetParam().bytes);
    ASSERT_TRUE(d.ok) << d.error;
    for (const auto &[report, bits] : d.report_bits)
    {
        if (report.first == ReportType::Feature)
            continue;
        EXPECT_LE(d.wire_bytes(report.first, report.second), size_t(CFG_TUD_HID_EP_BUFSIZE))
            << to_string(report.first) << " report " << int(report.second);
    }
}

// The test builds most descriptors from the hid_reports.h macros; make sure the device still does too
TEST_P(AllDescriptors, DeviceSourceUsesTheSameMacro)
{
    if (GetParam().source.empty())
        GTEST_SKIP() << "built from the real source";
    const std::string source = firmware_source(GetParam().source);
    ASSERT_FALSE(source.empty()) << "can't read " << GetParam().source;
    EXPECT_NE(source.find(strip_spaces(GetParam().snippet)), std::string::npos)
        << GetParam().source << " no longer contains " << GetParam().snippet;
}

// lib/hidparser is the firmware's own host side parser (a Santroller plugged into a Santroller). It only
// keeps some usages, but every item it keeps must agree with the walker.
TEST_P(AllDescriptors, HidParserAgreesWithTheWalker)
{
    const auto &bytes = GetParam().bytes;
    auto d = parse(bytes);
    ASSERT_TRUE(d.ok) << d.error;
    HID_ReportInfo_t *info = nullptr;
    uint8_t result = USB_ProcessHIDReport(bytes.data(), bytes.size(), &info);
    // hidparser's pools are shared, so give them back even when an assertion fails
    struct Free
    {
        HID_ReportInfo_t *&info;
        ~Free()
        {
            if (info)
                USB_FreeReportInfo(info);
        }
    } free_info{info};
    if (result == HID_PARSE_NoUnfilteredReportItems)
        GTEST_SKIP() << "hidparser keeps nothing from this descriptor";
    ASSERT_EQ(result, HID_PARSE_Successful);
    EXPECT_EQ(info->UsingReportIDs, d.uses_report_ids);
    int compared = 0;
    for (HID_ReportItem_t *item = info->FirstReportItem; item; item = item->Next)
    {
        const ReportType type = item->ItemType == HID_REPORT_ITEM_In    ? ReportType::Input
                                : item->ItemType == HID_REPORT_ITEM_Out ? ReportType::Output
                                                                        : ReportType::Feature;
        const Element *e = d.element_at(type, item->ReportID, item->BitOffset);
        ASSERT_NE(e, nullptr) << "hidparser item at bit " << item->BitOffset << " of report " << int(item->ReportID);
        EXPECT_EQ(e->bit_offset, item->BitOffset);
        EXPECT_EQ(e->bit_size, item->Attributes.BitSize);
        // hidparser doesn't repeat the last usage for the rest of a count, so only compare explicit ones
        if (item->Attributes.Usage.Usage)
        {
            EXPECT_EQ(e->usage, usage(item->Attributes.Usage.Page, item->Attributes.Usage.Usage))
                << "report " << int(item->ReportID) << " bit " << item->BitOffset;
        }
        compared++;
    }
    EXPECT_GT(compared, 0);
}

INSTANTIATE_TEST_SUITE_P(Firmware, AllDescriptors, ::testing::ValuesIn(all_descriptors()), descriptor_name);
