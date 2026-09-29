#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "../lib/psx_emulation/psx_spi_protocol.h"

typedef struct {
    unsigned capture_id;
    uint8_t tx;
    uint8_t rx;
} captured_byte_t;

typedef psx_spi_protocol_state_t psx_test_state_t;

typedef enum {
    PSX_TEST_DS2,
    PSX_TEST_GUITAR_HERO_GUITAR
} psx_test_controller_t;

static void init_test_controller(psx_test_state_t *s,
                                 psx_test_controller_t controller)
{
    PSX_SPI_PROTOCOL_INIT(s, controller == PSX_TEST_GUITAR_HERO_GUITAR);

}

static int hexbyte_after_colon(const char *s, uint8_t *out)
{
    const char *p = strchr(s, ':');
    if (!p)
        return 0;

    ++p;
    while (*p && isspace((unsigned char)*p))
        ++p;

    unsigned v;
    if (sscanf(p, "%2x", &v) != 1)
        return 0;

    *out = (uint8_t)v;
    return 1;
}

static int csv_field(const char *line, unsigned wanted, char *out, size_t out_sz)
{
    unsigned field = 0;
    const char *p = line;

    while (*p) {
        const char *start = p;
        while (*p && *p != ',')
            ++p;

        if (field == wanted) {
            size_t n = (size_t)(p - start);
            if (n >= out_sz)
                n = out_sz - 1;
            memcpy(out, start, n);
            out[n] = 0;
            return 1;
        }

        if (*p == ',')
            ++p;
        ++field;
    }

    return 0;
}

static int load_capture_columns(const char *path,
                                unsigned cmd_col,
                                unsigned data_col,
                                captured_byte_t **stream_out,
                                size_t *stream_n)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        return -1;
    }

    size_t cap = 1024, n = 0;
    captured_byte_t *stream = malloc(cap * sizeof(*stream));
    if (!stream) {
        fclose(f);
        return -1;
    }

    char line[4096];

    while (fgets(line, sizeof(line), f)) {
        char fc[256], fd[256];
        uint8_t cb, db;
        unsigned idc, idd;

        if (!csv_field(line, cmd_col, fc, sizeof(fc)) ||
            !csv_field(line, data_col, fd, sizeof(fd)))
            continue;

        const char *pc = strchr(fc, ':');
        const char *pd = strchr(fd, ':');
        if (!pc || !pd ||
            sscanf(fc, "%u:", &idc) != 1 ||
            sscanf(fd, "%u:", &idd) != 1 ||
            idc != idd ||
            !hexbyte_after_colon(fc, &cb) ||
            !hexbyte_after_colon(fd, &db))
            continue;

        if (n == cap) {
            cap *= 2;
            captured_byte_t *ns = realloc(stream, cap * sizeof(*stream));
            if (!ns) {
                free(stream);
                fclose(f);
                return -1;
            }
            stream = ns;
        }

        stream[n++] = (captured_byte_t){ idc, cb, db };
    }

    fclose(f);
    *stream_out = stream;
    *stream_n = n;
    return 0;
}

static int is_ps2_command(uint8_t b)
{
    switch (b) {
    case 0x40: case 0x41: case 0x42: case 0x43:
    case 0x44: case 0x45: case 0x46: case 0x47:
    case 0x4C: case 0x4D: case 0x4F:
        return 1;
    default:
        return 0;
    }
}

static void dump_bytes(const char *prefix, const uint8_t *p, unsigned n)
{
    fprintf(stderr, "%s", prefix);
    for (unsigned i = 0; i < n; ++i)
        fprintf(stderr, "%02X%s", p[i], i + 1 == n ? "" : " ");
    fputc('\n', stderr);
}

static void print_state_change(const char *name,
                               unsigned id,
                               uint8_t cmd,
                               const psx_test_state_t *before,
                               const psx_test_state_t *after)
{
    if (before->configMode != after->configMode)
        printf("  %s @%u: 43 -> config %u -> %u\n",
               name, id, before->configMode, after->configMode);

    if (before->config_responses[0x05][2] != after->config_responses[0x05][2] ||
        memcmp(before->report_mask, after->report_mask, 3) != 0 ||
        before->report_len != after->report_len) {
        printf("  %s @%u: %02X -> analog=%u len=%u mask=%02X %02X %02X\n",
               name, id, cmd,
               after->config_responses[0x05][2] == 1,
               after->report_len,
               after->report_mask[0],
               after->report_mask[1],
               after->report_mask[2]);
    }

    if (cmd == 0x45 || cmd == 0x41 || cmd == 0x4F ||
        cmd == 0x44 || cmd == 0x43) {
        printf("  %s @%u: %02X response/type=%02X %02X %02X %02X %02X %02X\n",
               name, id, cmd,
               after->config_responses[0x05][0],
               after->config_responses[0x05][1],
               after->config_responses[0x05][2],
               after->config_responses[0x05][3],
               after->config_responses[0x05][4],
               after->config_responses[0x05][5]);
    }
}

