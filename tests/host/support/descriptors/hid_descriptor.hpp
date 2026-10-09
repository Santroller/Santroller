#pragma once
// A complete HID report descriptor walker for tests (HID 1.11, section 6.2.2). lib/hidparser only keeps
// the usages the host-side mapper cares about and has small fixed pools, so it can't be used to check
// every field of the descriptors the firmware advertises. This records every Input / Output / Feature
// main item, one element per report count, with its report id, bit offset, size, usage and global state.
#include <cstdint>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hid_desc
{

enum class ReportType : uint8_t
{
    Input,
    Output,
    Feature,
};

inline const char *to_string(ReportType type)
{
    switch (type)
    {
    case ReportType::Input:
        return "Input";
    case ReportType::Output:
        return "Output";
    case ReportType::Feature:
        return "Feature";
    }
    return "?";
}

// Main item data bits (HID 1.11, 6.2.2.5)
constexpr uint32_t kConstant = 1 << 0;
constexpr uint32_t kVariable = 1 << 1;
constexpr uint32_t kRelative = 1 << 2;
constexpr uint32_t kNullState = 1 << 6;

// Usage pages, and extended (page << 16 | id) usages for the ones the tests look up
constexpr uint16_t kPageDesktop = 0x01;
constexpr uint16_t kPageKeyboard = 0x07;
constexpr uint16_t kPageLed = 0x08;
constexpr uint16_t kPageButton = 0x09;
constexpr uint16_t kPageConsumer = 0x0C;
constexpr uint16_t kPageVendor = 0xFF00;

constexpr uint32_t usage(uint16_t page, uint16_t id) { return (uint32_t(page) << 16) | id; }
constexpr uint32_t desktop(uint16_t id) { return usage(kPageDesktop, id); }
constexpr uint32_t button(uint16_t n) { return usage(kPageButton, n); }
constexpr uint32_t vendor(uint16_t id) { return usage(kPageVendor, id); }

constexpr uint16_t kDesktopPointer = 0x01;
constexpr uint16_t kDesktopJoystick = 0x04;
constexpr uint16_t kDesktopGamepad = 0x05;
constexpr uint16_t kDesktopKeyboard = 0x06;
constexpr uint16_t kDesktopX = 0x30;
constexpr uint16_t kDesktopY = 0x31;
constexpr uint16_t kDesktopZ = 0x32;
constexpr uint16_t kDesktopRx = 0x33;
constexpr uint16_t kDesktopRy = 0x34;
constexpr uint16_t kDesktopRz = 0x35;
constexpr uint16_t kDesktopWheel = 0x38;
constexpr uint16_t kDesktopHat = 0x39;

// One report count's worth of a main item
struct Element
{
    ReportType type;
    uint8_t report_id;
    uint32_t bit_offset; // from the start of the report data, not counting the report id byte
    uint32_t bit_size;
    uint32_t usage;      // extended usage, 0 for padding and for array items
    uint32_t flags;
    int64_t logical_min;
    int64_t logical_max;
    int64_t physical_min;
    int64_t physical_max;
    uint32_t unit;
    uint32_t item_offset; // byte offset of the main item in the descriptor, for messages
    uint32_t item_index;  // which element of its main item this is

    bool constant() const { return flags & kConstant; }
    bool variable() const { return flags & kVariable; }
    bool relative() const { return flags & kRelative; }
    bool null_state() const { return flags & kNullState; }
};

// A whole main item, for array items (keyboard / consumer) where the usages are values, not positions
struct MainItem
{
    ReportType type;
    uint8_t report_id;
    uint32_t bit_offset;
    uint32_t report_size;
    uint32_t report_count;
    uint32_t flags;
    int64_t logical_min;
    int64_t logical_max;
    std::vector<uint32_t> usages; // explicit usages, in order
    uint32_t usage_min = 0;       // extended, from usage minimum / maximum
    uint32_t usage_max = 0;
    bool has_range = false;
    std::vector<uint32_t> string_indices;
    uint32_t item_offset;
};

struct Collection
{
    uint8_t type;
    uint32_t usage;
    uint32_t depth;
};

struct Descriptor
{
    bool ok = true;
    std::string error;
    bool uses_report_ids = false;
    std::vector<Element> elements;
    std::vector<MainItem> items;
    std::vector<Collection> collections;
    // bits used by each report, by type and report id
    std::map<std::pair<ReportType, uint8_t>, uint32_t> report_bits;

    // Every (type, id) the descriptor defines
    std::set<std::pair<ReportType, uint8_t>> reports() const
    {
        std::set<std::pair<ReportType, uint8_t>> out;
        for (const auto &r : report_bits)
            out.insert(r.first);
        return out;
    }
    bool has_report(ReportType type, uint8_t id) const { return report_bits.count({type, id}) != 0; }
    // Length of a report's data in bytes, without the report id byte
    size_t report_bytes(ReportType type, uint8_t id) const
    {
        auto it = report_bits.find({type, id});
        return it == report_bits.end() ? 0 : (it->second + 7) / 8;
    }
    // ... and on the wire, with the report id byte if the descriptor uses them
    size_t wire_bytes(ReportType type, uint8_t id) const
    {
        return report_bytes(type, id) + (uses_report_ids ? 1 : 0);
    }
    // Data (non constant) elements with this usage in one report
    std::vector<const Element *> find_all(ReportType type, uint8_t id, uint32_t usage) const
    {
        std::vector<const Element *> out;
        for (const auto &e : elements)
            if (e.type == type && e.report_id == id && e.usage == usage && !e.constant())
                out.push_back(&e);
        return out;
    }
    const Element *find(ReportType type, uint8_t id, uint32_t usage) const
    {
        auto all = find_all(type, id, usage);
        return all.size() == 1 ? all[0] : nullptr;
    }
    // The element covering a bit of a report (constant padding included)
    const Element *element_at(ReportType type, uint8_t id, uint32_t bit) const
    {
        for (const auto &e : elements)
            if (e.type == type && e.report_id == id && bit >= e.bit_offset && bit < e.bit_offset + e.bit_size)
                return &e;
        return nullptr;
    }
};

namespace detail
{
struct Globals
{
    uint16_t usage_page = 0;
    int64_t logical_min = 0;
    int64_t logical_max = 0;
    int64_t physical_min = 0;
    int64_t physical_max = 0;
    bool physical_set = false;
    uint32_t unit = 0;
    int32_t unit_exponent = 0;
    uint32_t report_size = 0;
    uint32_t report_count = 0;
    // a report count of 0 is allowed (the item has no fields), so track whether one was given
    bool report_count_set = false;
    uint8_t report_id = 0;
};

struct Locals
{
    std::vector<uint32_t> usages;
    std::optional<uint32_t> usage_min;
    std::optional<uint32_t> usage_max;
    std::vector<uint32_t> string_indices;
    bool delimiter = false;
};
} // namespace detail

// Parses a whole descriptor. On a structural error, ok is false and error says what and where;
// the elements parsed before it are kept.
inline Descriptor parse(const uint8_t *desc, size_t len)
{
    Descriptor out;
    detail::Globals g;
    detail::Locals l;
    std::vector<detail::Globals> stack;
    std::vector<Collection> open;
    std::map<std::pair<ReportType, uint8_t>, uint32_t> offsets;
    bool main_without_id = false;

    auto fail = [&](size_t at, const std::string &why) {
        std::ostringstream s;
        s << why << " (item at byte " << at << ")";
        out.ok = false;
        out.error = s.str();
        return out;
    };

    size_t pos = 0;
    while (pos < len)
    {
        const size_t at = pos;
        const uint8_t prefix = desc[pos++];
        if (prefix == 0xFE)
            return fail(at, "long items are not supported");
        const uint8_t size_code = prefix & 0x03;
        const size_t size = size_code == 3 ? 4 : size_code;
        const uint8_t type = (prefix >> 2) & 0x03;
        const uint8_t tag = prefix >> 4;
        if (pos + size > len)
            return fail(at, "item data runs past the end of the descriptor");
        uint32_t u = 0;
        for (size_t i = 0; i < size; i++)
            u |= uint32_t(desc[pos + i]) << (8 * i);
        pos += size;
        int64_t s = u;
        if (size == 1)
            s = int8_t(u);
        else if (size == 2)
            s = int16_t(u);
        else if (size == 4)
            s = int32_t(u);

        if (type == 0)
        {
            // Main item
            if (tag == 0x8 || tag == 0x9 || tag == 0xB)
            {
                const ReportType rtype = tag == 0x8 ? ReportType::Input : tag == 0x9 ? ReportType::Output : ReportType::Feature;
                if (l.usage_min.has_value() != l.usage_max.has_value())
                    return fail(at, "usage minimum without a usage maximum (or the reverse)");
                if (l.usage_min && *l.usage_min > *l.usage_max)
                    return fail(at, "usage minimum is above usage maximum");
                if (g.report_size == 0)
                    return fail(at, "main item with no report size");
                if (!g.report_count_set)
                    return fail(at, "main item with no report count");
                if (!(u & kConstant) && g.logical_min > g.logical_max)
                    return fail(at, "logical minimum is above logical maximum");
                if (g.report_id == 0)
                {
                    if (out.uses_report_ids)
                        return fail(at, "main item outside a report id once report ids are in use");
                    main_without_id = true;
                }
                uint32_t &offset = offsets[{rtype, g.report_id}];

                MainItem item;
                item.type = rtype;
                item.report_id = g.report_id;
                item.bit_offset = offset;
                item.report_size = g.report_size;
                item.report_count = g.report_count;
                item.flags = u;
                item.logical_min = g.logical_min;
                item.logical_max = g.logical_max;
                item.usages = l.usages;
                item.has_range = l.usage_min.has_value();
                item.usage_min = l.usage_min.value_or(0);
                item.usage_max = l.usage_max.value_or(0);
                item.string_indices = l.string_indices;
                item.item_offset = at;
                out.items.push_back(item);

                // Usages for a variable item: the explicit ones, then the range, and the last one repeats
                std::vector<uint32_t> usages = l.usages;
                if (l.usage_min)
                    for (uint64_t x = *l.usage_min; x <= *l.usage_max && usages.size() < g.report_count; x++)
                        usages.push_back(uint32_t(x));
                for (uint32_t i = 0; i < g.report_count; i++)
                {
                    Element e;
                    e.type = rtype;
                    e.report_id = g.report_id;
                    e.bit_offset = offset + i * g.report_size;
                    e.bit_size = g.report_size;
                    e.usage = 0;
                    if ((u & kVariable) && !usages.empty())
                        e.usage = usages[i < usages.size() ? i : usages.size() - 1];
                    e.flags = u;
                    e.logical_min = g.logical_min;
                    e.logical_max = g.logical_max;
                    e.physical_min = g.physical_set ? g.physical_min : g.logical_min;
                    e.physical_max = g.physical_set ? g.physical_max : g.logical_max;
                    e.unit = g.unit;
                    e.item_offset = at;
                    e.item_index = i;
                    out.elements.push_back(e);
                }
                offset += g.report_size * g.report_count;
                out.report_bits[{rtype, g.report_id}] = offset;
            }
            else if (tag == 0xA)
            {
                uint32_t col_usage = l.usages.empty() ? (l.usage_min ? *l.usage_min : 0) : l.usages[0];
                Collection c{uint8_t(u), col_usage, uint32_t(open.size())};
                open.push_back(c);
                out.collections.push_back(c);
            }
            else if (tag == 0xC)
            {
                if (open.empty())
                    return fail(at, "end collection without an open collection");
                open.pop_back();
            }
            else
            {
                return fail(at, "unknown main item tag");
            }
            l = detail::Locals{};
        }
        else if (type == 1)
        {
            // Global item
            switch (tag)
            {
            case 0x0:
                g.usage_page = uint16_t(u);
                break;
            case 0x1:
                g.logical_min = s;
                break;
            case 0x2:
                g.logical_max = s;
                break;
            case 0x3:
                g.physical_min = s;
                g.physical_set = true;
                break;
            case 0x4:
                g.physical_max = s;
                g.physical_set = true;
                break;
            case 0x5:
                g.unit_exponent = int32_t(s);
                break;
            case 0x6:
                g.unit = u;
                break;
            case 0x7:
                g.report_size = u;
                break;
            case 0x8:
                if (u == 0 || u > 0xFF)
                    return fail(at, "report id must be 1 - 255");
                if (main_without_id)
                    return fail(at, "report id after main items that had none");
                g.report_id = uint8_t(u);
                out.uses_report_ids = true;
                break;
            case 0x9:
                g.report_count = u;
                g.report_count_set = true;
                break;
            case 0xA:
                if (size != 0)
                    return fail(at, "push has data");
                stack.push_back(g);
                break;
            case 0xB:
                if (size != 0)
                    return fail(at, "pop has data");
                if (stack.empty())
                    return fail(at, "pop without push");
                g = stack.back();
                stack.pop_back();
                break;
            default:
                return fail(at, "unknown global item tag");
            }
        }
        else if (type == 2)
        {
            // Local item. 1 / 2 byte usages take the current usage page, 4 byte ones carry their own.
            auto extend = [&](uint32_t value) { return size == 4 ? value : (uint32_t(g.usage_page) << 16) | value; };
            switch (tag)
            {
            case 0x0:
                l.usages.push_back(extend(u));
                break;
            case 0x1:
                if (l.usage_min)
                    return fail(at, "second usage minimum before a main item");
                l.usage_min = extend(u);
                break;
            case 0x2:
                if (l.usage_max)
                    return fail(at, "second usage maximum before a main item");
                l.usage_max = extend(u);
                break;
            case 0x3:
            case 0x4:
            case 0x5:
                break; // designators
            case 0x7:
                l.string_indices.push_back(u);
                break;
            case 0x8:
            case 0x9:
                break; // string minimum / maximum
            case 0xA:
                return fail(at, "delimiters are not supported");
            default:
                return fail(at, "unknown local item tag");
            }
        }
        else
        {
            return fail(at, "reserved item type");
        }
    }
    if (!open.empty())
        return fail(len, "collection not closed at the end of the descriptor");
    if (!stack.empty())
        return fail(len, "push without pop at the end of the descriptor");
    if (l.usage_min || l.usage_max || !l.usages.empty())
        return fail(len, "local items left over after the last main item");
    if (out.collections.empty() || out.collections[0].type != 0x01)
        return fail(0, "the descriptor does not start with an application collection");
    return out;
}

inline Descriptor parse(const std::vector<uint8_t> &desc) { return parse(desc.data(), desc.size()); }
template <size_t N>
Descriptor parse(const uint8_t (&desc)[N]) { return parse(desc, N); }

// Data (non constant) elements that overlap another element of the same report
inline std::vector<std::string> overlaps(const Descriptor &d)
{
    std::vector<std::string> out;
    for (size_t i = 0; i < d.elements.size(); i++)
        for (size_t j = i + 1; j < d.elements.size(); j++)
        {
            const auto &a = d.elements[i];
            const auto &b = d.elements[j];
            if (a.type != b.type || a.report_id != b.report_id)
                continue;
            if (a.bit_offset < b.bit_offset + b.bit_size && b.bit_offset < a.bit_offset + a.bit_size)
            {
                std::ostringstream s;
                s << to_string(a.type) << " report " << int(a.report_id) << " bits " << a.bit_offset << " and " << b.bit_offset;
                out.push_back(s.str());
            }
        }
    return out;
}

} // namespace hid_desc
