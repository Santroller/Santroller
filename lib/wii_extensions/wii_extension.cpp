#include "wii_extension.hpp"

#include <string.h>

#include "devices/midi.hpp"
#include "main.hpp"
#include <cmath>
#include "utils.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    WiiExtension *inst = (WiiExtension *)user_data;
    inst->process_data(WII_ADDR, false, false, false, false);
    return 0;
}

bool WiiExtension::verifyData(const uint8_t *dataIn, uint8_t dataSize)
{
    uint8_t orCheck = 0x00;  // Check if data is zeroed (bad connection)
    uint8_t andCheck = 0xFF; // Check if data is maxed (bad init)

    for (int i = 0; i < dataSize; i++)
    {
        orCheck |= dataIn[i];
        andCheck &= dataIn[i];
    }

    if (orCheck == 0x00 || andCheck == 0xFF)
    {
        return false; // No data or bad data
    }

    return true;
}
// state machine to handle polling wii extensions
void WiiExtension::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    lastPoll = to_ms_since_boot(get_absolute_time());
    if (timeout || abort_detected)
    {
        if (status != WII_INIT_FINISH_ENC)
        {
            failCount++;
        }
        // during high load, there might be the occassional drop, so allow a few failures
        if (failCount > 10 || status == WII_INIT_FINISH_ENC)
        {
            status = WII_INIT_FINISH_ENC;
            mType = WiiExtType::WiiNoExtension;
            restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
            return;
        }
    }
    if (stop_detected && !abort_detected)
    {
        failCount = 0;
        switch (status)
        {
        case WII_INIT_FINISH_ENC:
            status = WII_INIT_FB_0;
            break;
        case WII_INIT_FB_0:
            status = WII_INIT_READ_ID_WRITE_PTR;
            break;
        case WII_INIT_READ_ID_WRITE_PTR:
            status = WII_INIT_READ_ID_READ;
            break;
        case WII_INIT_READ_ID_READ:
            if (verifyData(bufferRx, WII_ID_LEN))
            {
                mType = static_cast<WiiExtType>(bufferRx[0] << 8 | bufferRx[5]);
                wiiPointer = 0;
                wiiBytes = 6;
                hiRes = false;
                hasTapBar = false;
                s_box = 0;
                if (mType == WiiUbisoftDrawsomeTablet)
                {
                    status = WII_INIT_DRAWSOME;
                }
                else if (mType == WiiExtType::WiiClassicController ||
                         mType == WiiExtType::WiiClassicControllerPro)
                {
                    status = WII_INIT_CLASSIC_0;
                }
                else
                {
                    status = WII_INIT_READ_DATA_WRITE_PTR;
                }
                if (mType == WiiExtType::WiiTaikoNoTatsujinController)
                {
                    // We can cheat a little with these controllers, as most of the bytes that
                    // get read back are constant. Hence we start at 0x5 instead of 0x0.
                    wiiPointer = 5;
                    wiiBytes = 1;
                }
            }
            break;
        case WII_INIT_DRAWSOME:
            status = WII_INIT_READ_DATA_WRITE_PTR;
            break;
        case WII_INIT_CLASSIC_0:
            status = WII_INIT_CLASSIC_1;
            break;
        case WII_INIT_CLASSIC_1:
            status = WII_INIT_CLASSIC_2;
            break;
        case WII_INIT_CLASSIC_2:
            status = WII_INIT_CLASSIC_READ_ID_WRITE_PTR;
            break;
        case WII_INIT_CLASSIC_READ_ID_WRITE_PTR:
            status = WII_INIT_CLASSIC_READ_ID_READ;
            break;
        case WII_INIT_CLASSIC_READ_ID_READ:
            if (bufferRx[4] == WII_HIGHRES_MODE)
            {
                hiRes = true;
                wiiBytes = 8;
            }
            else
            {
                hiRes = false;
            }
            status = WII_INIT_READ_DATA_WRITE_PTR;
            break;
        case WII_INIT_READ_DATA_WRITE_PTR:
            status = WII_INIT_READ_DATA_READ;
            break;
        case WII_INIT_READ_DATA_READ:
        {
            uint8_t orCheck = 0x00;
            for (int i = 0; i < wiiBytes; i++)
            {
                orCheck |= bufferRx[i];
            }
            if (orCheck == 0)
            {
                status = WII_INIT_ENABLE_ENC_0;
            }
            else
            {
                status = WII_INPUTS_WRITE_PTR;
            }
            break;
        }
        case WII_INIT_ENABLE_ENC_0:
            status = WII_INIT_ENABLE_ENC_1;
            break;
        case WII_INIT_ENABLE_ENC_1:
            status = WII_INIT_ENABLE_ENC_2;
            break;
        case WII_INIT_ENABLE_ENC_2:
            status = WII_INIT_ENABLE_ENC_3;
            break;
        case WII_INIT_ENABLE_ENC_3:
            status = WII_INIT_ENC_READ_ID_WRITE_PTR;
            break;
        case WII_INIT_ENC_READ_ID_WRITE_PTR:
            status = WII_INIT_ENC_READ_ID_READ;
            break;
        case WII_INIT_ENC_READ_ID_READ:
        {
            s_box = FIRST_PARTY_SBOX;
            if (bufferRx[3] != 0xFF)
            {
                s_box = THIRD_PARTY_SBOX;
            }
            status = WII_INPUTS_WRITE_PTR;
            break;
        }
        case WII_INPUTS_WRITE_PTR:
            status = WII_INPUTS_READ;
            break;
        case WII_INPUTS_READ:
        {
            status = WII_INPUTS_WRITE_PTR;
            if (verifyData(bufferRx, wiiBytes))
            {
                update_data(bufferRx, wiiBytes, m_device);
                // Update the led if it changes
                if (mType == WiiExtType::WiiDjHeroTurntable && ledUpdated)
                {
                    status = WII_INPUTS_UPDATE_LED;
                    ledUpdated = false;
                }
            }
            break;
        }
        case WII_INPUTS_UPDATE_LED:
            status = WII_INPUTS_WRITE_PTR;
            break;
        }
        // add 200us delay between commands otherwise the extension is overwhelmed
        restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
        return;
    }
    switch (status)
    {
    case WII_INIT_FINISH_ENC:
        bufferTx[0] = WII_ENCRYPTION_STATE_ID;
        bufferTx[1] = WII_ENCRYPTION_FINISH_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_FB_0:
        bufferTx[0] = 0xFB;
        bufferTx[1] = 0x00;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 1, nullptr, 0);
        break;
    case WII_INIT_READ_ID_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INIT_DRAWSOME:
        bufferTx[0] = 0xFB;
        bufferTx[1] = 0x01;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_0:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_1:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_2:
        bufferTx[0] = WII_SET_RES_MODE;
        bufferTx[1] = WII_HIGHRES_MODE;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 1, nullptr, 0);
        break;
    case WII_INIT_CLASSIC_READ_ID_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INIT_READ_DATA_WRITE_PTR:
        bufferTx[0] = wiiPointer;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 1, nullptr, 0);
        break;
    case WII_INIT_READ_DATA_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, wiiBytes);
        break;
    case WII_INIT_ENABLE_ENC_0:
        bufferTx[0] = WII_ENCRYPTION_STATE_ID;
        bufferTx[1] = WII_ENCRYPTION_ENABLE_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_1:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID;
        memset(bufferTx + 1, 0, 6);
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 7, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_2:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID_2;
        memset(bufferTx + 1, 0, 6);
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 7, nullptr, 0);
        break;
    case WII_INIT_ENABLE_ENC_3:
        bufferTx[0] = WII_ENCRYPTION_KEY_ID_3;
        memset(bufferTx + 1, 0, 4);
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 5, nullptr, 0);
        break;
    case WII_INIT_ENC_READ_ID_WRITE_PTR:
        bufferTx[0] = WII_READ_ID;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 1, nullptr, 0);
        break;
    case WII_INIT_ENC_READ_ID_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, WII_ID_LEN);
        break;
    case WII_INPUTS_WRITE_PTR:
        bufferTx[0] = wiiPointer;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 1, nullptr, 0);
        break;
    case WII_INPUTS_READ:
        mInterface.dmaWriteRead(WII_ADDR, nullptr, 0, bufferRx, wiiBytes);
        break;
    case WII_INPUTS_UPDATE_LED:
    {
        // encrypt if encryption is enabled
        uint8_t state = nextEuphoriaLedState ? 1 : 0;
        if (s_box)
        {
            state = (state - s_box) ^ s_box;
        }
        bufferTx[0] = WII_DJ_EUPHORIA;
        bufferTx[1] = state;
        mInterface.dmaWriteRead(WII_ADDR, bufferTx, 2, nullptr, 0);
        break;
    }
    }
}