static int replay_controller(const char *name,
                             captured_byte_t *stream,
                             size_t nstream,
                             psx_test_controller_t controller)
{
    psx_test_state_t s;
    init_test_controller(&s, controller);

    unsigned transactions = 0;
    unsigned failures = 0;
    unsigned skipped = 0;

    uint8_t queued_header = PSX_SPI_RESPONSE_HEADER(&s);
    uint8_t queued_config[0x10][6];
    memset(queued_config, 0, sizeof(queued_config));
    for (int r = 0; r < 0x10; ++r)
        memcpy(queued_config[r], s.config_responses[r], 6);

    printf("\n=== %s ===\n", name);
    printf("Initial: type=%s config=%u analog=%u mask=%02X %02X %02X\n",
           controller == PSX_TEST_GUITAR_HERO_GUITAR
               ? "GUITAR_HERO_GUITAR" : "DS2",
           s.configMode,
           s.config_responses[0x05][2] == 1,
           s.report_mask[0], s.report_mask[1], s.report_mask[2]);

    size_t i = 0;
    while (i + 1 < nstream) {
        if (stream[i].tx != 0x01 ||
            !is_ps2_command(stream[i + 1].tx)) {
            ++i;
            ++skipped;
            continue;
        }

        size_t next = i + 1;
        while (next < nstream &&
               !(stream[next].tx == 0x01 &&
                 next + 1 < nstream &&
                 is_ps2_command(stream[next + 1].tx)))
            ++next;

        size_t tx_len = next - i;
        if (tx_len == 0 || tx_len > 21) {
            fprintf(stderr,
                    "%s: SKIP invalid txn at stream=%zu cmd=%02X len=%zu\n",
                    name, i, stream[i + 1].tx, tx_len);
            ++skipped;
            i = next;
            continue;
        }

        if (i + tx_len > nstream) {
            fprintf(stderr,
                    "%s: SKIP truncated txn at stream=%zu cmd=%02X len=%zu\n",
                    name, i, stream[i + 1].tx, tx_len);
            ++skipped;
            break;
        }

        uint8_t tx[21] = {0};
        uint8_t rx[21] = {0};
        for (size_t k = 0; k < tx_len; ++k) {
            tx[k] = stream[i + k].tx;
            rx[k] = stream[i + k].rx;
        }

        ++transactions;

        if (rx[1] != queued_header) {
            fprintf(stderr,
                    "%s: FAIL txn %u stream=%zu cmd=%02X: header got %02X want %02X\n",
                    name, transactions, i, tx[1], rx[1], queued_header);
            dump_bytes("  TX: ", tx, tx_len);
            dump_bytes("  RX: ", rx, tx_len);
            ++failures;
        }

        if (s.configMode && tx[1] >= 0x40 && tx[1] <= 0x4F &&
            tx_len >= 9) {
            uint8_t reg = (uint8_t)(tx[1] - 0x40);
            uint8_t expected[6];

            if (reg < 0x10)
                memcpy(expected, queued_config[reg], 6);

            if (tx[1] == 0x40 && reg == 0) {
                expected[2] = s.button_attr[tx[3]];
            }

            if (reg < 0x10 && memcmp(rx + 3, expected, 6) != 0) {
                fprintf(stderr,
                        "%s: FAIL txn %u stream=%zu cmd=%02X: config payload mismatch\n",
                        name, transactions, i, tx[1]);
                dump_bytes("  TX: ", tx, tx_len);
                dump_bytes("  captured: ", rx + 3, 6);
                dump_bytes("  expected: ", expected, 6);
                ++failures;
            }
        }

        psx_test_state_t before = s;
        PSX_SPI_PROCESS_COMMAND(&s, tx);

        print_state_change(name, stream[i].capture_id, tx[1], &before, &s);

        queued_header = PSX_SPI_RESPONSE_HEADER(&s);
        for (int r = 0; r < 0x10; ++r)
            memcpy(queued_config[r], s.config_responses[r], 6);

        i += tx_len;
    }

    printf("Summary: %u transactions, final config=%u analog=%u len=%u mask=%02X %02X %02X",
           transactions,
           s.configMode,
           s.config_responses[0x05][2] == 1,
           psx_spi_current_report_len(&s),
           s.report_mask[0], s.report_mask[1], s.report_mask[2]);
    if (skipped)
        printf(" (%u non-command bytes skipped)", skipped);
    putchar('\n');

    if (failures) {
        fprintf(stderr, "%s: %u CSV replay checks failed\n",
                name, failures);
        return 1;
    }

    printf("PASS: %s parser/state replay\n", name);
    return 0;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1
        ? argv[1]
        : "tests/testing-data.csv";

    captured_byte_t *wireless = NULL;
    captured_byte_t *guitar = NULL;
    size_t wireless_n = 0;
    size_t guitar_n = 0;

    if (load_capture_columns(path, 4, 5, &guitar, &guitar_n) != 0 ||
        load_capture_columns(path, 6, 7, &wireless, &wireless_n) != 0) {
        free(guitar);
        free(wireless);
        return 1;
    }

    int rc_guitar = replay_controller(
        "Guitar Hero", guitar, guitar_n, PSX_TEST_GUITAR_HERO_GUITAR);

    int rc_wireless = replay_controller(
        "Wireless", wireless, wireless_n, PSX_TEST_DS2);

    free(guitar);
    free(wireless);

    return rc_guitar || rc_wireless;
}
