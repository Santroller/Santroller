#include "firmware_decompress.h"
#include <uzlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct uzlib_uncomp firmware_decompressor;

uint8_t* decompress_firmware(const uint8_t *compressed_data, uint32_t compressed_len, uint32_t decompressed_len)
{
    if (!compressed_data || compressed_len == 0 || decompressed_len == 0) {
        printf("Invalid firmware decompression input\n");
        return NULL;
    }

    uint8_t *decompressed = (uint8_t*)malloc(decompressed_len);
    if (!decompressed) {
        printf("Failed to allocate %lu bytes for firmware decompression\n", (unsigned long)decompressed_len);
        return NULL;
    }
    
    struct uzlib_uncomp *d = &firmware_decompressor;
    memset(d, 0, sizeof(*d));
    uzlib_uncompress_init(d, NULL, 0);
    
    d->source = compressed_data;
    d->source_limit = compressed_data + compressed_len;
    
    int res = uzlib_gzip_parse_header(d);
    if (res != TINF_OK) {
        printf("Failed to parse gzip header: %d\n", res);
        free(decompressed);
        return NULL;
    }
    
    d->dest = decompressed;
    d->dest_limit = decompressed + decompressed_len;
    
    res = uzlib_uncompress(d);
    if (res != TINF_OK) {
        printf("Decompression failed: %d\n", res);
        free(decompressed);
        return NULL;
    }
    
    uint32_t actual_len = d->dest - decompressed;
    if (actual_len != decompressed_len) {
        printf("Warning: decompressed %lu bytes, expected %lu\n", 
               (unsigned long)actual_len, (unsigned long)decompressed_len);
    }
    
    printf("Decompressed firmware: %lu -> %lu bytes\n", 
           (unsigned long)compressed_len, (unsigned long)actual_len);
    
    return decompressed;
}
