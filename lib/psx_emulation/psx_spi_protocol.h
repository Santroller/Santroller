#pragma once

#include <stdint.h>
#include <string.h>

/*
 * Shared PS2 command/state logic.
 *
 * This is intentionally header-only/inline: the firmware IRQ path and the
 * host replay harness execute the same parser without introducing a function
 * call into the time-critical path.
 */

static const uint8_t psx_test_resp_40[6] = {0,0,0,0,0,0x5A};
static const uint8_t psx_test_resp_41_digital[6] = {0,0,0,0,0,0};
static const uint8_t psx_test_resp_41_analog[6]  = {0xFF,0xFF,0x03,0,0,0x5A};
static const uint8_t psx_test_resp_46[2][6] = {
    {0,0,1,2,0,0x0A},
    {0,0,1,1,1,0x14}
};
static const uint8_t psx_test_resp_47[6] = {0,0,2,0,1,0};
static const uint8_t psx_test_resp_4d[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8_t psx_test_resp_4f[6] = {0,0,0,0,0,0x5A};
static const uint8_t psx_test_resp_4c[2][6] = {
    {0,0,0,4,0,0},
    {0,0,0,7,0,0}
};

#define PSX_SPI_PROTOCOL_INIT(s) do { \
    memset((s), 0, sizeof(*(s))); \
    memcpy((s)->config_responses[0x00], psx_test_resp_40, 6); \
    memcpy((s)->config_responses[0x01], psx_test_resp_41_digital, 6); \
    memcpy((s)->config_responses[0x06], psx_test_resp_46[0], 6); \
    memcpy((s)->config_responses[0x07], psx_test_resp_47, 6); \
    memcpy((s)->config_responses[0x0D], psx_test_resp_4d, 6); \
    memcpy((s)->config_responses[0x0F], psx_test_resp_4f, 6); \
    memcpy((s)->config_responses[0x0C], psx_test_resp_4c[0], 6); \
    memset((s)->button_attr, 0x02, sizeof((s)->button_attr)); \
    (s)->report_len = 2; \
    (s)->report_mask[0] = 0x03; \
    (s)->configMode = false; \
    (s)->config_responses[0x05][0] = 0x03; \
    (s)->config_responses[0x05][1] = 0x02; \
    (s)->config_responses[0x05][2] = 0x00; \
    (s)->config_responses[0x05][3] = 0x02; \
    (s)->config_responses[0x05][4] = 0x01; \
    (s)->config_responses[0x05][5] = 0x00; \
} while (0)

#define PSX_SPI_PROCESS_COMMAND(s, b) do { \
    switch ((b)[1]) { \
    case 0x40: \
        (s)->config_responses[0x00][2] = (s)->button_attr[(b)[3]]; \
        (s)->button_attr[(b)[3]] = (b)[4]; \
        break; \
    case 0x42: \
        if (!(s)->configMode) { (s)->rumble_small=(b)[3]; (s)->rumble_large=(b)[4]; } \
        break; \
    case 0x43: \
        memcpy((s)->config_responses[0x06], psx_test_resp_46[0], 6); \
        memcpy((s)->config_responses[0x0C], psx_test_resp_4c[0], 6); \
        (s)->configMode=(b)[3]; \
        break; \
    case 0x44: \
        (s)->locked=(b)[4]; \
        memset((s)->config_responses[0x01],0,sizeof((s)->config_responses[0x01])); \
        memset((s)->report_mask,0,sizeof((s)->report_mask)); \
        if ((b)[3]) { \
            (s)->config_responses[0x05][2]=1; \
            memcpy((s)->config_responses[0x01],psx_test_resp_41_analog,6); \
            (s)->report_mask[0]=0x3F; \
        } else { \
            (s)->config_responses[0x05][2]=0; \
            memcpy((s)->config_responses[0x01],psx_test_resp_41_digital,6); \
            (s)->report_mask[0]=0x03; \
        } \
        break; \
    case 0x4C: \
        if ((b)[3]==0) memcpy((s)->config_responses[0x0C],psx_test_resp_4c[1],6); \
        if ((b)[3]>=1) memset((s)->config_responses[0x0C],0,sizeof((s)->config_responses[0x0C])); \
        break; \
    case 0x46: \
        if ((b)[3]==0) memcpy((s)->config_responses[0x06],psx_test_resp_46[1],6); \
        if ((b)[3]>=1) memset((s)->config_responses[0x06],0,sizeof((s)->config_responses[0x06])); \
        break; \
    case 0x4F: \
        if ((s)->config_responses[0x05][2]) { (s)->report_mask[0]=(b)[3]; (s)->report_mask[1]=(b)[4]; (s)->report_mask[2]=(b)[5]; } \
        break; \
    case 0x4D: \
        for (int i=0;i<6;i++) (s)->config_responses[0x0D][i]=(b)[3+i]; \
        break; \
    default: break; \
    } \
} while (0)

#define PSX_SPI_RESPONSE_HEADER(s) ((s)->configMode ? 0xF3 : ((s)->config_responses[0x05][2] ? (uint8_t)(0x70|((s)->report_len/2)) : (uint8_t)(0x40|((s)->report_len/2))))
