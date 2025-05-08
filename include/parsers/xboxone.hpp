#pragma once
#include <map>

#include "base.hpp"
#include "devices/usb.hpp"
#include "interfaces/usb.hpp"
#include "protocols/xbox_one.hpp"

class XboxOneParser : public Parser {
   public:
    XboxOneParser(USBInterface* interface);
    void parse(uint8_t* report, uint8_t len, san_base_t* data);
    int build(san_base_t* data, uint8_t* output);

   private:
    XboxResult processChunk(GipHeader_t header, uint8_t* buffer, uint16_t len);
    static int decodeLEB128(uint8_t* data, int* bytesRead);
    static int encodeLEB128(uint8_t* buffer, int bufferLen, int value);
    void sendACK(GipHeader_t header);
    void powerOnController();
    void sendDescriptorRequest();
    void sendPacket(GipHeader_t header, void* data);
    uint8_t previousReceiveSequence[128];
    uint8_t previousSendSequence[128];
    uint8_t buffer[1024];
    int descriptorFailCount;
    USBInterface* usbInterface;
    GipHeader_t header;
    uint8_t currentChunk;
    uint16_t max_len;
    uint16_t bytes_used;
    bool inChunk;
    bool poweredOn;
};