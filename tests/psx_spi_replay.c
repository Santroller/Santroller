#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    uint8_t config_responses[0x10][6];
    uint8_t report_mask[3];
    uint8_t button_attr[12];
    uint8_t report_len;
    uint8_t rumble_small, rumble_large;
    uint8_t locked;
    uint8_t configMode;
} psx_test_state_t;

#include "../lib/psx_emulation/psx_spi_protocol.h"

static void report_len_from_mask(psx_test_state_t *s)
{
    if (!s->config_responses[0x05][2]) { s->report_len = 2; return; }
    unsigned n = 0;
    for (unsigned i = 0; i < 18; ++i)
        if (s->report_mask[i / 8] & (1u << (i % 8))) ++n;
    s->report_len = (uint8_t)n;
}

static int check_bytes(const char *what, const uint8_t *got,
                       const uint8_t *want, unsigned n)
{
    if (!memcmp(got, want, n)) return 0;
    fprintf(stderr, "FAIL %s\n  got: ", what);
    for (unsigned i=0;i<n;++i) fprintf(stderr,"%02X ",got[i]);
    fprintf(stderr,"\n want: ");
    for (unsigned i=0;i<n;++i) fprintf(stderr,"%02X ",want[i]);
    fprintf(stderr,"\n");
    return 1;
}

static int transaction(psx_test_state_t *s, unsigned n,
                       const uint8_t tx[8], uint8_t expected_header,
                       const uint8_t *expected_config)
{
    int failed = 0;
    uint8_t header = PSX_SPI_RESPONSE_HEADER(s);
    if (header != expected_header) {
        fprintf(stderr,"FAIL txn %u cmd=%02X: header got %02X want %02X\n",
                n,tx[1],header,expected_header);
        failed = 1;
    }
    if (s->configMode && expected_config)
        failed |= check_bytes("config payload",
            s->config_responses[tx[1]-0x40], expected_config, 6);

    PSX_SPI_PROCESS_COMMAND(s, tx);
    report_len_from_mask(s);
    return failed;
}

#define TX(...) { 0x01, __VA_ARGS__ }

int main(void)
{
    psx_test_state_t s;
    PSX_SPI_PROTOCOL_INIT(&s);

    static const uint8_t z[6]={0,0,0,0,0,0};
    static const uint8_t ds2_45[6]={0x03,0x02,0x00,0x02,0x01,0x00};
    static const uint8_t rumble_align_old[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    static const uint8_t fourf_resp[6]={0,0,0,0,0,0x5A};
    static const uint8_t analog_45[6]={0x03,0x02,0x01,0x02,0x01,0x00};

    const struct { uint8_t tx[8]; uint8_t header; const uint8_t *payload; } trace[] = {
        {TX(0x43,0x00,0x01,0,0,0,0),0x41,z},
        {TX(0x45,0x00,0,0,0,0,0),0xF3,ds2_45},
        {TX(0x43,0x00,0x00,0,0,0,0),0xF3,z},
        {TX(0x42,0x00,0,0,0,0,0),0x41,NULL},
        {TX(0x43,0x00,0,0,0,0,0),0x41,z},
        {TX(0x44,0x00,0x01,0x03,0,0,0),0xF3,z},
        {TX(0x43,0x00,0,0,0,0,0),0xF3,z},
        {TX(0x42,0x00,0,0,0,0,0),0x73,NULL},
        {TX(0x43,0x00,0,0,0,0,0),0x73,z},
        {TX(0x4D,0x00,0x01,0xFF,0xFF,0xFF,0xFF),0xF3,rumble_align_old},
        {TX(0x43,0x00,0,0,0,0,0),0xF3,z},
        {TX(0x42,0x00,0,0,0,0,0),0x73,NULL},
        {TX(0x43,0x00,0,0,0,0,0),0x73,z},
        {TX(0x4F,0x00,0xFF,0xFF,0x03,0,0),0xF3,fourf_resp},
        {TX(0x42,0x00,0,0,0,0,0),0x79,NULL},
        {TX(0x43,0x00,0,0,0,0,0),0x79,z},
        {TX(0x45,0x00,0,0,0,0,0),0xF3,analog_45},
        {TX(0x43,0x00,0,0,0,0,0),0xF3,z},
        {TX(0x42,0x00,0,0,0,0,0),0x79,NULL},
    };

    int failures=0;
    for (unsigned i=0;i<sizeof(trace)/sizeof(trace[0]);++i)
        failures += transaction(&s,i+1,trace[i].tx,trace[i].header,trace[i].payload);

    if (s.configMode || s.report_len != 18 ||
        s.report_mask[0]!=0xFF || s.report_mask[1]!=0xFF || s.report_mask[2]!=0x03) {
        fprintf(stderr,"FAIL final state: config=%u len=%u mask=%02X %02X %02X\n",
                s.configMode,s.report_len,s.report_mask[0],s.report_mask[1],s.report_mask[2]);
        ++failures;
    }

    if (failures) { fprintf(stderr,"%d checks failed\n",failures); return 1; }
    puts("PASS: 19 Wireless PS2 transactions/state transitions");
    return 0;
}
