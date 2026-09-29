#pragma once

#include <stdint.h>
#include <string.h>

/*
 * Canonical PS2 protocol defaults.  These are shared by the real firmware
 * initialization and the host replay harness.
 */
static const uint8_t init_resp_40[6] = {0,0,0,0,0,0x5A};
static const uint8_t init_resp_41_digital[6] = {0,0,0,0,0,0};
static const uint8_t init_resp_41_analog[6] = {0xFF,0xFF,0x03,0,0,0x5A};
static const uint8_t init_resp_42[32] = {0xFF,0xFF};
static const uint8_t init_resp_43[6] = {0,0,0,0,0,0};
static const uint8_t init_resp_44[6] = {0,0,0,0,0,0};
static const uint8_t init_resp_45_ds2[6] = {0x03,0x02,0x00,0x02,0x01,0x00};
static const uint8_t init_resp_45_gh[6]  = {0x01,0x02,0x00,0x02,0x01,0x00};
static const uint8_t init_resp_46[2][6] = {
    {0,0,1,2,0,0x0A},
    {0,0,1,1,1,0x14}
};
static const uint8_t init_resp_47[6] = {0,0,2,0,1,0};
static const uint8_t init_resp_4c[2][6] = {
    {0,0,0,4,0,0},
    {0,0,0,7,0,0}
};
static const uint8_t init_resp_4d[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8_t init_resp_4f[6] = {0,0,0,0,0,0x5A};

typedef struct psx_spi_protocol_state_t
{
    uint8_t config_responses[0x10][0x06];
    uint8_t resp_42[32];
    uint8_t report_mask[3];
    uint8_t button_attr[12];
    volatile uint8_t last_cmd;
    volatile bool has_new_cmd;
    volatile bool configMode;
    bool just_left_config;
    uint8_t cmd_id;
    bool locked;
    volatile uint8_t rumble_small;
    volatile uint8_t rumble_large;
    volatile uint8_t dma_buf[32];
    uint8_t dma_config_buf[6];
} psx_spi_protocol_state_t;

/*
 * Canonical protocol initialization.  Both firmware and host replay use
 * this exact state type and initializer; hardware/PIO state stays outside.
 */
#define PSX_SPI_PROTOCOL_INIT(s, guitar_hero_guitar) do { \
    memset((s), 0, sizeof(*(s))); \
    memcpy((s)->config_responses[0x00], init_resp_40, 6); \
    memcpy((s)->config_responses[0x01], init_resp_41_digital, 6); \
    memcpy((s)->config_responses[0x02], init_resp_42, sizeof(init_resp_42)); \
    memcpy((s)->config_responses[0x03], init_resp_43, 6); \
    memcpy((s)->config_responses[0x04], init_resp_44, 6); \
    memcpy((s)->config_responses[0x05], init_resp_45_ds2, 6); \
    memcpy((s)->config_responses[0x06], init_resp_46[0], 6); \
    memcpy((s)->config_responses[0x07], init_resp_47, 6); \
    memcpy((s)->config_responses[0x0C], init_resp_4c[0], 6); \
    memcpy((s)->config_responses[0x0D], init_resp_4d, 6); \
    memcpy((s)->config_responses[0x0F], init_resp_4f, 6); \
    memset((s)->button_attr, 0x02, sizeof((s)->button_attr)); \
    (s)->report_mask[0] = 0x03; \
    (s)->configMode = false; \
    if (guitar_hero_guitar) \
        memcpy((s)->config_responses[0x05], init_resp_45_gh, 6); \
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
        memcpy((s)->config_responses[0x06], init_resp_46[0], 6); \
        memcpy((s)->config_responses[0x0C], init_resp_4c[0], 6); \
        (s)->configMode=(b)[3]; \
        break; \
    case 0x44: \
        (s)->locked=(b)[4]; \
        memset((s)->config_responses[0x01],0,sizeof((s)->config_responses[0x01])); \
        memset((s)->report_mask,0,sizeof((s)->report_mask)); \
        if ((b)[3]) { \
            (s)->config_responses[0x05][2]=1; \
            memcpy((s)->config_responses[0x01],init_resp_41_analog,6); \
            (s)->report_mask[0]=0x3F; \
        } else { \
            (s)->config_responses[0x05][2]=0; \
            memcpy((s)->config_responses[0x01],init_resp_41_digital,6); \
            (s)->report_mask[0]=0x03; \
        } \
        break; \
    case 0x4C: \
        if ((b)[3]==0) memcpy((s)->config_responses[0x0C],init_resp_4c[1],6); \
        if ((b)[3]>=1) memset((s)->config_responses[0x0C],0,sizeof((s)->config_responses[0x0C])); \
        break; \
    case 0x46: \
        if ((b)[3]==0) memcpy((s)->config_responses[0x06],init_resp_46[1],6); \
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

static inline uint8_t psx_spi_current_report_len(const psx_spi_protocol_state_t *s)
{
    if (s->configMode)
        return 0;

    if (!s->config_responses[0x05][2])
        return 2;

    uint8_t len = 0;
    for (uint8_t i = 0; i < 18; ++i)
    {
        if (s->report_mask[i / 8] & (1u << (i % 8)))
            ++len;
    }
    return len;
}

/*
 * The header describes the response that will be queued next. In analog
 * mode its low bits are derived from the current report mask, rather than
 * the previous transaction's report_len value. Firmware materializes
 * report_len immediately before arming the response; deriving it here
 * keeps the replay state machine on the same protocol semantics.
 */
#define PSX_SPI_RESPONSE_HEADER(s,len) ((s)->configMode ? 0xF3 : ((s)->config_responses[0x05][2] ? (uint8_t)(0x70 | ((len) / 2)) : (uint8_t)(0x40 | ((len) / 2))))
