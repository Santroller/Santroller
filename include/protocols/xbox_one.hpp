#include <stdint.h>

#include <vector>

#include "reports/ps3_reports.h"
#define GIP_CMD_ACKNOWLEDGE 0x01
#define GIP_ARRIVAL 0x02
#define GIP_DEVICE_DESCRIPTOR 0x04
#define GIP_POWER_MODE_DEVICE_CONFIG 0x05
#define GIP_AUTHENTICATION 0x06
#define GIP_VIRTUAL_KEYCODE 0x07
#define GIP_CMD_RUMBLE 0x09
#define GIP_CMD_LED 0x0a
#define GIP_INPUT_REPORT 0x20
#define GHL_HID_REPORT 0x21
#define GHL_HID_OUTPUT 0x22
#define GIP_VKEY_LEFT_WIN 0x5b
#define GIP_POWER_ON 0x00
#define GIP_LED_ON 0x01

enum XboxResult {
    /// <summary>The packet was processed successfully.</summary>
    Success,
    /// <summary>More packet data is incoming and needs to be received.</summary>
    Pending,
    /// <summary>The device was disconnected.</summary>
    Disconnected,
    /// <summary>The packet contains an invalid message.</summary>
    InvalidMessage,
    /// <summary>The device being connected is not supported.</summary>
    UnsupportedDevice,
} typedef struct
{
    uint8_t command;
    uint8_t client : 4;
    uint8_t needsAck : 1;
    uint8_t internal : 1;
    uint8_t chunkStart : 1;
    uint8_t chunked : 1;
    uint8_t sequence;
    uint32_t packet_length;
    uint32_t chunk_offset;
} __attribute__((packed)) GipHeader_t;

class XboxChunk {
   public:
    std::unique_ptr<uint8_t*> data;
    XboxResult processChunk(XboxMessage message) {
        int bufferIndex = header.ChunkIndex;

        // Do nothing with chunks of length 0
        if (bufferIndex <= 0) {
            // Chunked packets with a length of 0 are valid and have been observed with Elite controllers
            bool emptySequence = bufferIndex == 0;
            Debug.Assert(emptySequence, $ "Negative buffer index {bufferIndex}!");
            return emptySequence ? Success : InvalidMessage;
        }

        // Start of the chunk sequence
        if (Buffer == null || (header.Flags & XboxCommandFlags.ChunkStart) != 0) {
            // Safety check
            if ((header.Flags & XboxCommandFlags.ChunkStart) == 0) {
                // Some devices trigger this condition during authentication,
                // so we don't fail if it's an auth packet
                Debug.Assert(header.CommandId == XboxAuthentication.CommandId,
                             "Invalid chunk sequence start! No chunk buffer exists, expected a chunk start packet");
                return InvalidMessage;
            }

            // Buffer index is the total size of the buffer on the starting packet
            Buffer = new byte[bufferIndex];
            bufferIndex = 0;
            BytesUsed = 0;
        }

        // Validate sequence alignment
        if (bufferIndex != BytesUsed) {
            // We don't fail here since this seems to be a consistent issue on devices it affects
            // Debug.Fail("Invalid chunk sequence ordering! Buffer index is not aligned with the previous chunk");
            return InvalidMessage;
        }

        // Buffer index equalling buffer length signals the end of the sequence
        if (bufferIndex >= Buffer.Length) {
            // Safety checks
            if (bufferIndex > Buffer.Length) {
                Debug.Fail("Invalid chunk sequence end! Buffer index is beyond the end of the chunk buffer");
                return InvalidMessage;
            }

            if (chunkData.Length != 0) {
                Debug.Fail("Invalid chunk sequence end! Data was provided beyond the end of the buffer");
                return InvalidMessage;
            }

            // Send off finished chunk buffer
            chunkData = Buffer;
            Buffer = null;
            BytesUsed = 0;

            // Update header
            header.DataLength = chunkData.Length;
            header.Flags &= ~(XboxCommandFlags.ChunkPacket | XboxCommandFlags.ChunkStart);
            return Success;
        }

        // Verify chunk data bounds
        if ((bufferIndex + chunkData.Length) > Buffer.Length) {
            Debug.Fail($ "Invalid chunk sequence! Data was provided beyond the end of the buffer");
            return InvalidMessage;
        }

        // Copy data to buffer
        chunkData.CopyTo(Buffer.AsSpan(bufferIndex, chunkData.Length));
        BytesUsed = bufferIndex + chunkData.Length;
        return Pending;
    }
};
class XboxMessage {
   public:
    GipHeader_t header;
    uint8_t* data;
    XboxMessage(uint8_t* buffer) {
        header = *(GipHeader_t*)buffer;
        int bytesRead = 3;
        header.packet_length = decodeLEB128(buffer + bytesRead, *bytesRead);
        if (header.chunked) {
            header.chunk_offset = decodeLEB128(buffer + bytesRead, *bytesRead);
        }
        data = buffer + bytesRead;
    }

