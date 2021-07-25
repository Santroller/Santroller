#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef enum
{
    HID_GET_REPORT = 0x01,
    HID_SET_REPORT = 0x09,
    WCID_GET_OS_FEATURE_DESCRIPTOR = 0x20
} USB_requests_t;
typedef enum
{
    USB_DIR_HOST_TO_DEVICE = 0,
    USB_DIR_DEVICE_TO_HOST = 1,

    USB_DIR_IN_MASK = 0x80
} USB_dir_t;
typedef enum
{
    USB_REQ_TYPE_STANDARD = 0,
    USB_REQ_TYPE_CLASS,
    USB_REQ_TYPE_VENDOR,
    USB_REQ_TYPE_INVALID
} USB_request_type_t;

typedef enum
{
    USB_REQ_RCPT_DEVICE = 0,
    USB_REQ_RCPT_INTERFACE,
    USB_REQ_RCPT_ENDPOINT,
    USB_REQ_RCPT_OTHER
} USB_request_recipient_t;
typedef union
{
    struct TU_ATTR_PACKED
    {
        uint8_t recipient : 5; ///< Recipient type USB_request_recipient_t.
        uint8_t type : 2;      ///< Request type USB_request_type_t.
        uint8_t direction : 1; ///< Direction type. USB_dir_t
    } bmRequestType_bit;

    uint8_t bmRequestType;
} requestType_t;
uint16_t controlRequest(const requestType_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength,  uint8_t **const buffer, bool *valid);
uint16_t descriptorRequest(const uint16_t wValue,
                           const uint16_t wIndex,
                           const void **const descriptorAddress);
