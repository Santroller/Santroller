// TODO: for this, we should actually just outright parse the packets fully and do this right
// could potentially even look at matching via guid instead of vid and pid
// but we put all the xb1 stuff here including auth.
#include "parsers/xboxone.hpp"

#include <stdint.h>

XboxOneParser::XboxOneParser() {

}

void XboxOneParser::parse(uint8_t* report, uint8_t len, san_base_t* data) {
    XboxMessage message = XboxMessage(report);
    if (message.header.packet_length != len) {
        printf("Xbox header length does not match! %d != %d\r\n", len, message.header.packet_length);
    }
    if (message.header.chunked) {
        auto res = chunk.processChunk(message);
        switch (res) {
            case Success:
                break;
            case Pending:
                return;
            default:
                if (message.header.command == GIP_DEVICE_DESCRIPTOR) {
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
    if (message.header.needsAck) {
        ack.header.command = GIP_CMD_ACKNOWLEDGE;
        // The Xbox One driver seems to always send this for the inner flag
        ack.header.internal = true; 
        ack.header.chunked = false;
        ack.header.needsAck = false;
        ack.header.sequence = message.header.sequence;
        ack.innerChunked = message.header.chunked;
        ack.innerCommand = message.header.command;
        ack.innerClient = message.header.client;
        ack.innerInternal = message.header.internal;
        ack.innerNeedsAck = message.header.needsAck;
        ack.unk1 = 0;
        ack.unk2 = 0;
        ack.bytesReceived = message.header.packet_length;
        if (message.header.chunked) {
            ack.bytesReceived = chunk.bytes_used;
            ack.remainingBuffer = chunk.max_len - chunk.bytes_used;
        }
        needsAck = true;

    }
    int prevSequence = previousReceiveSequence[message.header.command];
    // don't handle the same packet twice
    if (message.header.sequence == prevSequence) {
        return;
    }
    previousReceiveSequence[message.header.command] = message.header.sequence;
    printf("Parsed command, %d\r\n", message.header.command);
    
    // Essentially, we would parse data from the controller here and store it locally to the parser as state
    // TODO: it may pay to just give the parser a way to send packets without needing to be ticked
    // just need to consider that we have two parsers: one between console and pico and another between pico and controller
    // TODO: maybe instead of testing with the pico we just test with a pc to begin with, using packet captures since we have some from the portals.
}

int XboxOneParser::build(san_base_t* data, uint8_t* output) {
    if (needsAck) {
        memcpy(output, &ack, sizeof(ack));
        needsAck = false;
        return sizeof(ack);
    }
    // And then here we return controller inputs when auth is done, but otherwise before that we can return data for auth and descriptors and such
    return 0;
}