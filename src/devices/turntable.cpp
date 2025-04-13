#include "devices/turntable.hpp"

#include "interfaces/core.hpp"

void TurntableDevice::tick(san_base_t* data) {
    if ((Core::micros() - lastDj) > mPollRate) {
        lastDj = Core::micros();
        uint8_t left[3];
        uint8_t right[3];
        mLeftConnected = mInterface->readFromPointer(DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(left), left);
        mRightConnected = mInterface->readFromPointer(DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(right), right);
        if (mLeftConnected) {
            data->turntable.leftTableVelocity = (int8_t)left[2];
            data->turntable.leftGreen = left[0] & (1 << 4);
            data->turntable.leftRed = left[0] & (1 << 5);
            data->turntable.leftBlue = left[0] & (1 << 6);
        }
        if (mRightConnected) {
            data->turntable.rightTableVelocity = (int8_t)right[2];
            data->turntable.rightGreen = right[0] & (1 << 4);
            data->turntable.rightRed = right[0] & (1 << 5);
            data->turntable.rightBlue = right[0] & (1 << 6);
        }
    }
}