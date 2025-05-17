#include "devices/usb.hpp"

void USBDevice::tick(san_base_t* data) {
    mParser.parse(mReportData, sizeof(mReportData), data);
    if (data->midi.midiPacket[0]) {
        midiInterface.parsePacket(data->midi.midiPacket, sizeof(data->midi.midiPacket));
    }
}