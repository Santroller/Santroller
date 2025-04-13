#include "devices/wt_drum.hpp"

unsigned WTMidiTransport::available() {
    if (mBufferIndex > 0) {
        mBufferIndex--;
    } else {
        if (Core::micros() - lastTick > 500) {
            lastTick = Core::micros();
            mCsPin->low();
            Core::delayMicroseconds(50);
            // 1: Send 0xAA, resp 0xAA
            uint8_t resp = mInterface->transfer(0xAA);
            if (resp != 0xAA) {
                missing++;
                if (missing > 10) {
                    mFound = false;
                    missing = 0;
                }
                mCsPin->high();
                return mBufferIndex;
            }
            mFound = true;
            // 2: Send 0x55, response: packet count in buffer
            resp = mInterface->transfer(0x55);
            delayMicroseconds(50);
            if (!resp) {
                // no packets in buffer
                mCsPin->high();
                return mBufferIndex;
            }
            // 3: read the rest of the packet
            // TODO: figure out if we can handle packets longer than 3?
            for (int i = 0; i < 3; i++) {
                resp = mInterface->transfer(0x00);
                mBuffer[i] = resp;
                delayMicroseconds(50);
            }
            mCsPin->high();
            mBufferIndex = 3;
        }
    }
    return mBufferIndex;
};
