#pragma once
#include "output/descriptors.h"
#ifdef MULTI_ADAPTOR
#  define CompatibleDescriptorType USB_OSCompatibleIDDescriptor_4_t
#else
#  define CompatibleDescriptorType USB_OSCompatibleIDDescriptor_t
#endif
extern AVR_CONST uint8_t capabilities1[8];
extern AVR_CONST uint8_t capabilities2[20];
extern AVR_CONST uint8_t ID[4];
extern AVR_CONST USB_OSExtendedCompatibleIDDescriptor_t ExtendedIDs;
extern AVR_CONST CompatibleDescriptorType DevCompatIDs;