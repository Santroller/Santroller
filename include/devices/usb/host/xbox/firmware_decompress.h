#ifndef FIRMWARE_DECOMPRESS_H
#define FIRMWARE_DECOMPRESS_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decompress gzip-compressed firmware data
 * @param compressed_data Pointer to compressed data
 * @param compressed_len Length of compressed data
 * @param decompressed_len Expected length of decompressed data
 * @return Pointer to decompressed data (must be freed by caller) or NULL on error
 */
uint8_t* decompress_firmware(const uint8_t *compressed_data, uint32_t compressed_len, uint32_t decompressed_len);

#ifdef __cplusplus
}
#endif

#endif // FIRMWARE_DECOMPRESS_H
