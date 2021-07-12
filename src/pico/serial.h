#include <pico/unique_id.h>
#include <stdint.h>
#define SERIAL_LEN (2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1)
static inline uint16_t generateSerialString(uint16_t* const UnicodeString) {
    char id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(id, sizeof(id));
    for (int i = 0; i < sizeof(id); i++) {
        UnicodeString[i] = id[i];
    }
    return sizeof(id);
}