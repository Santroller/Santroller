#include <stdlib.h>
#include <string.h>

#include "excrypt.h"

// SHA1 code based on https://github.com/mohaps/TinySHA1


void ExCryptSha(const uint8_t *input, uint32_t input_size, uint8_t *output, uint32_t output_size)
{
  uint32_t w[80] = {0};
  uint32_t state[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
  memcpy(w, input, input_size);

  uint64_t bit_count = (uint64_t)input_size * 8;
  uint8_t *buffer = (uint8_t *)w;
  buffer[input_size] = 0x80;

  for (size_t i = 0; i < 15; i++)
  {
    w[i] = SWAP32(w[i]);
  }
  w[15] = bit_count;
  for (size_t i = 16; i < 80; i++)
  {
    w[i] = ROTL32((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);
  }

  uint32_t a = state[0];
  uint32_t b = state[1];
  uint32_t c = state[2];
  uint32_t d = state[3];
  uint32_t e = state[4];

  for (int i = 0; i < 80; ++i)
  {
    uint32_t f = 0;
    uint32_t k = 0;

    if (i < 20)
    {
      f = (b & c) | (~b & d);
      k = 0x5A827999;
    }
    else if (i < 40)
    {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1;
    }
    else if (i < 60)
    {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8F1BBCDC;
    }
    else
    {
      f = b ^ c ^ d;
      k = 0xCA62C1D6;
    }
    uint32_t temp = ROTL32(a, 5) + f + e + k + w[i];
    e = d;
    d = c;
    c = ROTL32(b, 30);
    b = a;
    a = temp;
  }

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  
  state[0] = SWAP32(state[0]);
  state[1] = SWAP32(state[1]);
  state[2] = SWAP32(state[2]);
  state[3] = SWAP32(state[3]);
  state[4] = SWAP32(state[4]);
  memcpy(output, state, sizeof(state));
}