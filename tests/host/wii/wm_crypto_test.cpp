// Wii extension encryption (lib/wm_crypto), against wiibrew's Wiimote/Extension_Controllers page:
//
//   decrypted_byte = (encrypted_byte XOR table1[address % 8]) + table2[address % 8]
//
//   "If the host key is 16 zero bytes, table1[x] and table2[x] are all 0x97. The value 0x17 was
//   used previously, which is equivalent."
//
// The page's identification table also lists the encrypted ID bytes 0xFE / 0xFF read with the
// all zero key, which give known plaintext / ciphertext pairs.
//
// wiibrew doesn't publish vectors for real (non zero) keys, so those come from Dolphin's
// Source/Core/Core/HW/WiimoteEmu/Encryption.cpp (KeyGen1stParty, the same marcan derived
// algorithm, with S-boxes that are all permutations of 0-255): a random 10 byte "rand" and an
// idx give the 6 byte key the Wii would send, and the ft / sb tables a real extension derives.
// The 16 bytes written to 0x40 are rand reversed, then key reversed.
#include <gtest/gtest.h>
#include <stdint.h>
#include <string.h>
#include <random>
#include "wm_crypto.h"

namespace
{
ext_crypto_state tables_for(const uint8_t key[16])
{
    ext_crypto_state state;
    memset(&state, 0xAA, sizeof(state));
    ext_generate_tables(&state, key);
    return state;
}

const uint8_t zero_key[16] = {};
} // namespace

// No idx matches an all zero key (it isn't one the key generation makes), so ext_generate_tables
// (lib/wm_crypto/wm_crypto.c) special cases it and gives the 0x97 tables wiibrew documents. The
// firmware's own extension reader (lib/wii_extensions) sends this key and decrypts with 0x97.
TEST(WmCrypto, AllZeroKeyGivesTablesOf0x97)
{
    ext_crypto_state state = tables_for(zero_key);
    for (int i = 0; i < 8; i++)
    {
        EXPECT_EQ(state.ft[i], 0x97) << i;
        EXPECT_EQ(state.sb[i], 0x97) << i;
    }
}

// see AllZeroKeyGivesTablesOf0x97
TEST(WmCrypto, ZeroKeyDecryptIsTheOld0x17Transform)
{
    // Older documentation: with a zero key, decrypted = (encrypted ^ 0x17) + 0x17
    ext_crypto_state state = tables_for(zero_key);
    for (int addr = 0; addr < 8; addr++)
    {
        for (int value = 0; value < 256; value++)
        {
            uint8_t byte = (uint8_t)value;
            ext_decrypt_bytes(&state, &byte, addr, 1);
            EXPECT_EQ(byte, (uint8_t)(((uint8_t)value ^ 0x17) + 0x17)) << addr << " " << value;
        }
    }
}

namespace
{
// Encrypted extension type bytes (0xFE, 0xFF) from wiibrew's identification table, read
// after the old style zero key initialisation
struct EncryptedId
{
    const char *name;
    uint8_t plain[2];
    uint8_t encrypted[2];
};
const EncryptedId encrypted_ids[] = {
    {"Nunchuk", {0x00, 0x00}, {0xFE, 0xFE}},
    {"Classic Controller", {0x01, 0x01}, {0xFD, 0xFD}},
    {"Drawsome tablet", {0x00, 0x13}, {0xFE, 0xEB}},
    {"Guitar Hero guitar / drums", {0x01, 0x03}, {0xFD, 0xFB}},
};
} // namespace

// see AllZeroKeyGivesTablesOf0x97
TEST(WmCrypto, ZeroKeyEncryptsExtensionIdsLikeRealExtensions)
{
    ext_crypto_state state = tables_for(zero_key);
    for (const auto &id : encrypted_ids)
    {
        uint8_t bytes[2] = {id.plain[0], id.plain[1]};
        ext_encrypt_bytes(&state, bytes, 0xFE, 2);
        EXPECT_EQ(bytes[0], id.encrypted[0]) << id.name;
        EXPECT_EQ(bytes[1], id.encrypted[1]) << id.name;
    }
}

