#ifndef EXCRYPT_SHA_H_
#define EXCRYPT_SHA_H_
// SHA1 hash & HMAC algorithm

typedef struct _EXCRYPT_SHA_STATE
{
  uint32_t count;
  uint32_t state[5];
  uint8_t buffer[64];
} EXCRYPT_SHA_STATE;

void ExCryptShaInit(EXCRYPT_SHA_STATE* state);
void ExCryptShaUpdate(EXCRYPT_SHA_STATE* state, const uint8_t* input, uint32_t input_size);
void ExCryptShaFinal(EXCRYPT_SHA_STATE* state, uint8_t* output, uint32_t output_size);
void ExCryptSha(const uint8_t* input1, uint32_t input1_size, uint8_t* output, uint32_t output_size);

#endif // EXCRYPT_SHA_H_