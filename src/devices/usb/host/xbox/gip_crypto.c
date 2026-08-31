#include "gip_crypto.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#ifdef HAVE_MBEDTLS
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_ecdh_context ecdh_ctx;
static bool ecdh_initialized = false;
#endif

static bool crypto_initialized = false;

int gip_crypto_init(void) {
    if (crypto_initialized) {
        return 0;
    }
    
    printf("GIP Crypto: Initializing\n");
    
#ifdef HAVE_MBEDTLS
    const char *pers = "gip_auth";
    int ret;
    
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        printf("GIP Crypto: Failed to seed RNG: -0x%04X\n", -ret);
        return -1;
    }
    
    printf("GIP Crypto: mbedTLS initialized successfully\n");
#else
    printf("GIP Crypto: WARNING - mbedTLS not available, using placeholders\n");
#endif
    
    crypto_initialized = true;
    return 0;
}

void gip_crypto_deinit(void) {
#ifdef HAVE_MBEDTLS
    if (ecdh_initialized) {
        mbedtls_ecdh_free(&ecdh_ctx);
        ecdh_initialized = false;
    }
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
#endif
    crypto_initialized = false;
}

int gip_crypto_random_bytes(uint8_t *output, size_t len) {
    if (!output || len == 0) {
        return -1;
    }
    
#ifdef HAVE_MBEDTLS
    int ret = mbedtls_ctr_drbg_random(&ctr_drbg, output, len);
    return ret;
#else
    for (size_t i = 0; i < len; i++) {
        output[i] = (uint8_t)(0xAA + i);
    }
    return 0;
#endif
}

static void xor_bytes(uint8_t *dest, const uint8_t *src, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        dest[i] ^= src[i];
    }
}

int gip_crypto_sha256(const uint8_t *input, uint16_t input_len, uint8_t *output) {
#ifdef HAVE_MBEDTLS
    mbedtls_sha256_context ctx;
    int ret;
    
    mbedtls_sha256_init(&ctx);
    ret = mbedtls_sha256_starts(&ctx, 0);  // 0 = SHA-256 (not SHA-224)
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);
        return -1;
    }
    
    ret = mbedtls_sha256_update(&ctx, input, input_len);
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);
        return -1;
    }
    
    ret = mbedtls_sha256_finish(&ctx, output);
    mbedtls_sha256_free(&ctx);
    
    return (ret == 0) ? 0 : -1;
#else
    printf("GIP Crypto: SHA256 not available (mbedTLS not compiled)\n");
    memset(output, 0, 32);
    return -1;
#endif
}

int gip_crypto_hmac_sha256(const uint8_t *key, uint16_t key_len,
                           const uint8_t *input, uint16_t input_len,
                           uint8_t *output) {
#ifdef HAVE_MBEDTLS
    const mbedtls_md_info_t *md_info;
    int ret;
    
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        return -1;
    }
    
    ret = mbedtls_md_hmac(md_info, key, key_len, input, input_len, output);
    return (ret == 0) ? 0 : -1;
#else
    printf("GIP Crypto: HMAC-SHA256 not available (mbedTLS not compiled)\n");
    memset(output, 0, 32);
    return -1;
#endif
}

int gip_crypto_prf(const char *label,
                   const uint8_t *secret, uint16_t secret_len,
                   const uint8_t *seed, uint16_t seed_len,
                   uint8_t *output, uint16_t output_len) {
    uint8_t a[32];
    uint8_t temp[32];
    uint16_t label_len = strlen(label);
    uint16_t pos = 0;
    
#ifdef HAVE_MBEDTLS
    printf("GIP Crypto: PRF(\"%s\")\n", label);
#else
    printf("GIP Crypto: PRF(\"%s\") - WARNING: using placeholder HMAC\n", label);
#endif
    
    uint8_t combined_seed[256];
    uint16_t combined_len = 0;
    
    memcpy(combined_seed, label, label_len);
    combined_len += label_len;
    memcpy(combined_seed + combined_len, seed, seed_len);
    combined_len += seed_len;
    
    if (gip_crypto_hmac_sha256(secret, secret_len, combined_seed, combined_len, a) < 0) {
        return -1;
    }
    
    while (pos < output_len) {
        uint8_t hmac_input[64];
        uint16_t hmac_len = 0;
        
        memcpy(hmac_input, a, 32);
        hmac_len += 32;
        memcpy(hmac_input + hmac_len, combined_seed, combined_len);
        hmac_len += combined_len;
        
        if (gip_crypto_hmac_sha256(secret, secret_len, hmac_input, hmac_len, temp) < 0) {
            return -1;
        }
        
        uint16_t to_copy = (output_len - pos < 32) ? (output_len - pos) : 32;
        memcpy(output + pos, temp, to_copy);
        pos += to_copy;
        
        if (pos < output_len) {
            if (gip_crypto_hmac_sha256(secret, secret_len, a, 32, a) < 0) {
                return -1;
            }
        }
    }
    
    return 0;
}

