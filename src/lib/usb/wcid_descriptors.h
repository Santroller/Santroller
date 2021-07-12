#include "std_descriptors.h"
#pragma once
typedef struct {
    USB_Descriptor_Header_t Header;
    uint16_t Signature[7];
    uint8_t VendorCode;
    uint8_t Reserved;
} ATTR_PACKED USB_OSDescriptor_t;

typedef struct {
    uint8_t FirstInterfaceNumber;
    uint8_t Reserved;
    uint8_t CompatibleID[8];
    uint8_t SubCompatibleID[8];
    uint8_t Reserved2[6];
} ATTR_PACKED USB_OSCompatibleSection_t;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    USB_OSCompatibleSection_t CompatID;
    USB_OSCompatibleSection_t CompatID2;
    USB_OSCompatibleSection_t CompatID3;
    USB_OSCompatibleSection_t CompatID4;
} ATTR_PACKED USB_OSCompatibleIDDescriptor_4_t;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    USB_OSCompatibleSection_t CompatID;
    USB_OSCompatibleSection_t CompatID2;
} ATTR_PACKED USB_OSCompatibleIDDescriptor_t;

typedef struct {
    uint32_t PropertyDataType;
    uint16_t PropertyNameLength;
    uint16_t PropertyName[20];
    uint32_t PropertyDataLength;
    uint16_t PropertyData[39];
} ATTR_PACKED USB_OSExtendedSection_t;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint16_t TotalSections;
    uint32_t SectionSize;
    USB_OSExtendedSection_t ExtendedID;
} ATTR_PACKED USB_OSExtendedCompatibleIDDescriptor_t;