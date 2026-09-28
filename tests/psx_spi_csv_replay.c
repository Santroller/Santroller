#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct {
    unsigned capture_id;
    uint8_t tx;
    uint8_t rx;
} captured_byte_t;

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
                                 captured_byte_t **stream_out, size_t *stream_n)
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

        if (!csv_field(line, 6, fc, sizeof(fc)) ||
            !csv_field(line, 7, fd, sizeof(fd)))
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

int main(int argc, char **argv)
{
    const char *path = argc > 1
        ? argv[1]
        : "tests/cmd-data on startup_aligned - Sheet1.csv";

    captured_byte_t *stream = NULL;
    size_t nstream = 0;

    if (load_wireless_columns(path, &stream, &nstream) != 0)
        return 1;

    psx_test_state_t s;
    PSX_SPI_PROTOCOL_INIT(&s);

    unsigned transactions = 0;
    unsigned failures = 0;
    unsigned skipped = 0;

    /*
     * Command length is determined by the protocol state, not by searching
     * for 0x01: 0x01 can legitimately occur inside a 0x79 report payload.
     */
    /*
     * Model the PIO write-buffer pipeline explicitly.  The response seen
     * during transaction N was selected before the CPU processed command N.
     * Therefore keep a snapshot for transaction N, validate it first, then
     * process the command to prepare transaction N+1.
     */
    uint8_t queued_header = PSX_SPI_RESPONSE_HEADER(&s);
    uint8_t queued_config[0x10][6];
    memset(queued_config, 0, sizeof(queued_config));
    for (int r = 0; r < 0x10; ++r)
        memcpy(queued_config[r], s.config_responses[r], 6);

    size_t i = 0;
    while (i + 1 < nstream) {
        if (stream[i].tx != 0x01 ||
            !is_ps2_command(stream[i + 1].tx)) {
            ++i;
            ++skipped;
            continue;
        }

        size_t tx_len;
        /*
         * Transfer length belongs to the transaction, not the opcode.
         * In normal mode the console clocks the current report length even
         * when the command is 43/40/etc.  Config-mode transactions are 9
         * bytes.
         */
        /* Find the next capture record beginning a PS2 command. */
        size_t next = i + 1;
        while (next < nstream &&
               !(stream[next].tx == 0x01 &&
                 next + 1 < nstream &&
                 is_ps2_command(stream[next + 1].tx)))
            ++next;

        tx_len = next - i;
        if (tx_len == 0 || tx_len > 21)
            tx_len = s.configMode ? 9 : (size_t)(3u + s.report_len);

        if (i + tx_len > nstream) {
            fprintf(stderr,
                    "SKIP truncated txn at stream=%zu cmd=%02X len=%zu\\n",
                    i, stream[i + 1].tx, tx_len);
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
                    "FAIL txn %u stream=%zu cmd=%02X: header got %02X want %02X\\n",
                    transactions, i, tx[1], rx[1], queued_header);
            dump_bytes("  TX: ", tx, tx_len);
            dump_bytes("  RX: ", rx, tx_len);
            fprintf(stderr,
                    "  state before: config=%u analog=%u len=%u mask=%02X %02X %02X\\n",
                    s.configMode,
                    s.config_responses[0x05][2] == 1,
                    s.report_len,
                    s.report_mask[0], s.report_mask[1], s.report_mask[2]);
            ++failures;
        }

        /*
         * Config transactions are byte-pipelined.  The F3 header is already
         * queued when the transfer begins, but the command byte itself
         * selects/mutates the six-byte register payload while the remaining
         * bytes are being clocked.  Therefore validate the header now, but
         * validate the register payload against the post-command state below.
         */
        uint8_t config_reg = 0xFF;
        if (s.configMode && tx[1] >= 0x40 && tx[1] <= 0x4F &&
            tx_len >= 9)
            config_reg = (uint8_t)(tx[1] - 0x40);

        PSX_SPI_PROCESS_COMMAND(&s, tx);
        report_len_from_mask(&s);

        if (config_reg < 0x10) {
            uint8_t expected[6];
            memcpy(expected, s.config_responses[config_reg], 6);

            if (memcmp(rx + 3, expected, 6) != 0) {
                fprintf(stderr,
                        "FAIL txn %u stream=%zu cmd=%02X: config payload mismatch\\n",
                        transactions, i, tx[1]);
                dump_bytes("  TX: ", tx, tx_len);
                dump_bytes("  captured: ", rx + 3, 6);
                dump_bytes("  expected: ", expected, 6);
                ++failures;
            }
        }

        /*
         * CPU-side command processing is now complete.  This is what the
         * PIO/CPU path can expose on the following SPI transaction.
         */
        queued_header = PSX_SPI_RESPONSE_HEADER(&s);
        for (int r = 0; r < 0x10; ++r)
            memcpy(queued_config[r], s.config_responses[r], 6);

        i += tx_len;
    }

    printf("Parsed %u Wireless transactions from %zu aligned bytes",
           transactions, nstream);
    if (skipped)
        printf(" (%u non-command bytes skipped)", skipped);
    putchar('\n');

    printf("Final state: config=%u analog=%u len=%u mask=%02X %02X %02X\n",
           s.configMode,
           s.config_responses[0x05][2] == 1,
           s.report_len,
           s.report_mask[0], s.report_mask[1], s.report_mask[2]);

    free(stream);

    if (failures) {
        fprintf(stderr, "%u CSV replay checks failed\n", failures);
        return 1;
    }

    puts("PASS: Wireless CSV parser/state replay");
    return 0;
}
