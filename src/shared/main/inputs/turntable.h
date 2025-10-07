#include "config.h"

#ifdef INPUT_QUAD
    if ((lastQuadPoll - millis()) > 1) {
        lastQuadPoll = millis();
        new_value = quadrature_encoder_get_count(pio, sm);
        delta = new_value - old_value;
        old_value = new_value;
    }
#endif
#ifdef INPUT_DJ_TURNTABLE
uint8_t *dj_left = lastSuccessfulTurntablePacketLeft;
uint8_t *dj_right = lastSuccessfulTurntablePacketRight;
int8_t dj_turntable_left = 0;
int8_t dj_turntable_right = 0;
bool djLeftValid = true;
bool djRightValid = true;
bool elapsed = (micros() - lastDj) > INPUT_DJ_TURNTABLE_POLL_RATE;
if (elapsed) {
    lastDj = micros();
    djLeftValid = twi_readFromPointer(DJ_TWI_PORT, DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketLeft), dj_left);
    djRightValid = twi_readFromPointer(DJ_TWI_PORT, DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(lastSuccessfulTurntablePacketRight), dj_right);
    lastTurntableWasSuccessfulLeft = djLeftValid;
    lastTurntableWasSuccessfulRight = djRightValid;
}
#ifdef CONFIGURABLE_BLOBS

if (djLeftValid) {
    if (INPUT_DJ_TURNTABLE_SMOOTHING) {
        if (elapsed) {
            dj_sum_left -= dj_last_readings_left[dj_next_left];
            dj_last_readings_left[dj_next_left] = (int8_t)dj_left[2];
            dj_sum_left += dj_last_readings_left[dj_next_left];
            dj_next_left++;
            if (dj_next_left >= TURNTABLE_BUFFER_SIZE) {
                dj_next_left = 0;
            }
        }
        dj_turntable_left = (dj_sum_left / TURNTABLE_BUFFER_SIZE);
    } else {
        dj_turntable_left = (int8_t)dj_left[2];
    }
}
if (djRightValid) {
    if (INPUT_DJ_TURNTABLE_SMOOTHING) {
        if (elapsed) {
            dj_sum_right -= dj_last_readings_right[dj_next_right];
            dj_last_readings_right[dj_next_right] = (int8_t)dj_right[2];
            dj_sum_right += dj_last_readings_right[dj_next_right];
            dj_next_right++;
            if (dj_next_right >= TURNTABLE_BUFFER_SIZE) {
                dj_next_right = 0;
            }
        }
    } else {
        dj_turntable_right = (int8_t)dj_right[2];
    }
}

#else
// DJ Hero turntables are pretty noisy, so smooth that out with a moving average
#if INPUT_DJ_TURNTABLE_SMOOTHING
if (djLeftValid) {
    if (elapsed) {
        dj_sum_left -= dj_last_readings_left[dj_next_left];
        dj_last_readings_left[dj_next_left] = (int8_t)dj_left[2];
        dj_sum_left += dj_last_readings_left[dj_next_left];
        dj_next_left++;
        if (dj_next_left >= TURNTABLE_BUFFER_SIZE) {
            dj_next_left = 0;
        }
    }
    dj_turntable_left = (dj_sum_left / TURNTABLE_BUFFER_SIZE);
}
if (djRightValid) {
    if (elapsed) {
        dj_sum_right -= dj_last_readings_right[dj_next_right];
        dj_last_readings_right[dj_next_right] = (int8_t)dj_right[2];
        dj_sum_right += dj_last_readings_right[dj_next_right];
        dj_next_right++;
        if (dj_next_right >= TURNTABLE_BUFFER_SIZE) {
            dj_next_right = 0;
        }
    }
    dj_turntable_right = (dj_sum_right / TURNTABLE_BUFFER_SIZE);
}
#else
dj_turntable_left = (int8_t)dj_left[2];
dj_turntable_right = (int8_t)dj_right[2];
#endif
#endif

#endif