    static int decodeLEB128(uint8_t* data, int* bytesRead) {
        int result = 0;
        int byteLength = 0;

        // Decode variable-length length value
        // Sequence length is limited to 4 bytes
        uint8_t value;
        do {
            value = data[byteLength];
            result |= (value & 0x7F) << (byteLength * 7);
            byteLength++;
        } while ((value & 0x80) != 0 && byteLength < sizeof(int));

        // Detect length sequences longer than 4 bytes
        if ((value & 0x80) != 0) {
            printf("Variable-length value is greater than 4 bytes! Buffer: {ParsingUtils.ToHexString(data)}");
            return -1;
        }
        *bytesRead += byteLength;
        return result;
    }

    static int encodeLEB128(uint8_t* buffer, int bufferLen, int value) {
        int byteLength = 0;
        if (!bufferLen)
            return false;

        // Encode the given value
        // Sequence length is limited to 4 bytes
        uint8_t result;
        do {
            result = (uint8_t)(value & 0x7F);
            if (value > 0x7F) {
                result |= 0x80;
                value >>= 7;
            }

            buffer[byteLength] = result;
            byteLength++;
        } while (value > 0x7F && byteLength < sizeof(int));

        // Detect values too large to encode
        if (value > 0x7F) {
            printf("Value to encode ({value}) is greater than allowed!");
            return -1;
        }

        return byteLength;
    }
};

typedef struct
{
    GipHeader_t Header;
    uint8_t unk1;
    uint8_t innerCommand;
    uint8_t innerClient : 4;
    uint8_t innerNeedsAck : 1;
    uint8_t innerInternal : 1;
    uint8_t innerChunkStart : 1;
    uint8_t innerChunked : 1;
    uint16_t bytesReceived;
    uint16_t unk2;
    uint16_t remainingBuffer;
} __attribute__((packed)) Gip_Ack_t;
typedef struct
{
    GipHeader_t Header;
} __attribute__((packed)) Gip_DeviceDescriptorRequest_t;
typedef struct
{
    GipHeader_t Header;
    uint8_t unk1;
    uint8_t unk2;
} __attribute__((packed)) Gip_Auth_Done_t;
typedef struct
{
    GipHeader_t Header;
    uint8_t unk;
    uint8_t mode;
    uint8_t brightness;
} __attribute__((packed)) Gip_Led_On_t;
typedef struct {
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view
} __attribute__((packed)) XboxOneInputHeader_Data_t;
typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;

    uint16_t leftTrigger;
    uint16_t rightTrigger;

    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
} __attribute__((packed)) XboxOneGamepad_Data_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t : 1;
    uint8_t solo : 1;  // leftThumbClick
    uint8_t : 1;

    uint8_t tilt;
    uint8_t whammy;
    uint8_t pickup;

    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t : 3;

    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;
    uint8_t soloOrange : 1;
    uint8_t : 3;

    uint8_t unknown[3];
} __attribute__((packed)) XboxOneRockBandGuitar_Data_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t : 1;
    uint8_t solo : 1;  // leftThumbClick
    uint8_t : 1;

    uint8_t tilt;
    uint8_t lowEFret;
    uint8_t aFret;

    uint8_t dFret;
    uint8_t gFret;
    uint8_t bFret;
    uint8_t highEFret;

    uint8_t unknown[1];
} __attribute__((packed)) XboxOneRockBandProGuitar_Data_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t : 1;

    uint8_t solo : 1;  // leftThumbClick
    uint8_t : 1;

    uint8_t key8 : 1;
    uint8_t key7 : 1;
    uint8_t key6 : 1;
    uint8_t key5 : 1;
    uint8_t key4 : 1;
    uint8_t key3 : 1;
    uint8_t key2 : 1;
    uint8_t key1 : 1;

    uint8_t key16 : 1;
    uint8_t key15 : 1;
    uint8_t key14 : 1;
    uint8_t key13 : 1;
    uint8_t key12 : 1;
    uint8_t key11 : 1;
    uint8_t key10 : 1;
    uint8_t key9 : 1;

    uint8_t key24 : 1;
    uint8_t key23 : 1;
    uint8_t key22 : 1;
    uint8_t key21 : 1;
    uint8_t key20 : 1;
    uint8_t key19 : 1;
    uint8_t key18 : 1;
    uint8_t key17 : 1;

    union {
        struct {
            uint8_t velocity1 : 7;
            uint8_t key25 : 1;

            uint8_t velocity2 : 7;
            uint8_t : 1;

            uint8_t velocity3 : 7;
            uint8_t : 1;

            uint8_t velocity4 : 7;
            uint8_t : 1;

            uint8_t velocity5 : 7;
            uint8_t : 1;
        };
        uint8_t velocities[5];
    };

    uint8_t : 7;
    uint8_t overdrive : 1;
    uint8_t pedalAnalog : 7;
    uint8_t pedalDigital : 1;

    uint8_t touchPad : 7;
    uint8_t : 1;

    uint8_t pedalConnection : 1;  // If this matches PS3 MPA behavior, always 0 with the MIDI Pro Adapter
    uint8_t : 7;
} __attribute__((packed)) XboxOneRockBandKeyboard_Data_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;   // kick1
    uint8_t rightShoulder : 1;  // kick2
    uint8_t : 1;
    uint8_t : 1;

    uint8_t yellowVelocity : 4;
    uint8_t redVelocity : 4;
    uint8_t greenVelocity : 4;
    uint8_t blueVelocity : 4;

    uint8_t blueCymbalVelocity : 4;
    uint8_t yellowCymbalVelocity : 4;
    uint8_t : 4;
    uint8_t greenCymbalVelocity : 4;
} __attribute__((packed)) XboxOneRockBandDrums_Data_t;

