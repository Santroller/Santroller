#include <stdint.h>
#include <string.h>

#include <cstdio>
#include <vector>

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
};
typedef struct
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

class XboxMessage {
   public:
    GipHeader_t header;
    uint8_t* data;
    XboxMessage(uint8_t* buffer, uint8_t len) {
        header = *(GipHeader_t*)buffer;
        int bytesRead = 3;
        header.packet_length = decodeLEB128(buffer + bytesRead, &bytesRead);
        if (header.chunked) {
            header.chunk_offset = decodeLEB128(buffer + bytesRead, &bytesRead);
        }
        data = buffer + bytesRead;
        if (header.packet_length != len - bytesRead) {
            printf("Xbox header length does not match! %d != %d\r\n", len - bytesRead, header.packet_length);
        }
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
            printf("Variable-length value is greater than 4 bytes! Buffer: {ParsingUtils.ToHexString(data)}\r\n");
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
            printf("Value to encode ({%02x}) is greater than allowed!\r\n", value);
            return -1;
        }

        return byteLength;
    }
};

class XboxChunk {
   public:
    uint8_t data[1024];
    uint16_t max_len;
    uint16_t bytes_used;
    bool valid;
    uint8_t currentChunk;
    XboxResult setChunkData(uint8_t* dataToSend, uint16_t dataLen) {
        max_len = dataLen;
        currentChunk = 0;
        bytes_used = 0;
        memcpy(data, dataToSend, dataLen);
    }
    XboxResult getChunk(XboxMessage message) {
        if (currentChunk == 0 || (currentChunk + 1) % 5 == 0) {
            message.header.needsAck = 1;
        } else {
            message.header.needsAck = 0;
        }
        if (currentChunk == 0) {
            message.header.chunkStart = 1;
            message.header.chunk_offset = max_len;
        } else if (bytes_used == max_len) {
            // end of packet
            message.header.needsAck = 0;
            message.header.packet_length = 0;
        } else {
            message.header.chunkStart = 0;
            message.header.chunk_offset = max_len;
        }
        currentChunk++;
    }
    XboxResult processChunk(XboxMessage message) {
        int bufferIndex = message.header.chunk_offset;

        // Do nothing with chunks of length 0
        if (bufferIndex <= 0) {
            // Chunked packets with a length of 0 are valid and have been observed with Elite controllers
            bool emptySequence = bufferIndex == 0;
            if (!emptySequence) {
                printf("Negative buffer index %d!\r\n", bufferIndex);
            }
            return emptySequence ? Success : InvalidMessage;
        }

        // Start of the chunk sequence
        if (!valid || message.header.chunkStart) {
            // Safety check
            if (!message.header.chunkStart) {
                // Some devices trigger this condition during authentication,
                // so we don't fail if it's an auth packet
                if (message.header.command != GIP_AUTHENTICATION) {
                    printf("Invalid chunk sequence start! No chunk buffer exists, expected a chunk start packet\r\n");
                }
                return InvalidMessage;
            }

            // Buffer index is the total size of the buffer on the starting packet
            max_len = message.header.chunk_offset;
            bufferIndex = 0;
            bytes_used = 0;
            valid = true;
        }

        // Validate sequence alignment
        if (bufferIndex != bytes_used) {
            // We don't fail here since this seems to be a consistent issue on devices it affects
            // Debug.Fail("Invalid chunk sequence ordering! Buffer index is not aligned with the previous chunk");
            return InvalidMessage;
        }

        // Buffer index equalling buffer length signals the end of the sequence
        if (bufferIndex >= max_len) {
            // Safety checks
            if (bufferIndex > max_len) {
                printf("Invalid chunk sequence end! Buffer index is beyond the end of the chunk buffer\r\n");
                return InvalidMessage;
            }

            if (message.header.packet_length != 0) {
                printf("Invalid chunk sequence end! Data was provided beyond the end of the buffer\r\n");
                return InvalidMessage;
            }

            // Send off finished chunk buffer
            message.data = data;
            valid = false;
            bytes_used = 0;

            // Update header
            message.header.packet_length = max_len;
            message.header.chunkStart = false;
            return Success;
        }

        // Verify chunk data bounds
        if ((bufferIndex + message.header.packet_length) > max_len) {
            printf("Invalid chunk sequence! Data was provided beyond the end of the buffer\r\n");
            return InvalidMessage;
        }
        memcpy(data, message.data, message.header.packet_length);
        // Copy data to buffer
        bytes_used = bufferIndex + message.header.packet_length;
        return Pending;
    }
};
typedef struct
{
    GipHeader_t header;
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
    GipHeader_t header;
} __attribute__((packed)) Gip_DeviceDescriptorRequest_t;
typedef struct
{
    GipHeader_t header;
    uint8_t unk1;
    uint8_t unk2;
} __attribute__((packed)) Gip_Auth_Done_t;
typedef struct
{
    GipHeader_t header;
    uint8_t unk;
    uint8_t mode;
    uint8_t brightness;
} __attribute__((packed)) Gip_Led_On_t;
typedef struct {
    GipHeader_t header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view
} __attribute__((packed)) XboxOneInputHeader_Data_t;

typedef struct
{
    GipHeader_t header;
    bool pressed : 1;
    uint8_t : 7;
    uint8_t keycode;
} __attribute__((packed)) GipKeystroke_t;

typedef struct
{
    GipHeader_t header;
    uint8_t subcommand;
} __attribute__((packed)) GipPowerMode_t;

typedef struct
{
    GipHeader_t header;
    uint8_t data[];
} __attribute__((packed)) GipPacket_t;

typedef struct {
    GipHeader_t header;
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
    packet->header.command = cmd;                \
    packet->header.internal = isInternal;        \
    packet->header.sequence = seq;               \
    packet->header.client = 0;                   \
    packet->header.needsAck = 0;                 \
    packet->header.chunkStart = 0;               \
    packet->header.chunked = 0;                  \
    packet->header.length = sizeof(*packet) - sizeof(GipHeader_t);

class XboxOneParser {
   public:
    XboxOneParser();
    void parse(uint8_t* report, uint8_t len);

   private:
    uint8_t previousReceiveSequence[128];
    uint8_t previousSendSequence[128];
    XboxChunk chunk;
    int descriptorFailCount;
    Gip_Ack_t ack;
    bool needsAck = false;
};