int gip_crypto_rsa_encrypt(const uint8_t *pubkey, uint16_t pubkey_len,
                           const uint8_t *input, uint16_t input_len,
                           uint8_t *output, uint16_t output_len) {
#ifdef HAVE_MBEDTLS
    mbedtls_pk_context pk;
    mbedtls_rsa_context *rsa;
    int ret;
    
    mbedtls_pk_init(&pk);
    
    ret = mbedtls_pk_parse_public_key(&pk, pubkey, pubkey_len);
    if (ret != 0) {
        printf("GIP Crypto: Failed to parse RSA public key: -0x%04X\n", -ret);
        mbedtls_pk_free(&pk);
        return -1;
    }
    
    if (!mbedtls_pk_can_do(&pk, MBEDTLS_PK_RSA)) {
        printf("GIP Crypto: Key is not RSA\n");
        mbedtls_pk_free(&pk);
        return -1;
    }
    
    rsa = mbedtls_pk_rsa(pk);
    mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, 0);
    
    ret = mbedtls_rsa_pkcs1_encrypt(rsa, mbedtls_ctr_drbg_random, &ctr_drbg,
                                    input_len, input, output);
    
    mbedtls_pk_free(&pk);
    
    if (ret != 0) {
        printf("GIP Crypto: RSA encryption failed: -0x%04X\n", -ret);
        return -1;
    }
    
    printf("GIP Crypto: RSA encryption successful\n");
    return 0;
#else
    printf("GIP Crypto: RSA not available (mbedTLS not compiled)\n");
    memset(output, 0xAA, output_len);
    return -1;
#endif
}

int gip_crypto_ecdh_generate_keypair(uint8_t *public_key, uint16_t *public_key_len) {
    if (!public_key || !public_key_len) {
        return -1;
    }
    
    #ifdef HAVE_MBEDTLS
        int ret;
        size_t olen;
        
        // Use mbedTLS v3 ECDH API
        ret = mbedtls_ecdh_setup(&ecdh_ctx, MBEDTLS_ECP_DP_SECP256R1);
        if (ret != 0) {
            printf("Failed to setup ECDH context: %d\n", ret);
            return ret;
        }
        
        ret = mbedtls_ecdh_make_public(&ecdh_ctx, &olen,
                                       public_key, *public_key_len,
                                       mbedtls_ctr_drbg_random,
                                       &ctr_drbg);
        if (ret != 0) {
            printf("Failed to generate ECDH keypair: %d\n", ret);
            return ret;
        }
        
        ecdh_initialized = true;
        *public_key_len = (uint16_t)olen;
        return 0;
    #else
        memset(public_key, 0xBB, 64);
        *public_key_len = 64;
        return 0;
    #endif
}

int gip_crypto_ecdh_compute_shared(const uint8_t *peer_public_key, uint16_t peer_public_key_len,
                                   uint8_t *shared_secret, uint16_t *shared_secret_len) {
    if (!peer_public_key || !shared_secret || !shared_secret_len) {
        return -1;
    }
    
    #ifdef HAVE_MBEDTLS
        if (!ecdh_initialized) {
            return -1;
        }
        
        int ret;
        size_t olen;
        
        // Use mbedTLS v3 ECDH API
        ret = mbedtls_ecdh_read_public(&ecdh_ctx, peer_public_key, peer_public_key_len);
        if (ret != 0) {
            printf("Failed to read peer public key: %d\n", ret);
            return ret;
        }
        
        ret = mbedtls_ecdh_calc_secret(&ecdh_ctx, &olen,
                                       shared_secret, *shared_secret_len,
                                       mbedtls_ctr_drbg_random,
                                       &ctr_drbg);
        if (ret != 0) {
            printf("Failed to compute shared secret: %d\n", ret);
            return ret;
        }
        
        *shared_secret_len = (uint16_t)olen;
        return 0;
    #else
        memset(shared_secret, 0xCC, 32);
        *shared_secret_len = 32;
        return 0;
    #endif
}