// This isnt actually real but by doing this we can get some sane gamepad mappings in turntable mode
typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t : 4;

    uint16_t leftTrigger;
    uint16_t rightTrigger;

    uint16_t leftGreen : 1;
    uint16_t leftRed : 1;
    uint16_t leftBlue : 1;
    uint16_t : 13;

    uint16_t rightGreen : 1;
    uint16_t rightRed : 1;
    uint16_t rightBlue : 1;
    uint16_t : 13;

    int16_t leftTableVelocity;
    int16_t rightTableVelocity;
    int16_t effectsKnob;
    int16_t crossfader;
} __attribute__((packed)) XboxOneTurntable_Data_t;

typedef struct
{
    GipHeader_t Header;
    PS3GHLGuitar_Data_t report;
} __attribute__((packed)) XboxOneGHLGuitar_Data_t;
typedef struct
{
    XboxOneGHLGuitar_Data_t guitar;
    XboxOneGamepad_Data_t gamepad;
} __attribute__((packed)) XboxOneGHLGuitarWithGamepad_Data_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t sub_command;
    uint8_t data[7];
} __attribute__((packed)) XboxOneGHLGuitar_Output_t;

typedef struct
{
    GipHeader_t Header;
    bool pressed : 1;
    uint8_t : 7;
    uint8_t keycode;
} __attribute__((packed)) GipKeystroke_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t subcommand;
} __attribute__((packed)) GipPowerMode_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t data[];
} __attribute__((packed)) GipPacket_t;

typedef struct {
    GipHeader_t Header;
    uint8_t subCommand;  // Assumed based on the descriptor reporting a larger max length than what this uses
    uint8_t flags;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t leftMotor;
    uint8_t rightMotor;
    uint8_t duration;  // in deciseconds?
    uint8_t delay;     // in deciseconds?
    uint8_t repeat;    // in deciseconds?
} __attribute__((packed)) GipRumble_t;

#define GIP_HEADER(packet, cmd, isInternal, seq) \
    packet->Header.command = cmd;                \
    packet->Header.internal = isInternal;        \
    packet->Header.sequence = seq;               \
    packet->Header.client = 0;                   \
    packet->Header.needsAck = 0;                 \
    packet->Header.chunkStart = 0;               \
    packet->Header.chunked = 0;                  \
    packet->Header.length = sizeof(*packet) - sizeof(GipHeader_t);
