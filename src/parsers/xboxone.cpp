#include "parsers/xboxone.hpp"

#include <stdint.h>

XboxOneParser::XboxOneParser(USBInterface* usbInterface) : usbInterface(usbInterface) {
}
void XboxOneParser::sendPacket(GipHeader_t header, void* data) {
    int bytesRead = 3;
    memcpy(buffer, &header, bytesRead);
    bytesRead += encodeLEB128(buffer + bytesRead, sizeof(buffer), header.packet_length);
    if (header.chunked) {
        bytesRead += encodeLEB128(buffer + bytesRead, sizeof(buffer), header.chunk_offset);
    }
    if (data) {
        memcpy(buffer + bytesRead, (uint8_t*)data, header.packet_length);
        bytesRead += header.packet_length;
    }
    usbinterface.sendReport(buffer, bytesRead);
}
void XboxOneParser::sendACK(GipHeader_t header) {
    Gip_Ack_t ack;
    GipHeader_t ackHeader;
    ackHeader.command = GIP_CMD_ACKNOWLEDGE;
    // The Xbox One driver seems to always send this for the inner flag
    ackHeader.internal = true;
    ackHeader.chunked = false;
    ackHeader.needsAck = false;
    ackHeader.sequence = header.sequence;
    ackHeader.packet_length = sizeof(ack);
    ack.innerChunked = header.chunked;
    ack.innerCommand = header.command;
    ack.innerClient = header.client;
    ack.innerInternal = header.internal;
    ack.innerNeedsAck = header.needsAck;
    ack.unk1 = 0;
    ack.unk2 = 0;
    ack.bytesReceived = header.packet_length;
    if (header.chunked) {
        ack.bytesReceived = bytes_used;
        ack.remainingBuffer = max_len - bytes_used;
    }
    printf("needs ack!\r\n");
    sendPacket(ackHeader, &ack);
}
void XboxOneParser::sendDescriptorRequest() {
    GipHeader_t header;
    header.command = GIP_DEVICE_DESCRIPTOR;
    header.sequence = 1;
    header.internal = true;
    header.chunked = false;
    sendPacket(header, nullptr);
}
void XboxOneParser::powerOnController() {
    GipHeader_t header;
    header.command = GIP_DEVICE_DESCRIPTOR;
    header.sequence = 1;
    header.internal = true;
    header.chunked = false;
    sendPacket(header, nullptr);
}

// XboxResult setChunkData(uint8_t* dataToSend, uint16_t dataLen) {
//     max_len = dataLen;
//     currentChunk = 0;
//     bytes_used = 0;
//     memcpy(data, dataToSend, dataLen);
//     return Success;
// }
// XboxResult getChunk(XboxMessage message) {
//     if (currentChunk == 0 || (currentChunk + 1) % 5 == 0) {
//         header.needsAck = 1;
//     } else {
//         header.needsAck = 0;
//     }
//     if (currentChunk == 0) {
//         header.chunkStart = 1;
//         header.chunk_offset = max_len;
//     } else if (bytes_used == max_len) {
//         // end of packet
//         header.needsAck = 0;
//         header.packet_length = 0;
//     } else {
//         header.chunkStart = 0;
//         header.chunk_offset = max_len;
//     }
//     currentChunk++;
//     return Success;
// }
XboxResult XboxOneParser::processChunk(GipHeader_t header, uint8_t* packet, uint16_t len) {
    int bufferIndex = header.chunk_offset;

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
    if (!inChunk || header.chunkStart) {
        // Safety check
        if (!header.chunkStart) {
            // Some devices trigger this condition during authentication,
            // so we don't fail if it's an auth packet
            if (header.command != GIP_AUTHENTICATION) {
                printf("Invalid chunk sequence start! No chunk buffer exists, expected a chunk start packet\r\n");
            }
            return InvalidMessage;
        }

        // Buffer index is the total size of the buffer on the starting packet
        max_len = header.chunk_offset;
        bufferIndex = 0;
        bytes_used = 0;
        inChunk = true;
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

        if (header.packet_length != 0) {
            printf("Invalid chunk sequence end! Data was provided beyond the end of the buffer\r\n");
            return InvalidMessage;
        }

        // Send off finished chunk buffer
        inChunk = false;
        bytes_used = 0;

        // Update header
        header.packet_length = max_len;
        header.chunkStart = false;
        return Success;
    }

    // Verify chunk data bounds
    if ((bufferIndex + header.packet_length) > max_len) {
        printf("Invalid chunk sequence! Data was provided beyond the end of the buffer\r\n");
        return InvalidMessage;
    }
    memcpy(buffer, packet, header.packet_length);
    // Copy data to buffer
    bytes_used = bufferIndex + header.packet_length;
    return Pending;
}

void XboxOneParser::parse(uint8_t* report, uint8_t len, san_base_t* data) {
    // TODO: i kinda like how gp2040 did the https://github.com/OpenStickCommunity/GP2040-CE/blob/main/src/drivers/shared/xgip_protocol.cpp, honestly might be easier to just copy that and work from thats
    int bytesRead = 3;
    memcpy(&header, report, bytesRead);
    header.packet_length = decodeLEB128(report + bytesRead, &bytesRead);
    if (header.chunked) {
        header.chunk_offset = decodeLEB128(report + bytesRead, &bytesRead);
    }
    if (header.needsAck) {
        sendACK(header);
    }
    if (header.chunked) {
        auto res = processChunk(header, report + bytesRead, len - bytesRead);
        switch (res) {
            case Success:
                break;
            case Pending:
                return;
            default:
                if (header.command == GIP_DEVICE_DESCRIPTOR) {
                    descriptorFailCount++;
                    if (descriptorFailCount >= 3) {
                        // Disconnect device after too many failed descriptors
                        printf("Descriptor read fail!\r\n");
                        return;
                    }

                    // var resendResult = SendMessage(XboxDescriptor.GetDescriptor);
                    // if (resendResult != XboxResult.Success)
                    //     return resendResult;
                }
                return;
        }
    }
    int prevSequence = previousReceiveSequence[header.command];
    // don't handle the same packet twice
    if (header.sequence == prevSequence) {
        return;
    }
    previousReceiveSequence[header.command] = header.sequence;
    printf("Parsed command, %d\r\n", header.command);
    if (header.command == GIP_ARRIVAL) {
        sendDescriptorRequest();
    }
    if (header.command == GIP_DEVICE_DESCRIPTOR && !poweredOn) {
        powerOnController();
    }
}

int XboxOneParser::decodeLEB128(uint8_t* data, int* bytesRead) {
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

int XboxOneParser::encodeLEB128(uint8_t* buffer, int bufferLen, int value) {
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

int XboxOneParser::build(san_base_t* data, uint8_t* output) {
    // And then here we return controller inputs when auth is done, but otherwise before that we can return data for auth and descriptors and such
    return 0;
}