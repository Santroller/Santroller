#pragma once
#include <stddef.h>

// Collection of helper macros used for generating masks for the various controller structs for RF
// 3 uint64_ts is enough to store our biggest packet (keyboard)
#define bitoffsetof(t, f) \
	({ union { uint64_t raw[3]; t typ; }; \
	raw[0] = 0; raw[1] =0; raw[2] = 0; typ.f=true; raw[0]>0 ? __builtin_ctzll(raw[0]) : raw[1]>0 ? __builtin_ctzll(raw[1])+64 :__builtin_ctzll(raw[2])+64*3  ; })

#define maskbit(t, f) mask.mask[bitoffsetof(t, f) / 8] |= 1 << bitoffsetof(t, f) % 8
#define maskfield(t,f) for (size_t i = 0; i < sizeof(((t *)0)->f); i++) mask.mask[offsetof(t,f)+i] = 0xFF