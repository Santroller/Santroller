#pragma once
// Helpers for the GIP tests: an independent GIP packet encoder written from the MS-GIPUSB spec
// (and checked against medusalix/xone's bus/protocol.c), a builder for binary GIP metadata
// blobs, and a recorder for the gip_device_interface_t callbacks.
//
// References used throughout:
//   [MS-GIPUSB] Gaming Input Protocol (GIP) USB Extension, v20240916
//     2.2.10   Message header (Tables 12-15), 2.2.10.4 MTUs (Table 16)
//     3.1.5.1  Reliable message acknowledgement
//     3.1.5.2  Reliable large message transmission (Tables 21-25)
//     2.2.2    Metadata, including the compiled gamepad metadata example
//   xone:   https://github.com/medusalix/xone bus/protocol.c (gip_encode_header,
//           gip_acknowledge_pkt, gip_send_pkt / gip_send_remaining_chunks)
//   xpad:   linux drivers/input/joystick/xpad.c (xboxone_* init packets, xpadone_process_packet)
//   PlasticBand: https://github.com/TheNathannator/PlasticBand Docs/Instruments/*/Xbox One.md
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include "gip_device.h"
#include "gip_device_interface.h"
#include "gip_packet_handler.h"

namespace gip_test
{
using Bytes = std::vector<uint8_t>;

// [MS-GIPUSB] Table 14: GIP Flags
constexpr uint8_t FLAG_FRAGMENT = 0x80;  // bit 7
constexpr uint8_t FLAG_INIT_FRAG = 0x40; // bit 6
constexpr uint8_t FLAG_SYSTEM = 0x20;    // bit 5
constexpr uint8_t FLAG_ACME = 0x10;      // bit 4, "ACknowledge ME"

// [MS-GIPUSB] 3.1.5.2: fragments carry at most 58 bytes on the 64 byte MTU classes (xone's
// GIP_PKT_MAX_LENGTH is the same 58)
constexpr size_t FRAGMENT_PAYLOAD = 58;

// [MS-GIPUSB] 2.2.10.4: "Payload Length can become a multiple byte field by setting bit 7, which
// indicates the next byte represents the next higher order 7 bits"
inline Bytes varint(uint32_t value)
{
    Bytes out;
    do
    {
        uint8_t b = value & 0x7F;
        value >>= 7;
        if (value)
            b |= 0x80;
        out.push_back(b);
    } while (value);
    return out;
}

// A GIP header the way MS-GIPUSB 3.1.5.2 / xone gip_encode_header build it: type, flags,
// sequence, varint payload length, then the varint TLO (total length / offset) on fragments.
// A header that would come out an odd length gets its payload length padded to an extra byte
// ("one of these length fields ... becomes a two-byte field to bring the GIP header size to an
// even 6 bytes"; xone pads the length field the same way).
inline Bytes header(uint8_t type, uint8_t flags, uint8_t sequence, uint32_t length, bool has_tlo = false, uint32_t tlo = 0)
{
    Bytes out = {type, flags, sequence};
    Bytes len = varint(length);
    Bytes tlo_bytes = has_tlo ? varint(tlo) : Bytes{};
    if ((3 + len.size() + tlo_bytes.size()) % 2)
    {
        len.back() |= 0x80;
        len.push_back(0x00);
    }
    out.insert(out.end(), len.begin(), len.end());
    out.insert(out.end(), tlo_bytes.begin(), tlo_bytes.end());
    return out;
}

// A complete single packet message
inline Bytes packet(uint8_t type, uint8_t flags, uint8_t sequence, const Bytes &payload = {})
{
    Bytes out = header(type, flags, sequence, (uint32_t)payload.size());
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

inline Bytes concat(std::initializer_list<Bytes> parts)
{
    Bytes out;
    for (auto &p : parts)
        out.insert(out.end(), p.begin(), p.end());
    return out;
}

// A message split into fragments the way a device sends it ([MS-GIPUSB] 3.1.5.2, xone
// gip_send_pkt + gip_send_remaining_chunks): every fragment has the Fragment flag and the same
// sequence; the first has InitFrag + ACME and the total length as TLO, later ones the offset,
// and the last data fragment has ACME. The terminating empty fragment (sent once the receiver
// has acked everything) is returned separately by fragment_complete().
inline std::vector<Bytes> fragments(uint8_t type, uint8_t sequence, const Bytes &message, uint8_t extra_flags = FLAG_SYSTEM,
                                    size_t mtu = FRAGMENT_PAYLOAD)
{
    std::vector<Bytes> out;
    size_t offset = 0;
    while (offset < message.size())
    {
        size_t len = std::min(mtu, message.size() - offset);
        bool first = offset == 0;
        bool last = offset + len == message.size();
        uint8_t flags = FLAG_FRAGMENT | extra_flags;
        if (first)
            flags |= FLAG_INIT_FRAG | FLAG_ACME;
        if (last)
            flags |= FLAG_ACME;
        Bytes pkt = header(type, flags, sequence, (uint32_t)len, true, first ? (uint32_t)message.size() : (uint32_t)offset);
        pkt.insert(pkt.end(), message.begin() + offset, message.begin() + offset + len);
        out.push_back(pkt);
        offset += len;
    }
    return out;
}

// [MS-GIPUSB] Table 26 "Metadata Complete" / "Security Data Complete": flags 0xA0, payload length
// 0, TLO = total length (xone: "empty chunk signals the completion of the transfer")
inline Bytes fragment_complete(uint8_t type, uint8_t sequence, uint32_t total, uint8_t extra_flags = FLAG_SYSTEM)
{
    return header(type, FLAG_FRAGMENT | extra_flags, sequence, 0, true, total);
}

// Decoded GIP header, parsed independently of XGIPProtocol (varint fields per 2.2.10.4)
struct Header
{
    uint8_t type = 0;
    uint8_t flags = 0;
    uint8_t sequence = 0;
    uint32_t length = 0;
    uint32_t tlo = 0;
    size_t header_size = 0;
};

inline bool decode_varint(const Bytes &data, size_t &pos, uint32_t &out)
{
    out = 0;
    for (int i = 0; i < 4; i++)
    {
        if (pos >= data.size())
            return false;
        uint8_t b = data[pos++];
        out |= (uint32_t)(b & 0x7F) << (7 * i);
        if (!(b & 0x80))
            return true;
    }
    return false;
}

inline Header decode(const Bytes &data)
{
    Header h;
    if (data.size() < 4)
        return h;
    h.type = data[0];
    h.flags = data[1];
    h.sequence = data[2];
    size_t pos = 3;
    decode_varint(data, pos, h.length);
    if (h.flags & FLAG_FRAGMENT)
        decode_varint(data, pos, h.tlo);
    h.header_size = pos;
    return h;
}

// The payload of a packet, cut short if the packet is
inline Bytes payload_of(const Bytes &data)
{
    Header h = decode(data);
    size_t start = std::min(h.header_size, data.size());
    size_t end = std::min(h.header_size + h.length, data.size());
    return Bytes(data.begin() + start, data.begin() + end);
}

// An ACK for a received message or fragment, as xone gip_acknowledge_pkt and xpad
// xpadone_ack_mode_report build it: Protocol Control (0x01), system flag, the acked sequence,
// 9 payload bytes: 0x00, acked type, 0x20, bytes received so far (le16), 2 padding, remaining (le16)
inline Bytes ack(uint8_t type, uint8_t sequence, uint16_t received, uint16_t remaining)
{
    return {0x01, 0x20, sequence, 0x09, 0x00, type, 0x20,
            (uint8_t)(received & 0xFF), (uint8_t)(received >> 8), 0x00, 0x00,
            (uint8_t)(remaining & 0xFF), (uint8_t)(remaining >> 8)};
}

inline Bytes message_bytes(size_t n, uint8_t seed = 0)
{
    Bytes out(n);
    for (size_t i = 0; i < n; i++)
        out[i] = (uint8_t)(seed + i * 7 + 1);
    return out;
}

// --- GIP metadata ([MS-GIPUSB] 2.2.2) ---

// GUIDs as they appear in metadata: the usual Windows mixed-endian GUID layout
inline Bytes guid(uint32_t d1, uint16_t d2, uint16_t d3, std::initializer_list<uint8_t> d4)
{
    Bytes out = {(uint8_t)d1, (uint8_t)(d1 >> 8), (uint8_t)(d1 >> 16), (uint8_t)(d1 >> 24),
                 (uint8_t)d2, (uint8_t)(d2 >> 8), (uint8_t)d3, (uint8_t)(d3 >> 8)};
    out.insert(out.end(), d4);
    return out;
}
// [MS-GIPUSB] 2.2.2: Windows.Xbox.Input.IController / IGamepad / INavigationController
inline const Bytes GUID_ICONTROLLER = guid(0x9776FF56, 0x9BFD, 0x4581, {0xAD, 0x45, 0xB6, 0x45, 0xBB, 0xA5, 0x26, 0xD6});
inline const Bytes GUID_IGAMEPAD = guid(0x082E402C, 0x07DF, 0x45E1, {0xA5, 0xAB, 0xA3, 0x12, 0x7A, 0xF1, 0x97, 0xB5});
inline const Bytes GUID_INAVIGATION = guid(0xB8F31FE7, 0x7386, 0x40E9, {0xA9, 0xF8, 0x2F, 0x21, 0x26, 0x3A, 0xCF, 0xB7});
// [MS-GIPUSB] Table 60: IConsoleFunctionMap for message type 0x20
inline const Bytes GUID_ICONSOLE_FUNCTION_MAP = guid(0xECDDD2FE, 0xD387, 0x4294, {0xBD, 0x96, 0x1A, 0x71, 0x2E, 0x3D, 0xC7, 0x7D});

struct MetadataMessage
{
    uint8_t type;
    uint16_t length;
    bool upstream;
};

// Compiles metadata into the binary layout of the [MS-GIPUSB] 2.2.2 compiled gamepad example:
// a 16 byte header (header length, major 1, minor 0, reserved, total size), a 22 byte device
// metadata table of offsets (relative to the table), then the firmware versions, audio formats,
// in / out system commands, preferred type strings, interface GUIDs and 23 byte message entries.
inline Bytes metadata(const std::vector<std::string> &preferred_types, const std::vector<Bytes> &interfaces,
                      const std::vector<MetadataMessage> &messages,
                      const Bytes &in_commands = {1, 2, 3, 4, 6, 7}, const Bytes &out_commands = {1, 4, 5, 6, 10})
{
    Bytes body;
    auto le16 = [](Bytes &b, uint16_t v) {
        b.push_back((uint8_t)v);
        b.push_back((uint8_t)(v >> 8));
    };
    const uint16_t table = 22;
    uint16_t fw = table;
    Bytes fw_bytes = {1, 1, 0, 0, 0}; // one version: major 1, minor 0
    uint16_t audio = fw + fw_bytes.size();
    Bytes audio_bytes = {0};
    uint16_t in = audio + audio_bytes.size();
    Bytes in_bytes = {(uint8_t)in_commands.size()};
    in_bytes.insert(in_bytes.end(), in_commands.begin(), in_commands.end());
    uint16_t out = in + in_bytes.size();
    Bytes out_bytes = {(uint8_t)out_commands.size()};
    out_bytes.insert(out_bytes.end(), out_commands.begin(), out_commands.end());
    uint16_t preferred = out + out_bytes.size();
    Bytes preferred_bytes = {(uint8_t)preferred_types.size()};
    for (auto &s : preferred_types)
    {
        le16(preferred_bytes, (uint16_t)s.size());
        preferred_bytes.insert(preferred_bytes.end(), s.begin(), s.end());
    }
    uint16_t itf = preferred + preferred_bytes.size();
    Bytes itf_bytes = {(uint8_t)interfaces.size()};
    for (auto &g : interfaces)
        itf_bytes.insert(itf_bytes.end(), g.begin(), g.end());
    uint16_t msgs = itf + itf_bytes.size();
    Bytes msg_bytes = {(uint8_t)messages.size()};
    for (auto &m : messages)
    {
        Bytes entry(23, 0);
        entry[0] = 23;
        entry[2] = m.type;
        entry[3] = (uint8_t)m.length;
        entry[4] = (uint8_t)(m.length >> 8);
        entry[5] = 0x01;
        entry[7] = m.upstream ? 0x10 : 0x08;
        msg_bytes.insert(msg_bytes.end(), entry.begin(), entry.end());
    }
    le16(body, msgs);
    le16(body, fw);
    le16(body, audio);
    le16(body, in);
    le16(body, out);
    le16(body, preferred);
    le16(body, itf);
    le16(body, 0); // HID descriptor
    body.resize(table, 0);
    for (auto *part : {&fw_bytes, &audio_bytes, &in_bytes, &out_bytes, &preferred_bytes, &itf_bytes, &msg_bytes})
        body.insert(body.end(), part->begin(), part->end());
    Bytes blob = {0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0, 0};
    le16(blob, (uint16_t)(16 + body.size()));
    blob.insert(blob.end(), body.begin(), body.end());
    return blob;
}

// [MS-GIPUSB] 2.2.2 "GIP Gamepad metadata Example", the compiled 182 byte blob given in the spec:
// preferred type Windows.Xbox.Input.Gamepad, interfaces IController / IGamepad /
// INavigationController, messages 0x20 (14 bytes, upstream) and 0x09 (9 bytes, downstream)
inline const Bytes SPEC_GAMEPAD_METADATA = {
    0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB6, 0x00,
    0x77, 0x00, 0x16, 0x00, 0x1B, 0x00, 0x1C, 0x00, 0x23, 0x00, 0x29, 0x00, 0x46, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x02, 0x03,
    0x04, 0x06, 0x07, 0x05, 0x01, 0x04, 0x05, 0x06, 0x0A, 0x01, 0x1A, 0x00, 0x57, 0x69, 0x6E, 0x64,
    0x6F, 0x77, 0x73, 0x2E, 0x58, 0x62, 0x6F, 0x78, 0x2E, 0x49, 0x6E, 0x70, 0x75, 0x74, 0x2E, 0x47,
    0x61, 0x6D, 0x65, 0x70, 0x61, 0x64, 0x03, 0x56, 0xFF, 0x76, 0x97, 0xFD, 0x9B, 0x81, 0x45, 0xAD,
    0x45, 0xB6, 0x45, 0xBB, 0xA5, 0x26, 0xD6, 0x2C, 0x40, 0x2E, 0x08, 0xDF, 0x07, 0xE1, 0x45, 0xA5,
    0xAB, 0xA3, 0x12, 0x7A, 0xF1, 0x97, 0xB5, 0xE7, 0x1F, 0xF3, 0xB8, 0x86, 0x73, 0xE9, 0x40, 0xA9,
    0xF8, 0x2F, 0x21, 0x26, 0x3A, 0xCF, 0xB7, 0x02, 0x17, 0x00, 0x20, 0x0E, 0x00, 0x01, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17,
    0x00, 0x09, 0x09, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// --- Firmware source tables ---

// The bytes of a `const uint8_t name[] = { ... };` table in a firmware source file. The emulated
// device's announce / metadata tables are static in xone_device.cpp, so this reads them from there.
inline Bytes firmware_table(const std::string &relative, const std::string &name)
{
    std::ifstream file(std::string(SANTROLLER_ROOT) + "/" + relative);
    std::stringstream s;
    s << file.rdbuf();
    std::string src = s.str();
    size_t start = src.find("uint8_t " + name + "[]");
    if (start == std::string::npos)
        return {};
    size_t open = src.find('{', start);
    size_t close = src.find("};", open);
    std::string body = src.substr(open + 1, close - open - 1);
    Bytes out;
    std::stringstream tokens(body);
    std::string tok;
    while (std::getline(tokens, tok, ','))
    {
        size_t p = tok.find_first_not_of(" \t\r\n");
        if (p == std::string::npos)
            continue;
        out.push_back((uint8_t)std::stoul(tok.substr(p), nullptr, 0));
    }
    return out;
}

// --- gip_device_interface_t recorder ---

// The GIP packet starting at `data`, its length taken from its own header rather than from the
// caller, so GipDevice.QueuedPacketsAreWhole can check the caller's length against it.
// XGIPProtocol builds packets in a 64 byte buffer, so never reads past that.
inline Bytes packet_at(const uint8_t *data)
{
    Bytes head(data, data + 8);
    Header h = decode(head);
    size_t size = std::min<size_t>(h.header_size + h.length, 64);
    return Bytes(data, data + size);
}

struct Recorder
{
    // Packets queued for the controller, each read whole from its header, and the length the
    // firmware passed along with it
    std::vector<Bytes> queued;
    std::vector<uint16_t> queued_lengths;
    std::vector<Bytes> acks;
    std::vector<SubType> descriptors;
    int arrivals = 0;
    int disconnects = 0;
    bool request_descriptor_on_arrival = false;
    gip_device_t *device = nullptr;

    static void on_device_descriptor(void *ctx, SubType subtype)
    {
        auto *r = (Recorder *)ctx;
        r->descriptors.push_back(subtype);
        if (r->device)
            r->device->subtype = subtype;
    }
    static void on_arrival(void *ctx);
    static void queue_packet(void *ctx, const uint8_t *data, uint16_t len)
    {
        ((Recorder *)ctx)->queued.push_back(packet_at(data));
        ((Recorder *)ctx)->queued_lengths.push_back(len);
    }
    static void send_ack(void *ctx, const uint8_t *data, uint16_t len)
    {
        ((Recorder *)ctx)->acks.emplace_back(data, data + len);
    }
    static void on_disconnect(void *ctx)
    {
        ((Recorder *)ctx)->disconnects++;
    }
    static const gip_device_interface_t interface;
};

inline void Recorder::on_arrival(void *ctx)
{
    auto *r = (Recorder *)ctx;
    r->arrivals++;
    if (r->request_descriptor_on_arrival && r->device)
        gip_default_arrival_callback(r->device, nullptr);
}

inline const gip_device_interface_t Recorder::interface = {
    .on_device_descriptor = Recorder::on_device_descriptor,
    .on_arrival = Recorder::on_arrival,
    .queue_packet = Recorder::queue_packet,
    .send_ack = Recorder::send_ack,
    .on_disconnect = Recorder::on_disconnect,
};
} // namespace gip_test
