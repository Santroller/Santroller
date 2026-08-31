#ifndef GIP_CRYPTO_H
#define GIP_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

int gip_crypto_init(void);
void gip_crypto_deinit(void);

int gip_crypto_random_bytes(uint8_t *output, size_t len);

int gip_crypto_rsa_encrypt(const uint8_t *pubkey, uint16_t pubkey_len,
                           const uint8_t *input, uint16_t input_len,
                           uint8_t *output, uint16_t output_len);

int gip_crypto_ecdh_generate_keypair(uint8_t *pubkey, uint16_t *pubkey_len);

int gip_crypto_ecdh_compute_shared(const uint8_t *peer_pubkey, uint16_t peer_pubkey_len,
                                   uint8_t *shared_secret, uint16_t *secret_len);

int gip_crypto_prf(const char *label,
                   const uint8_t *secret, uint16_t secret_len,
                   const uint8_t *seed, uint16_t seed_len,
                   uint8_t *output, uint16_t output_len);

int gip_crypto_sha256(const uint8_t *input, uint16_t input_len,
                      uint8_t *output);

int gip_crypto_hmac_sha256(const uint8_t *key, uint16_t key_len,
                           const uint8_t *input, uint16_t input_len,
                           uint8_t *output);

#endif
