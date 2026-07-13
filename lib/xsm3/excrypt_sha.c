#include <stdlib.h>
#include <string.h>

#include "excrypt.h"

// SHA1 code based on https://github.com/mohaps/TinySHA1

void sha1_process_block(EXCRYPT_SHA_STATE *state)
{
  uint32_t w[80];
  for (size_t i = 0; i < 16; i++)
  {
    w[i] = ((uint32_t)state->buffer[i * 4 + 0] << 24);
    w[i] |= ((uint32_t)state->buffer[i * 4 + 1] << 16);
    w[i] |= ((uint32_t)state->buffer[i * 4 + 2] << 8);
    w[i] |= ((uint32_t)state->buffer[i * 4 + 3]);
  }
  for (size_t i = 16; i < 80; i++)
  {
    w[i] = ROTL32((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);
  }

  uint32_t a = state->state[0];
  uint32_t b = state->state[1];
  uint32_t c = state->state[2];
  uint32_t d = state->state[3];
  uint32_t e = state->state[4];

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

  state->state[0] += a;
  state->state[1] += b;
  state->state[2] += c;
  state->state[3] += d;
  state->state[4] += e;
}

void sha1_process_byte(EXCRYPT_SHA_STATE *state, uint8_t octet)
{
  uint32_t offset = state->count++ & 0x3F;
  state->buffer[offset] = octet;
  if ((state->count & 0x3F) == 0)
  {
    sha1_process_block(state);
  }
}

void ExCryptSha(const uint8_t *input, uint32_t input_size, uint8_t *output, uint32_t output_size)
{
  EXCRYPT_SHA_STATE state={0, {0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476,0xC3D2E1F0}, {0}};

  for (uint32_t i = 0; i < input_size; i++)
  {
    sha1_process_byte(&state, input[i]);
  }

  uint64_t bit_count = (uint64_t)state.count * 8;

  sha1_process_byte(&state, 0x80);

  if ((state.count & 0x3F) > 56)
  {
    while ((state.count & 0x3F) != 0)
    {
      sha1_process_byte(&state, 0);
    }
    while ((state.count & 0x3F) < 56)
    {
      sha1_process_byte(&state, 0);
    }
  }
  else
  {
    while ((state.count & 0x3F) < 56)
    {
      sha1_process_byte(&state, 0);
    }
  }

  sha1_process_byte(&state, 0);
  sha1_process_byte(&state, 0);
  sha1_process_byte(&state, 0);
  sha1_process_byte(&state, 0);

  sha1_process_byte(&state, (uint8_t)((bit_count >> 24) & 0xFF));
  sha1_process_byte(&state, (uint8_t)((bit_count >> 16) & 0xFF));
  sha1_process_byte(&state, (uint8_t)((bit_count >> 8) & 0xFF));
  sha1_process_byte(&state, (uint8_t)((bit_count) & 0xFF));

  // sha1_process_block(state);
  uint32_t result[5];
  result[0] = SWAP32(state.state[0]);
  result[1] = SWAP32(state.state[1]);
  result[2] = SWAP32(state.state[2]);
  result[3] = SWAP32(state.state[3]);
  result[4] = SWAP32(state.state[4]);
  memcpy(output, result, 0x14);
}