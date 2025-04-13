#include "devices/bh_drum.hpp"
unsigned BHMidiTransport::available() {
    if (mBufferIndex > 0) {
        mBufferIndex--;
    } else {
        uint8_t data[8];
        if (mInterface->readFromPointer(DRUM_ADDR, BH_DRUM_PTR, sizeof(data), data) && data[0] == BH_DRUM_PTR) {
            memcpy(mBuffer, data + 1, sizeof(data) - 1);
            mBufferIndex = sizeof(data) - 1;
        }
    }
    return mBufferIndex;
};