#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

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
    if (!s->config_responses[0x05][2]) {
        s->report_len = 2;
        return;
    }

    unsigned n = 0;
    for (unsigned i = 0; i < 18; ++i)
        if (s->report_mask[i / 8] & (1u << (i % 8)))
            ++n;

    s->report_len = (uint8_t)n;
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

static int load_wireless_columns(const char *path,
                                 uint8_t **cmd_out, size_t *cmd_n,
                                 uint8_t **rx_out, size_t *rx_n)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        return -1;
    }

    size_t cap = 1024, n = 0;
    uint8_t *cmd = malloc(cap);
    uint8_t *rx = malloc(cap);
    if (!cmd || !rx) {
        fclose(f);
        free(cmd);
        free(rx);
        return -1;
    }

    char line[4096];

    while (fgets(line, sizeof(line), f)) {
        char fc[256], fd[256];
        uint8_t cb, db;

        /*
         * CSV columns:
         *   0/1 dual shock
         *   2/3 blank
         *   4/5 guitar hero
         *   6/7 wireless
         */
        if (!csv_field(line, 6, fc, sizeof(fc)) ||
            !csv_field(line, 7, fd, sizeof(fd)))
            continue;

        if (!hexbyte_after_colon(fc, &cb) ||
            !hexbyte_after_colon(fd, &db))
            continue;

        if (n == cap) {
            cap *= 2;
            uint8_t *nc = realloc(cmd, cap);
            uint8_t *nr = realloc(rx, cap);
            if (!nc || !nr) {
                free(nc);
                free(nr);
                free(cmd);
                free(rx);
                fclose(f);
                return -1;
            }
            cmd = nc;
            rx = nr;
        }

        cmd[n] = cb;
        rx[n] = db;
        ++n;
    }

    fclose(f);
    *cmd_out = cmd;
    *rx_out = rx;
    *cmd_n = n;
    *rx_n = n;
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

int main(int argc, char **argv)
{
    const char *path = argc > 1
        ? argv[1]
        : "tests/cmd-data on startup_aligned - Sheet1.csv";

    uint8_t *stream_cmd = NULL, *stream_rx = NULL;
    size_t ncmd = 0, nrx = 0;

    if (load_wireless_columns(path, &stream_cmd, &ncmd, &stream_rx, &nrx) != 0)
        return 1;

    psx_test_state_t s;
    PSX_SPI_PROTOCOL_INIT(&s);

    unsigned transactions = 0;
    unsigned failures = 0;
    unsigned skipped = 0;

    /*
     * The capture is byte-aligned, with the response header appearing in
     * the RX column on the same byte position as the command opcode.
     *
     * A normal captured command is eight TX bytes:
     *   01 CMD ARG0 ARG1 ARG2 ARG3 ARG4 ARG5
     *
     * Therefore RX for that transaction is stream_rx[i+1 .. i+8].
     */
    for (size_t i = 0; i + 8 < ncmd; ) {
        if (stream_cmd[i] != 0x01 ||
            !is_ps2_command(stream_cmd[i + 1])) {
            ++i;
            ++skipped;
            continue;
        }

        uint8_t tx[8];
        memcpy(tx, stream_cmd + i, sizeof(tx));

        uint8_t rx[8];
        memcpy(rx, stream_rx + i + 1, sizeof(rx));

        ++transactions;

        uint8_t expected_header = PSX_SPI_RESPONSE_HEADER(&s);
        if (rx[0] != expected_header) {
            fprintf(stderr,
                    "FAIL txn %u stream=%zu cmd=%02X: header got %02X want %02X\n",
                    transactions, i, tx[1], rx[0], expected_header);
            dump_bytes("  TX: ", tx, 8);
            dump_bytes("  RX: ", rx, 8);
            fprintf(stderr,
                    "  state before: config=%u analog=%u len=%u mask=%02X %02X %02X\n",
                    s.configMode,
                    s.config_responses[0x05][2] == 1,
                    s.report_len,
                    s.report_mask[0], s.report_mask[1], s.report_mask[2]);
            ++failures;
        }

        /*
         * Config responses are six bytes following the response header.
         * The capture has the same six bytes at RX[1..6].
         */
        if (s.configMode && tx[1] >= 0x40 && tx[1] <= 0x4F) {
            uint8_t reg = (uint8_t)(tx[1] - 0x40);
            if (reg < 0x10) {
                if (memcmp(rx + 1, s.config_responses[reg], 6) != 0) {
                    fprintf(stderr,
                            "FAIL txn %u stream=%zu cmd=%02X: config payload mismatch\n",
                            transactions, i, tx[1]);
                    dump_bytes("  TX: ", tx, 8);
                    dump_bytes("  captured: ", rx + 1, 6);
                    dump_bytes("  expected: ", s.config_responses[reg], 6);
                    ++failures;
                }
            }
        }

        PSX_SPI_PROCESS_COMMAND(&s, tx);
        report_len_from_mask(&s);

        /*
         * Do not advance by a guessed protocol length. The startup capture
         * is explicitly aligned in rows, and every command record we consume
         * here is the eight TX-byte record beginning at 01.
         */
        i += 8;
    }

    printf("Parsed %u Wireless transactions from %zu aligned bytes",
           transactions, ncmd);
    if (skipped)
        printf(" (%u non-command bytes skipped)", skipped);
    putchar('\n');

    printf("Final state: config=%u analog=%u len=%u mask=%02X %02X %02X\n",
           s.configMode,
           s.config_responses[0x05][2] == 1,
           s.report_len,
           s.report_mask[0], s.report_mask[1], s.report_mask[2]);

    free(stream_cmd);
    free(stream_rx);

    if (failures) {
        fprintf(stderr, "%u CSV replay checks failed\n", failures);
        return 1;
    }

    puts("PASS: Wireless CSV parser/state replay");
    return 0;
}
