#include "devices/usb.hpp"
unsigned USBDevice::available() {
    if (mBufferIndex > 0) {
        mBufferIndex--;
    }
    return mBufferIndex;
};

void USBDevice::tick(san_base_t* data) {
    mParser.parse(mReportData, sizeof(mReportData), data);
    if (data->midi.midiPacket[0]) {
        memcpy(mBuffer, data->midi.midiPacket, sizeof(data->midi.midiPacket));
        mBufferIndex += sizeof(mBuffer);
    }
}