WiiExtension::WiiExtension(MidiDevice *midiDevice, uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock) : mInterface(block, sda, scl, clock), mFound(false), m_block(block), m_device(midiDevice)
{
    printf("WiiExtension::WiiExtension\r\n");
}
void WiiExtension::begin()
{
    printf("WiiExtension::begin\r\n");
    memset(bufferRx, 0, sizeof(bufferRx));
    mInterface.dmaInit(WII_ADDR, this);
    process_data(WII_ADDR, false, false, false, false);
}
void WiiExtension::load_state(const DeviceReloadState *state)
{
    printf("WiiExtension::load_state %d\r\n", state->wii_status);
    // load state from previous instance
    mFound = state->wii_mFound;
    mType = state->wii_mType;
    hiRes = state->wii_hiRes;
    packetIssueCount = state->wii_packetIssueCount;
    mBufferIndex = state->wii_mBufferIndex;
    lastTick = state->wii_lastTick;
    wiiBytes = state->wii_wiiBytes;
    wiiPointer = state->wii_wiiPointer;
    s_box = state->wii_s_box;
    m_block = state->wii_m_block;
    status = state->wii_status;
}
void WiiExtension::save_state(DeviceReloadState &state) const
{
    printf("WiiExtension::save_state\r\n");
    state.wii_mFound = mFound;
    state.wii_mType = mType;
    state.wii_hiRes = hiRes;
    state.wii_packetIssueCount = packetIssueCount;
    state.wii_mBufferIndex = mBufferIndex;
    state.wii_lastTick = lastTick;
    state.wii_wiiBytes = wiiBytes;
    state.wii_wiiPointer = wiiPointer;
    state.wii_s_box = s_box;
    state.wii_m_block = m_block;
    state.wii_status = status;
}
WiiExtension::~WiiExtension()
{
    printf("WiiExtension::~WiiExtension\r\n");
}
void WiiExtension::end()
{
    printf("WiiExtension::end\r\n");
    cancel_alarm(restart_alarm_id);
    mInterface.dmaDeinit(WII_ADDR);
}

void WiiExtension::setEuphoriaLed(bool state)
{
    nextEuphoriaLedState = state;
    ledUpdated = true;
}
void WiiExtension::tick()
{
    mInterface.tick();
    if (lastPoll && to_ms_since_boot(get_absolute_time()) - lastPoll > 500)
    {
        process_data(WII_ADDR, false, false, false, false);
    }
}