// see AllZeroKeyGivesTablesOf0x97
TEST(WmCrypto, ZeroKeyDecryptsExtensionIdsLikeAWiimote)
{
    ext_crypto_state state = tables_for(zero_key);
    for (const auto &id : encrypted_ids)
    {
        uint8_t bytes[2] = {id.encrypted[0], id.encrypted[1]};
        ext_decrypt_bytes(&state, bytes, 0xFE, 2);
        EXPECT_EQ(bytes[0], id.plain[0]) << id.name;
        EXPECT_EQ(bytes[1], id.plain[1]) << id.name;
    }
}

TEST(WmCrypto, DecryptIsWiibrewsTransformWithTheGeneratedTables)
{
    // table1 is the XOR table (sb), table2 the add table (ft)
    const uint8_t key[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                             0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x01};
    ext_crypto_state state = tables_for(key);
    uint8_t buffer[16];
    for (int i = 0; i < 16; i++)
    {
        buffer[i] = (uint8_t)(i * 37 + 5);
    }
    uint8_t original[16];
    memcpy(original, buffer, sizeof(buffer));
    ext_decrypt_bytes(&state, buffer, 0x08, sizeof(buffer));
    for (int i = 0; i < 16; i++)
    {
        int addr = 0x08 + i;
        EXPECT_EQ(buffer[i], (uint8_t)((original[i] ^ state.sb[addr % 8]) + state.ft[addr % 8])) << i;
    }
}

TEST(WmCrypto, EncryptThenDecryptRoundTripsForAnyKeyAndOffset)
{
    std::mt19937 rng(1234);
    for (int round = 0; round < 200; round++)
    {
        uint8_t key[16];
        for (auto &k : key)
        {
            k = (uint8_t)rng();
        }
        ext_crypto_state state = tables_for(key);
        int offset = (int)(rng() % 256);
        uint8_t plain[21];
        for (auto &p : plain)
        {
            p = (uint8_t)rng();
        }
        uint8_t buffer[sizeof(plain)];
        memcpy(buffer, plain, sizeof(plain));
        ext_encrypt_bytes(&state, buffer, offset, sizeof(buffer));
        ext_decrypt_bytes(&state, buffer, offset, sizeof(buffer));
        EXPECT_EQ(memcmp(buffer, plain, sizeof(plain)), 0) << "round " << round;
    }
}

TEST(WmCrypto, OnlyTheBottomThreeAddressBitsMatter)
{
    const uint8_t key[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04,
                             0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C};
    ext_crypto_state state = tables_for(key);
    for (int addr = 0; addr < 8; addr++)
    {
        uint8_t low = 0x5A, high = 0x5A;
        ext_encrypt_bytes(&state, &low, addr, 1);
        ext_encrypt_bytes(&state, &high, addr + 0xF8, 1);
        EXPECT_EQ(low, high) << addr;
    }
}

TEST(WmCrypto, DifferentKeysGiveDifferentTables)
{
    const uint8_t key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    ext_crypto_state zero = tables_for(zero_key);
    ext_crypto_state other = tables_for(key);
    EXPECT_NE(memcmp(&zero, &other, sizeof(zero)), 0);
}

TEST(WmCrypto, EncryptionIsABijectionPerAddress)
{
    const uint8_t key[16] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE,
                             0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    ext_crypto_state state = tables_for(key);
    for (int addr = 0; addr < 8; addr++)
    {
        bool seen[256] = {};
        for (int value = 0; value < 256; value++)
        {
            uint8_t byte = (uint8_t)value;
            ext_encrypt_bytes(&state, &byte, addr, 1);
            EXPECT_FALSE(seen[byte]) << addr << " " << value;
            seen[byte] = true;
        }
    }
}

