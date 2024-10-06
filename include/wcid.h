#pragma once
#include <stdint.h>
typedef struct {
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
				                          *   given by the specific class. */
    uint16_t Signature[7];
    uint8_t VendorCode;
    uint8_t Reserved;
} __attribute__((packed)) OS_DESCRIPTOR;

typedef struct {
    uint8_t FirstInterfaceNumber;
    uint8_t Reserved;
    uint8_t CompatibleID[8];
    uint8_t SubCompatibleID[8];
    uint8_t Reserved2[6];
} __attribute__((packed)) OS_COMPATIBLE_SECTION;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    OS_COMPATIBLE_SECTION CompatID[1];
} __attribute__((packed)) OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    OS_COMPATIBLE_SECTION CompatID[3];
} __attribute__((packed)) OS_COMPATIBLE_ID_DESCRIPTOR_THREE;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint8_t TotalSections;
    uint8_t Reserved[7];
    OS_COMPATIBLE_SECTION CompatID[4];
} __attribute__((packed)) OS_COMPATIBLE_ID_DESCRIPTOR;

typedef struct {
    uint32_t PropertyDataType;
    uint16_t PropertyNameLength;
    uint16_t PropertyName[20];
    uint32_t PropertyDataLength;
    uint16_t PropertyData[39];
} __attribute__((packed)) OS_EXTENDED_SECTION;

typedef struct {
    uint32_t TotalLength;
    uint16_t Version;
    uint16_t Index;
    uint16_t TotalSections;
    uint32_t SectionSize;
    OS_EXTENDED_SECTION ExtendedID;
} __attribute__((packed)) OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR;