#include <pico/unique_id.h>
#include <stdint.h>
#include <Usb.h>
#define SERIAL_LEN (PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2)

static inline uint16_t generateSerialString(STRING_DESCRIPTOR_PICO* const UnicodeString) {
    char id[SERIAL_LEN];
    pico_get_unique_board_id_string(id, SERIAL_LEN);
    for (int i = 0; i < SERIAL_LEN; i++) {
        UnicodeString->UnicodeString[i] = id[i];
    }
    return sizeof(id);
}