// TODO: for this, we should actually just outright parse the packets fully and do this right
// could potentially even look at matching via guid instead of vid and pid
// but we put all the xb1 stuff here including auth
#include "xbox_one_parser.hpp"

#include <stdint.h>

XboxOneParser::XboxOneParser() {

}
// TODO: give the parser some way to send ACKs and other messages
// TODO: we can now disconnect the client and the host from each other, since we can just take the auth packets, decode them and then send them on their way
// TODO: grab a full packet capture with auth, and then test parsing that
// TODO: and then test with a console
void XboxOneParser::parse(uint8_t* report, uint8_t len) {
    XboxMessage message = XboxMessage(report, len);

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
        printf("needs ack!\r\n");

    }
    if (message.header.chunked) {
        auto res = chunk.processChunk(message);
        switch (res) {
            case Success:
                break;
            case Pending:
                needsAck = message.header.needsAck;
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
    needsAck = message.header.needsAck;
    int prevSequence = previousReceiveSequence[message.header.command];
    // don't handle the same packet twice
    if (message.header.sequence == prevSequence) {
        return;
    }
    previousReceiveSequence[message.header.command] = message.header.sequence;
    printf("Parsed command, %d\r\n", message.header.command);
}