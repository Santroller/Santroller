#include "pico/cyw43_driver.h"

#ifdef USE_COMPRESSED_CYW43_FIRMWARE

#include "cyw43_firmware_compressed.h"
#include <uzlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Decompressed firmware buffer (allocated once, freed never since it's needed for WiFi)
static uint8_t *decompressed_firmware = NULL;
uint32_t cyw43_firmware_decompressed_len = 0;
static struct uzlib_uncomp firmware_decompressor;

// Ensure firmware is decompressed (called before any firmware access)
void cyw43_ensure_firmware_decompressed(void) {
    if (!decompressed_firmware) {
        printf("Decompressing CYW43 firmware (%lu -> %lu bytes)...\n",
               (unsigned long)cyw43_firmware_compressed_len,
               (unsigned long)cyw43_firmware_original_len);
        
        // Allocate buffer
        decompressed_firmware = (uint8_t*)malloc(cyw43_firmware_original_len);
        if (!decompressed_firmware) {
            printf("ERROR: Failed to allocate %lu bytes for CYW43 firmware\n",
                   (unsigned long)cyw43_firmware_original_len);
            return;
        }
        
        // Initialize decompressor
        struct uzlib_uncomp *d = &firmware_decompressor;
        memset(d, 0, sizeof(*d));
        uzlib_uncompress_init(d, NULL, 0);
        
        // Set up input
        d->source = cyw43_firmware_compressed;
        d->source_limit = cyw43_firmware_compressed + cyw43_firmware_compressed_len;
        
        // Parse gzip header
        int res = uzlib_gzip_parse_header(d);
        if (res != TINF_OK) {
            printf("ERROR: Failed to parse CYW43 firmware gzip header: %d\n", res);
            free(decompressed_firmware);
            decompressed_firmware = NULL;
            return;
        }
        
        // Set up output
        d->dest = decompressed_firmware;
        d->dest_limit = decompressed_firmware + cyw43_firmware_original_len;
        
        // Decompress
        res = uzlib_uncompress(d);
        if (res != TINF_OK) {
            printf("ERROR: CYW43 firmware decompression failed: %d\n", res);
            free(decompressed_firmware);
            decompressed_firmware = NULL;
            return;
        }
        
        printf("CYW43 firmware decompressed successfully\n");
        cyw43_firmware_decompressed_len = cyw43_firmware_original_len;
    }
}

// Get pointer to decompressed firmware
const unsigned char *cyw43_get_decompressed_firmware(void) {
    return decompressed_firmware;
}

// BT firmware decompression
static uint8_t *decompressed_btfw = NULL;
uint32_t cyw43_btfw_decompressed_len = 0;
static struct uzlib_uncomp btfw_decompressor;

void cyw43_ensure_btfw_decompressed(void) {
    if (!decompressed_btfw) {
        printf("Decompressing CYW43 BT firmware (%lu -> %lu bytes)...\n",
               (unsigned long)cyw43_btfw_compressed_len,
               (unsigned long)cyw43_btfw_original_len);
        
        decompressed_btfw = (uint8_t*)malloc(cyw43_btfw_original_len);
        if (!decompressed_btfw) {
            printf("ERROR: Failed to allocate %lu bytes for BT firmware\n",
                   (unsigned long)cyw43_btfw_original_len);
            return;
        }
        
        struct uzlib_uncomp *d = &btfw_decompressor;
        memset(d, 0, sizeof(*d));
        uzlib_uncompress_init(d, NULL, 0);
        
        d->source = cyw43_btfw_compressed;
        d->source_limit = cyw43_btfw_compressed + cyw43_btfw_compressed_len;
        
        int res = uzlib_gzip_parse_header(d);
        if (res != TINF_OK) {
            printf("ERROR: Failed to parse BT firmware gzip header: %d\n", res);
            free(decompressed_btfw);
            decompressed_btfw = NULL;
            return;
        }
        
        d->dest = decompressed_btfw;
        d->dest_limit = decompressed_btfw + cyw43_btfw_original_len;
        
        res = uzlib_uncompress(d);
        if (res != TINF_OK) {
            printf("ERROR: BT firmware decompression failed: %d\n", res);
            free(decompressed_btfw);
            decompressed_btfw = NULL;
            return;
        }
        
        printf("CYW43 BT firmware decompressed successfully\n");
        cyw43_btfw_decompressed_len = cyw43_btfw_original_len;
    }
}

const unsigned char *cyw43_get_decompressed_btfw(void) {
    return decompressed_btfw;
}

#endif // USE_COMPRESSED_CYW43_FIRMWARE
