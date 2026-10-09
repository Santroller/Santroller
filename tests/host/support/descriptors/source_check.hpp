#pragma once
// The device descriptors are mostly static arrays inside device .cpp files that can't be built on the
// host, so the tests build the same arrays from the hid_reports.h macros. These helpers check that a
// device source still instantiates the macro the test builds, so the pairing can't silently drift.
#include <fstream>
#include <sstream>
#include <string>

namespace hid_desc
{
inline std::string strip_spaces(const std::string &in)
{
    std::string out;
    for (char c : in)
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
            out += c;
    return out;
}

// Contents of a file under the firmware root, without whitespace
inline std::string firmware_source(const std::string &relative)
{
    std::ifstream file(std::string(SANTROLLER_ROOT) + "/" + relative);
    std::stringstream s;
    s << file.rdbuf();
    return strip_spaces(s.str());
}

inline bool source_contains(const std::string &relative, const std::string &snippet)
{
    return firmware_source(relative).find(strip_spaces(snippet)) != std::string::npos;
}
} // namespace hid_desc