namespace
{
// Tables for real keys, derived with Dolphin's 1st party key generation (see the top of the file)
struct KeyVector
{
    const char *name;
    int idx;
    uint8_t ext_key[16];
    uint8_t ft[8];
    uint8_t sb[8];
};

void expect_tables(const KeyVector &v)
{
    ext_crypto_state state = tables_for(v.ext_key);
    for (int i = 0; i < 8; i++)
    {
        EXPECT_EQ(state.ft[i], v.ft[i]) << v.name << " ft[" << i << "]";
        EXPECT_EQ(state.sb[i], v.sb[i]) << v.name << " sb[" << i << "]";
    }
}

const KeyVector random_keys[] = {
    {"random idx 0", 0,
     {0xD4, 0xB5, 0xFF, 0x7D, 0x87, 0xD1, 0x66, 0x9B, 0x5D, 0xF0, 0xF3, 0x3D, 0x2D, 0x83, 0x03, 0x4A},
     {0x78, 0x06, 0x54, 0x39, 0xD5, 0x78, 0xE4, 0x12},
     {0x64, 0x2F, 0x70, 0x99, 0xF5, 0xEB, 0x22, 0xCA}},
    {"random idx 1", 1,
     {0x1D, 0xD2, 0x6C, 0x4B, 0xEF, 0x69, 0x26, 0xA9, 0x9E, 0x6F, 0xFE, 0xC7, 0x43, 0xBE, 0x06, 0xC4},
     {0x77, 0x0C, 0x05, 0x64, 0xD7, 0x75, 0x8A, 0x0E},
     {0x7F, 0x32, 0xA7, 0x95, 0x3C, 0x22, 0xB4, 0xCD}},
    {"random idx 2", 2,
     {0x66, 0xB0, 0xA9, 0xC7, 0xA7, 0x47, 0x3F, 0xEE, 0xD5, 0xB2, 0x9A, 0x4E, 0x33, 0xA5, 0x50, 0x63},
     {0xF5, 0x82, 0x1D, 0x40, 0x64, 0x89, 0xF1, 0x2F},
     {0x23, 0xB9, 0x21, 0xF5, 0x7C, 0xA4, 0xE1, 0xD7}},
    {"random idx 3", 3,
     {0x73, 0x14, 0x68, 0x75, 0xD0, 0x6D, 0xA2, 0xD4, 0xDA, 0xA6, 0x5A, 0x1A, 0x78, 0xC2, 0x69, 0xED},
     {0x3D, 0x25, 0xF4, 0x87, 0xB3, 0xDD, 0x97, 0x4A},
     {0xE3, 0x8C, 0x06, 0x3A, 0x61, 0x32, 0x56, 0xE8}},
    {"random idx 4", 4,
     {0xBB, 0xED, 0x78, 0x76, 0xA9, 0x39, 0xD7, 0xA3, 0x84, 0x09, 0xBD, 0x45, 0x1A, 0x6E, 0x1F, 0xEE},
     {0xB9, 0xF5, 0x15, 0x56, 0x4B, 0x84, 0x51, 0x2B},
     {0x9E, 0x74, 0x2D, 0x6F, 0x0C, 0x75, 0xE1, 0xB4}},
    {"random idx 5", 5,
     {0xEE, 0xAB, 0xAC, 0xC6, 0x86, 0x48, 0xFC, 0xBC, 0x67, 0x45, 0xC2, 0x65, 0x65, 0xE9, 0x33, 0xD6},
     {0xCA, 0x63, 0x0C, 0xCE, 0x03, 0x87, 0x1E, 0x3F},
     {0x46, 0x1A, 0x45, 0x7C, 0x48, 0xEF, 0x0A, 0xB4}},
    {"random idx 6", 6,
     {0xE0, 0x4D, 0x02, 0x58, 0x32, 0x21, 0x69, 0xA9, 0x43, 0x56, 0x8F, 0xE1, 0xFD, 0x29, 0x1A, 0x41},
     {0xC1, 0x68, 0xA5, 0x51, 0x40, 0x3B, 0x9F, 0xCD},
     {0x18, 0x15, 0xF5, 0x94, 0x2A, 0x56, 0x7D, 0xBC}},
};
} // namespace

