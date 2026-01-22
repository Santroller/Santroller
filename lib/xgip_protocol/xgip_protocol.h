#pragma once
 
 #include <stdint.h>
 
 // All max chunks are this size
 #define GIP_MAX_CHUNK_SIZE 0x3A
 
typedef enum {
    GIP_ACK_RESPONSE = 0x01,              // Xbox One ACK
    GIP_ANNOUNCE = 0x02,                  // Xbox One Announce
    GIP_KEEPALIVE = 0x03,                 // Xbox One Keep-Alive
    GIP_DEVICE_DESCRIPTOR = 0x04,         // Xbox One Definition
    GIP_SET_STATE = 0x05,                 // Xbox One Power Mode Config
    GIP_AUTH = 0x06,                      // Xbox One Authentication
    GIP_VIRTUAL_KEYCODE = 0x07,           // XBox One Guide button pressed
    GIP_CMD_RUMBLE = 0x09,                // Xbox One Rumble Command
    GIP_CMD_LED_ON = 0x0A,                // Xbox One (LED On)
    GIP_FINAL_AUTH = 0x1E,                // Xbox One (Final auth?)
    GIP_INPUT_REPORT = 0x20,              // Xbox One Input Report
    GIP_HID_REPORT = 0x21,                // Xbox One HID Report
} XboxOneReport;

typedef enum {
    GIP_STATE_START = 0x00,
    GIP_STATE_STOP = 0x01,
    GIP_STATE_FULL_POWER = 0x03,
    GIP_STATE_OFF = 0x04,
    GIP_STATE_QUIESCE = 0x05,
    GIP_STATE_RESET = 0x07
} XboxOneDeviceState;

 typedef struct
 {
     uint8_t command;
     uint8_t client : 4;
     uint8_t needsAck : 1;
     uint8_t internal : 1;
     uint8_t chunkStart : 1;
     uint8_t chunked : 1;
     uint8_t sequence;
     uint8_t length;
 } __attribute__((packed)) GipHeader_t;
 
 #define GIP_HEADER(packet, cmd, isInternal, seq) \
     packet->header.command = cmd;                \
     packet->header.internal = isInternal;        \
     packet->header.sequence = seq;               \
     packet->header.client = 0;                   \
     packet->header.needsAck = 0;                 \
     packet->header.chunkStart = 0;               \
     packet->header.chunked = 0;                  \
     packet->header.length = sizeof(*packet) - sizeof(GipHeader_t);
 
 class XGIPProtocol {
 public:
     XGIPProtocol();
     ~XGIPProtocol();
     void reset();                               // Reset packet information
     bool parse(const uint8_t * buffer, uint16_t len); // Parse incoming packet
     bool validate();                            // is valid packet?
     bool endOfChunk();                          // Is this the end of the chunk?
     void setAttributes(uint8_t cmd, uint8_t seq, uint8_t internal, uint8_t isChunked, uint8_t needsAck);   // Set attributes for next output packet
     void incrementSequence();                   // Add 1 to sequence
     bool setData(const uint8_t* data, uint16_t len); // Set data (buf and length)
     uint8_t * generatePacket();                 // Generate output packet (chunk will generate on-going packet)
     uint8_t * generateAckPacket();              // Generate an ack for the last received packet
     bool validateAck(XGIPProtocol & ackPacket); // Validate an incoming ack packet against 
     uint8_t getCommand();                       // Get command of a parsed packet
     uint8_t getSequence();                      // Get sequence of a parsed packet
     uint8_t getChunked();                       // Is this packet chunked?
     uint8_t getPacketAck();                     // Did the packet require an ACK?
     uint8_t getPacketLength();                  // Get packet length of our last output
     uint8_t * getData();                        // Get data from a packet or packet-chunk
     uint16_t getDataLength();                   // Get length of a packet or packet-chunk
     bool getChunkData(XGIPProtocol & packet);   // Get chunk data from incoming packet
     bool ackRequired();                         // Did our last parsed packet require an ack?
 private:
     void writeLeb128(uint8_t* dest, uint16_t len);
     GipHeader_t header;             // On-going GIP header
     uint16_t totalChunkLength;      // How big is the chunk?
     uint16_t actualDataReceived;    // How much actual data have we received?
     uint16_t totalChunkReceived;    // How much have we received in chunk mode length? (length | 0x80)
     uint16_t totalChunkSent;        // How much have we sent?
     uint16_t totalDataSent;         // How much actual data have we sent?
     uint16_t numberOfChunksSent;    // How many actual chunks have we sent?
     bool chunkEnded;                // did we hit the end of the chunk successfully?
     uint8_t packet[64];             // for output packets
     uint16_t packetLength;          // LAST SENT packet length
     uint8_t data[1024];             // Total data in this packet
     uint16_t dataLength;            // actual length of data
     bool isValidPacket;             // is this a valid packet or did we get an error?
 };