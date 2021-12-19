#include "std_descriptors.h"
#pragma once
typedef struct {
    TUSB_Descriptor_Header_t Header;
    uint16_t Signature[7];
    uint8_t VendorCode;
    uint8_t Reserved;
} TATTR_PACKED TUSB_OSDescriptor_t;

typedef struct {
    uint8_t FirstInterfaceNumber;
    uint8_t Reserved;
    uint8_t CompatibleID[8];
    uint8_t SubCompatibleID[8];
    uint8_t Reserved2[6];
} TATTR_PACKED TUSB_OSCompatibleSection_t;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    TUSB_OSCompatibleSection_t CompatID[2];
} TATTR_PACKED TUSB_OSCompatibleIDDescriptor_t;

typedef struct {
    uint32_t PropertyDataType;
    uint16_t PropertyNameLength;
    uint16_t PropertyName[20];
    uint32_t PropertyDataLength;
    uint16_t PropertyData[39];
} TATTR_PACKED TUSB_OSExtendedSection_t;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint16_t TotalSections;
    uint32_t SectionSize;
    TUSB_OSExtendedSection_t ExtendedID;
} TATTR_PACKED TUSB_OSExtendedCompatibleIDDescriptor_t;