TEST(WmCrypto, RealKeysGiveTheSameTablesAsDolphin)
{
    for (const auto &v : random_keys)
    {
        expect_tables(v);
    }
}

namespace
{
// Keys whose rand bytes land on the four S-box entries wm_crypto.c used to get wrong
const KeyVector keys_on_bad_sbox_entries[] = {
    {"idx 2, rand[0] = 0x60 (sboxes[3][0x60])", 2,
     {0xF6, 0x86, 0xEC, 0x7A, 0x91, 0x64, 0x29, 0x75, 0xB3, 0x60, 0xCD, 0x53, 0x6A, 0xC4, 0x63, 0x85},
     {0x58, 0x82, 0xFC, 0x6A, 0x15, 0xE1, 0xED, 0x35},
     {0xF6, 0x9A, 0x7C, 0xD7, 0xEA, 0xC8, 0xDF, 0xDF}},
    {"idx 2, rand[6] = 0xD8 (sboxes[4][0xD8])", 2,
     {0x33, 0x80, 0x45, 0xD8, 0x9A, 0x9A, 0x24, 0x66, 0xD4, 0xDF, 0xAF, 0xE9, 0xD3, 0xC4, 0x98, 0x42},
     {0xBA, 0x0C, 0xAB, 0x55, 0x41, 0x10, 0xDF, 0x35},
     {0x3E, 0xC0, 0xC5, 0x7F, 0x94, 0xE8, 0x9A, 0x5A}},
    {"idx 3, rand[0] = 0xD8 (sboxes[4][0xD8])", 3,
     {0xC1, 0x17, 0xB5, 0xA3, 0x72, 0x5A, 0xC6, 0x64, 0x6F, 0xD8, 0x88, 0x27, 0x27, 0xCD, 0x17, 0x76},
     {0x43, 0xB4, 0x92, 0x2A, 0xC0, 0xD8, 0x8D, 0x65},
     {0xF8, 0x2D, 0x0D, 0x14, 0x49, 0xB4, 0x60, 0xC0}},
    {"idx 5, rand[1] = 0x88 (sboxes[6][0x88])", 5,
     {0xF9, 0x78, 0x58, 0x2C, 0x6B, 0x40, 0x1C, 0x75, 0x88, 0x25, 0xCE, 0xE2, 0x07, 0x36, 0xD2, 0xCE},
     {0xE8, 0x58, 0x2A, 0x78, 0x25, 0xAC, 0x18, 0xBF},
     {0x58, 0xA5, 0x4A, 0xD4, 0xB0, 0x71, 0xF1, 0x06}},
    {"idx 4, rand[6] = 0xEC (sboxes[6][0xEC])", 4,
     {0x00, 0xA1, 0x5C, 0xEC, 0x0F, 0x35, 0x40, 0x2C, 0x52, 0x54, 0x10, 0xC5, 0xEB, 0x89, 0x32, 0xB8},
     {0x23, 0xAD, 0x2D, 0xEE, 0xA4, 0x02, 0x93, 0xE9},
     {0xCB, 0x10, 0x93, 0x62, 0x79, 0x01, 0xFA, 0xE5}},
};
} // namespace

// sboxes[3][0x60], sboxes[4][0xD8] and sboxes[6][0x88] (0xF0) and sboxes[6][0xEC] (0xE2) in
// lib/wm_crypto/wm_crypto.c match Dolphin's Encryption.cpp, so every box is a permutation of 0-255
// and keys that look these entries up get the same tables as a real extension.
TEST(WmCrypto, KeysUsingEverySBoxEntryGiveTheSameTablesAsDolphin)
{
    for (const auto &v : keys_on_bad_sbox_entries)
    {
        expect_tables(v);
    }
}
