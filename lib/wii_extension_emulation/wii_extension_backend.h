#ifndef WII_EXTENSION_BACKEND_H
#define WII_EXTENSION_BACKEND_H

#include <stdbool.h>
#include <stdint.h>
#include "wm_crypto.h"

enum wii_extension_backend_type {
    WII_EXTENSION_NUNCHUK = 0,
    WII_EXTENSION_CLASSIC = 1,
    WII_EXTENSION_GUITAR = 2,
    WII_EXTENSION_DRUMS = 3,
    WII_EXTENSION_TURNTABLE = 4,
    WII_EXTENSION_BALANCE_BOARD = 5,
};

#ifdef __cplusplus
extern "C" {
#endif

void wii_extension_backend_init(uint8_t *registers, bool *encrypted, uint8_t extension_type);
uint8_t wii_extension_backend_read(const uint8_t *registers, bool encrypted,
                                   const struct ext_crypto_state *crypto, uint8_t offset);
uint8_t wii_extension_backend_write(uint8_t *registers, bool *encrypted,
                                    struct ext_crypto_state *crypto, uint8_t offset,
                                    uint8_t value);
void wii_extension_backend_generate_tables(const uint8_t *registers,
                                            struct ext_crypto_state *crypto,
                                            bool *encrypted);

#ifdef __cplusplus
}
#endif

#endif
