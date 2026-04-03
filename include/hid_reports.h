#include "tusb.h"
#include "enums.pb.h"
#include "protocols/ps4.hpp"

// Button usages as defined in the linux kernel
#define BTN_A 0x01
#define BTN_B 0x02
#define BTN_C 0x03
#define BTN_X 0x04
#define BTN_Y 0x05
#define BTN_Z 0x06
#define BTN_TL 0x07
#define BTN_TR 0x08
#define BTN_TL2 0x09
#define BTN_TR2 0x0a
#define BTN_SELECT 0x0b
#define BTN_START 0x0c
#define BTN_GUIDE 0x0d
#define BTN_THUMBL 0x0e
#define BTN_THUMBR 0x0f
#define BTN_END 0x0f

#define TUD_HID_REPORT_DESC_KEYBOARD_NKRO(...)                                                                     \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                                                        \
        HID_USAGE(HID_USAGE_DESKTOP_KEYBOARD),                                                                     \
        HID_COLLECTION(HID_COLLECTION_APPLICATION), /* Report ID if any */                                         \
        __VA_ARGS__                                 /* 8 bits Modifier Keys (Shift, Control, Alt) */               \
        HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),                                                                   \
        HID_USAGE_MIN(224),                                                                                        \
        HID_USAGE_MAX(231),                                                                                        \
        HID_LOGICAL_MIN(0),                                                                                        \
        HID_LOGICAL_MAX(1),                                                                                        \
        HID_REPORT_COUNT(8),                                                                                       \
        HID_REPORT_SIZE(1),                                                                                        \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), /* 8 bit reserved */                                    \
        HID_REPORT_COUNT(1),                                                                                       \
        HID_REPORT_SIZE(8),                                                                                        \
        HID_INPUT(HID_CONSTANT), /* Output 5-bit LED Indicator Kana | Compose | ScrollLock | CapsLock | NumLock */ \
        HID_USAGE_PAGE(HID_USAGE_PAGE_LED),                                                                        \
        HID_USAGE_MIN(1),                                                                                          \
        HID_USAGE_MAX(5),                                                                                          \
        HID_REPORT_COUNT(5),                                                                                       \
        HID_REPORT_SIZE(1),                                                                                        \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), /* led padding */                                      \
        HID_REPORT_COUNT(1),                                                                                       \
        HID_REPORT_SIZE(3),                                                                                        \
        HID_OUTPUT(HID_CONSTANT),                                                                                  \
        HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),                                                                   \
        HID_USAGE_MIN(0),                                                                                          \
        HID_USAGE_MAX(0x73),                                                                                       \
        HID_LOGICAL_MIN(0),                                                                                        \
        HID_LOGICAL_MAX(1),                                                                                        \
        HID_REPORT_COUNT(0x73 + 1),                                                                                \
        HID_REPORT_SIZE(1),                                                                                        \
        HID_INPUT(HID_DATA | HID_ARRAY | HID_ABSOLUTE),                                                            \
        HID_COLLECTION_END
#define TUD_HID_REPORT_DESC_GENERIC_aINFEATURE(report_size, ...)         \
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),                          \
        HID_USAGE(0x01),                                                 \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                      \
        __VA_ARGS__          /* Report ID if any */                      \
            HID_USAGE(0x02), /* Input */                                 \
        HID_LOGICAL_MIN(0x00),                                           \
        HID_LOGICAL_MAX_N(0xff, 2),                                      \
        HID_REPORT_SIZE(8),                                              \
        HID_REPORT_COUNT(report_size),                                   \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), /* Feature */ \
        HID_USAGE(0x03),                                                 \
        HID_LOGICAL_MIN(0x00),                                           \
        HID_LOGICAL_MAX_N(0xff, 2),                                      \
        HID_REPORT_SIZE(8),                                              \
        HID_REPORT_COUNT(report_size),                                   \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),             \
        HID_COLLECTION_END
#define TUD_HID_REPORT_DESC_GENERIC_INFEATURE(report_size, ...)          \
    __VA_ARGS__          /* Report ID if any */                          \
        HID_USAGE(0x02), /* Input */                                     \
        HID_LOGICAL_MIN(0x00),                                           \
        HID_LOGICAL_MAX_N(0xff, 2),                                      \
        HID_REPORT_SIZE(8),                                              \
        HID_REPORT_COUNT(report_size),                                   \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), /* Feature */ \
        HID_USAGE(0x03),                                                 \
        HID_LOGICAL_MIN(0x00),                                           \
        HID_LOGICAL_MAX_N(0xff, 2),                                      \
        HID_REPORT_SIZE(8),                                              \
        HID_REPORT_COUNT(report_size),                                   \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

#define TUD_HID_REPORT_DESC_GENERIC_FEATURE(report_size, ...) \
    __VA_ARGS__ /* Report ID if any */                        \
        HID_USAGE(0x03),                                      \
        HID_LOGICAL_MIN(0x00),                                \
        HID_LOGICAL_MAX_N(0xff, 2),                           \
        HID_REPORT_SIZE(8),                                   \
        HID_REPORT_COUNT(report_size),                        \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

#define TUD_HID_REPORT_DESC_GUITAR_HERO_ARCADE()                                 \
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_DESKTOP, 2),                                 \
        HID_USAGE(HID_USAGE_DESKTOP_JOYSTICK),                                   \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
        HID_USAGE(HID_USAGE_DESKTOP_POINTER),                                    \
        HID_COLLECTION(HID_COLLECTION_PHYSICAL),                                 \
        HID_USAGE(HID_USAGE_DESKTOP_X),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_RX),                                         \
        HID_USAGE(HID_USAGE_DESKTOP_RY),                                         \
        HID_LOGICAL_MAX(0x7F),                                                   \
        HID_PHYSICAL_MIN(0x00),                                                  \
        HID_PHYSICAL_MAX(0x00),                                                  \
        HID_UNIT(0),                                                             \
        HID_UNIT_EXPONENT(0),                                                    \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(5),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_COLLECTION_END,                                                      \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),                                 \
        HID_LOGICAL_MIN(0x00),                                                   \
        HID_LOGICAL_MAX(3),                                                      \
        HID_PHYSICAL_MAX_N(0x010E, 2),                                           \
        HID_UNIT(0x14), /*Unit (System: English Rotation, Length: Centimeter) */ \
        HID_REPORT_SIZE(4),                                                      \
        HID_REPORT_COUNT(1),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                                   \
        HID_USAGE_MIN(0x01),                                                     \
        HID_USAGE_MAX(0x0A),                                                     \
        HID_LOGICAL_MAX(1),                                                      \
        HID_PHYSICAL_MAX(1),                                                     \
        HID_UNIT(0),                                                             \
        HID_REPORT_SIZE(1),                                                      \
        HID_REPORT_COUNT(0x0A),                                                  \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_REPORT_COUNT(0x02),                                                  \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                   \
        HID_COLLECTION_END

#define TUD_HID_REPORT_DESC_SANTROLLER_DESC()               \
    HID_REPORT_ID(ReportIdSantrollerCapabilities)           \
    HID_USAGE_N(0x2021, 2),                                 \
        HID_REPORT_SIZE(8),                                 \
        HID_REPORT_COUNT(1),                                \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_N(0x2021, 2),                             \
        HID_REPORT_SIZE(8),                                 \
        HID_REPORT_COUNT(2),                                \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

#define TUD_HID_REPORT_DESC_PS3_4_5()                        \
    HID_USAGE_N(0x2621, 2),                                  \
        HID_REPORT_SIZE(8),                                  \
        HID_REPORT_COUNT(8),                                 \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),  \
        HID_REPORT_ID(ReportIdPs4Feature)                    \
            HID_USAGE_N(0x2821, 2),                          \
        HID_REPORT_COUNT(47),                                \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_N(0x2721, 2),                              \
        HID_REPORT_COUNT(47),                                \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_N(0x2821, 2),                              \
        HID_REPORT_COUNT(47),                                \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_N(0x2621, 2),                              \
        HID_REPORT_COUNT(32),                                \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

#define TUD_HID_REPORT_DESC_PS3()                           \
    HID_USAGE_N(0x2621, 2),                                 \
        HID_REPORT_SIZE(8),                                 \
        HID_REPORT_COUNT(8),                                \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_N(0x2621, 2),                             \
        HID_REPORT_COUNT(32),                               \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

#define TUD_HID_REPORT_DESC_PS3_VENDOR()        \
    HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2), \
        HID_USAGE(0x20),                        \
        HID_USAGE(0x21),                        \
        HID_USAGE(0x22),                        \
        HID_USAGE(0x23),                        \
        HID_USAGE(0x24),                        \
        HID_USAGE(0x25),                        \
        HID_USAGE(0x26),                        \
        HID_USAGE(0x27),                        \
        HID_USAGE(0x28),                        \
        HID_USAGE(0x29),                        \
        HID_USAGE(0x2A),                        \
        HID_USAGE(0x2B),                        \
        HID_REPORT_COUNT(0x0C),                 \
        HID_LOGICAL_MIN(0x00),                  \
        HID_LOGICAL_MAX_N(0xff, 2),             \
        HID_REPORT_SIZE(8),                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

#define TUD_HID_REPORT_PID()                                                                                                                        \
    0x05, 0x0F,                          /*   Usage Page (PID Page)*/                                                                               \
        0x09, 0x92,                      /*   Usage (0x92)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, PID_BLOCK_LOAD_REPORT,     /*     Report ID (2)*/                                                                                     \
        0x09, 0x9F,                      /*     Usage (0x9F)*/                                                                                      \
        0x09, 0xA0,                      /*     Usage (0xA0)*/                                                                                      \
        0x09, 0xA5,                      /*     Usage (0xA5)*/                                                                                      \
        0x09, 0xA6,                      /*     Usage (0xA6)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x25, 0x01,                      /*     Logical Maximum (1)*/                                                                               \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x45, 0x01,                      /*     Physical Maximum (1)*/                                                                              \
        0x75, 0x01,                      /*     Report Size (1)*/                                                                                   \
        0x95, 0x04,                      /*     Report Count (4)*/                                                                                  \
        0x81, 0x02,                      /*     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/                              \
        0x95, 0x04,                      /*     Report Count (4)*/                                                                                  \
        0x75, 0x01,                      /*     Report Size (1)*/                                                                                   \
        0x81, 0x03,                      /*     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/                             \
        0x09, 0x94,                      /*     Usage (0x94)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x25, 0x01,                      /*     Logical Maximum (1)*/                                                                               \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x45, 0x01,                      /*     Physical Maximum (1)*/                                                                              \
        0x75, 0x01,                      /*     Report Size (1)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x81, 0x02,                      /*     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/                              \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x07,                      /*     Report Size (7)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x81, 0x02,                      /*     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/                              \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x21,                      /*   Usage (0x21)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_EFFECT,         /*     Report ID (1)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x25,                      /*     Usage (0x25)*/                                                                                      \
        0xA1, 0x02,                      /*     Collection (Logical)*/                                                                              \
        0x09, 0x26,                      /*       Usage (0x26)*/                                                                                    \
        0x09, 0x27,                      /*       Usage (0x27)*/                                                                                    \
        0x09, 0x30,                      /*       Usage (0x30)*/                                                                                    \
        0x09, 0x31,                      /*       Usage (0x31)*/                                                                                    \
        0x09, 0x32,                      /*       Usage (0x32)*/                                                                                    \
        0x09, 0x33,                      /*       Usage (0x33)*/                                                                                    \
        0x09, 0x34,                      /*       Usage (0x34)*/                                                                                    \
        0x09, 0x40,                      /*       Usage (0x40)*/                                                                                    \
        0x09, 0x41,                      /*       Usage (0x41)*/                                                                                    \
        0x09, 0x42,                      /*       Usage (0x42)*/                                                                                    \
        0x09, 0x43,                      /*       Usage (0x43)*/                                                                                    \
        0x09, 0x28,                      /*       Usage (0x28)*/                                                                                    \
        0x25, 0x0C,                      /*       Logical Maximum (12)*/                                                                            \
        0x15, 0x01,                      /*       Logical Minimum (1)*/                                                                             \
        0x35, 0x01,                      /*       Physical Minimum (1)*/                                                                            \
        0x45, 0x0C,                      /*       Physical Maximum (12)*/                                                                           \
        0x75, 0x08,                      /*       Report Size (8)*/                                                                                 \
        0x95, 0x01,                      /*       Report Count (1)*/                                                                                \
        0x91, 0x00,                      /*       Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/            \
        0xC0,                            /*     End Collection*/                                                                                    \
        0x09, 0x50,                      /*     Usage (0x50)*/                                                                                      \
        0x09, 0x54,                      /*     Usage (0x54)*/                                                                                      \
        0x09, 0x51,                      /*     Usage (0x51)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x7F,                /*     Logical Maximum (32767)*/                                                                           \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x7F,                /*     Physical Maximum (32767)*/                                                                          \
        0x66, 0x03, 0x10,                /*     Unit (System: English Linear, Time: Seconds)*/                                                      \
        0x55, 0xFD,                      /*     Unit Exponent*/                                                                                     \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x03,                      /*     Report Count (3)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x55, 0x00,                      /*     Unit Exponent (0)*/                                                                                 \
        0x66, 0x00, 0x00,                /*     Unit (None)*/                                                                                       \
        0x09, 0x52,                      /*     Usage (0x52)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x53,                      /*     Usage (0x53)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x08,                      /*     Logical Maximum (8)*/                                                                               \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x08,                      /*     Physical Maximum (8)*/                                                                              \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x55,                      /*     Usage (0x55)*/                                                                                      \
        0xA1, 0x02,                      /*     Collection (Logical)*/                                                                              \
        0x05, 0x01,                      /*       Usage Page (Generic Desktop Ctrls)*/                                                              \
        0x09, 0x30,                      /*       Usage (X)*/                                                                                       \
        0x09, 0x31,                      /*       Usage (Y)*/                                                                                       \
        0x15, 0x00,                      /*       Logical Minimum (0)*/                                                                             \
        0x25, 0x01,                      /*       Logical Maximum (1)*/                                                                             \
        0x75, 0x01,                      /*       Report Size (1)*/                                                                                 \
        0x95, 0x02,                      /*       Report Count (2)*/                                                                                \
        0x91, 0x02,                      /*       Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/              \
        0xC0,                            /*     End Collection*/                                                                                    \
        0x05, 0x0F,                      /*     Usage Page (PID Page)*/                                                                             \
        0x09, 0x56,                      /*     Usage (0x56)*/                                                                                      \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x95, 0x05,                      /*     Report Count (5)*/                                                                                  \
        0x91, 0x03,                      /*     Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/               \
        0x09, 0x57,                      /*     Usage (0x57)*/                                                                                      \
        0xA1, 0x02,                      /*     Collection (Logical)*/                                                                              \
        0x0B, 0x01, 0x00, 0x0A, 0x00,    /*       Usage (0x0A0001)*/                                                                                \
        0x0B, 0x02, 0x00, 0x0A, 0x00,    /*       Usage (0x0A0002)*/                                                                                \
        0x66, 0x14, 0x00,                /*       Unit (System: English Rotation, Length: Centimeter)*/                                             \
        0x55, 0xFE,                      /*       Unit Exponent*/                                                                                   \
        0x15, 0x00,                      /*       Logical Minimum (0)*/                                                                             \
        0x26, 0xFF, 0x00,                /*       Logical Maximum (255)*/                                                                           \
        0x35, 0x00,                      /*       Physical Minimum (0)*/                                                                            \
        0x47, 0xA0, 0x8C, 0x00, 0x00,    /*       Physical Maximum (35999)*/                                                                        \
        0x66, 0x00, 0x00,                /*       Unit (None)*/                                                                                     \
        0x75, 0x08,                      /*       Report Size (8)*/                                                                                 \
        0x95, 0x02,                      /*       Report Count (2)*/                                                                                \
        0x91, 0x02,                      /*       Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/              \
        0x55, 0x00,                      /*       Unit Exponent (0)*/                                                                               \
        0x66, 0x00, 0x00,                /*       Unit (None)*/                                                                                     \
        0xC0,                            /*     End Collection*/                                                                                    \
        0x05, 0x0F,                      /*     Usage Page (PID Page)*/                                                                             \
        0x09, 0xA7,                      /*     Usage (0xA7)*/                                                                                      \
        0x66, 0x03, 0x10,                /*     Unit (System: English Linear, Time: Seconds)*/                                                      \
        0x55, 0xFD,                      /*     Unit Exponent*/                                                                                     \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x7F,                /*     Logical Maximum (32767)*/                                                                           \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x7F,                /*     Physical Maximum (32767)*/                                                                          \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x66, 0x00, 0x00,                /*     Unit (None)*/                                                                                       \
        0x55, 0x00,                      /*     Unit Exponent (0)*/                                                                                 \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x05, 0x0F,                      /*   Usage Page (PID Page)*/                                                                               \
        0x09, 0x5A,                      /*   Usage (0x5A)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_STATUS,         /*     Report ID (2)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x5B,                      /*     Usage (0x5B)*/                                                                                      \
        0x09, 0x5D,                      /*     Usage (0x5D)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x95, 0x02,                      /*     Report Count (2)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x5C,                      /*     Usage (0x5C)*/                                                                                      \
        0x09, 0x5E,                      /*     Usage (0x5E)*/                                                                                      \
        0x66, 0x03, 0x10,                /*     Unit (System: English Linear, Time: Seconds)*/                                                      \
        0x55, 0xFD,                      /*     Unit Exponent*/                                                                                     \
        0x26, 0xFF, 0x7F,                /*     Logical Maximum (32767)*/                                                                           \
        0x46, 0xFF, 0x7F,                /*     Physical Maximum (32767)*/                                                                          \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x45, 0x00,                      /*     Physical Maximum (0)*/                                                                              \
        0x66, 0x00, 0x00,                /*     Unit (None)*/                                                                                       \
        0x55, 0x00,                      /*     Unit Exponent (0)*/                                                                                 \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x5F,                      /*   Usage (0x5F)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, PID_SIMULTANEOUS_MAX,      /*     Report ID (3)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x23,                      /*     Usage (0x23)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x25, 0x01,                      /*     Logical Maximum (1)*/                                                                               \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x45, 0x01,                      /*     Physical Maximum (1)*/                                                                              \
        0x75, 0x04,                      /*     Report Size (4)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x58,                      /*     Usage (0x58)*/                                                                                      \
        0xA1, 0x02,                      /*     Collection (Logical)*/                                                                              \
        0x0B, 0x01, 0x00, 0x0A, 0x00,    /*       Usage (0x0A0001)*/                                                                                \
        0x0B, 0x02, 0x00, 0x0A, 0x00,    /*       Usage (0x0A0002)*/                                                                                \
        0x75, 0x02,                      /*       Report Size (2)*/                                                                                 \
        0x95, 0x02,                      /*       Report Count (2)*/                                                                                \
        0x91, 0x02,                      /*       Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/              \
        0xC0,                            /*     End Collection*/                                                                                    \
        0x15, 0x80,                      /*     Logical Minimum (-128)*/                                                                            \
        0x25, 0x7F,                      /*     Logical Maximum (127)*/                                                                             \
        0x36, 0xF0, 0xD8,                /*     Physical Minimum (-10000)*/                                                                         \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x09, 0x60,                      /*     Usage (0x60)*/                                                                                      \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x36, 0xF0, 0xD8,                /*     Physical Minimum (-10000)*/                                                                         \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x09, 0x61,                      /*     Usage (0x61)*/                                                                                      \
        0x09, 0x62,                      /*     Usage (0x62)*/                                                                                      \
        0x95, 0x02,                      /*     Report Count (2)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x09, 0x63,                      /*     Usage (0x63)*/                                                                                      \
        0x09, 0x64,                      /*     Usage (0x64)*/                                                                                      \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x02,                      /*     Report Count (2)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x65,                      /*     Usage (0x65)*/                                                                                      \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x6E,                      /*   Usage (0x6E)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_PERIODIC,       /*     Report ID (4)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x70,                      /*     Usage (0x70)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x6F,                      /*     Usage (0x6F)*/                                                                                      \
        0x15, 0x80,                      /*     Logical Minimum (-128)*/                                                                            \
        0x25, 0x7F,                      /*     Logical Maximum (127)*/                                                                             \
        0x36, 0xF0, 0xD8,                /*     Physical Minimum (-10000)*/                                                                         \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x71,                      /*     Usage (0x71)*/                                                                                      \
        0x66, 0x14, 0x00,                /*     Unit (System: English Rotation, Length: Centimeter)*/                                               \
        0x55, 0xFE,                      /*     Unit Exponent*/                                                                                     \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x47, 0xA0, 0x8C, 0x00, 0x00,    /*     Physical Maximum (35999)*/                                                                          \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x72,                      /*     Usage (0x72)*/                                                                                      \
        0x26, 0xFF, 0x7F,                /*     Logical Maximum (32767)*/                                                                           \
        0x46, 0xFF, 0x7F,                /*     Physical Maximum (32767)*/                                                                          \
        0x66, 0x03, 0x10,                /*     Unit (System: English Linear, Time: Seconds)*/                                                      \
        0x55, 0xFD,                      /*     Unit Exponent*/                                                                                     \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x66, 0x00, 0x00,                /*     Unit (None)*/                                                                                       \
        0x55, 0x00,                      /*     Unit Exponent (0)*/                                                                                 \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x73,                      /*   Usage (0x73)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_CONSTANT_FORCE, /*     Report ID (5)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x70,                      /*     Usage (0x70)*/                                                                                      \
        0x16, 0x01, 0xFF,                /*     Logical Minimum (-255)*/                                                                            \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x36, 0xF0, 0xD8,                /*     Physical Minimum (-10000)*/                                                                         \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x74,                      /*   Usage (0x74)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_6,              /*     Report ID (6)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x75,                      /*     Usage (0x75)*/                                                                                      \
        0x09, 0x76,                      /*     Usage (0x76)*/                                                                                      \
        0x15, 0x80,                      /*     Logical Minimum (-128)*/                                                                            \
        0x25, 0x7F,                      /*     Logical Maximum (127)*/                                                                             \
        0x36, 0xF0, 0xD8,                /*     Physical Minimum (-10000)*/                                                                         \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x02,                      /*     Report Count (2)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x68,                      /*   Usage (0x68)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_7,              /*     Report ID (7)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x6C,                      /*     Usage (0x6C)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0x10, 0x27,                /*     Logical Maximum (10000)*/                                                                           \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x69,                      /*     Usage (0x69)*/                                                                                      \
        0x15, 0x81,                      /*     Logical Minimum (-127)*/                                                                            \
        0x25, 0x7F,                      /*     Logical Maximum (127)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x00,                /*     Physical Maximum (255)*/                                                                            \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x0C,                      /*     Report Count (12)*/                                                                                 \
        0x92, 0x02, 0x01,                /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile,Buffered Bytes)*/ \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x66,                      /*   Usage (0x66)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_8,              /*     Report ID (8)*/                                                                                     \
        0x05, 0x01,                      /*     Usage Page (Generic Desktop Ctrls)*/                                                                \
        0x09, 0x30,                      /*     Usage (X)*/                                                                                         \
        0x09, 0x31,                      /*     Usage (Y)*/                                                                                         \
        0x15, 0x81,                      /*     Logical Minimum (-127)*/                                                                            \
        0x25, 0x7F,                      /*     Logical Maximum (127)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x00,                /*     Physical Maximum (255)*/                                                                            \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x02,                      /*     Report Count (2)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x05, 0x0F,                      /*   Usage Page (PID Page)*/                                                                               \
        0x09, 0x77,                      /*   Usage (0x77)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_EFFECT_OPERATION,   /*     Report ID (10)*/                                                                                    \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x78,                      /*     Usage (0x78)*/                                                                                      \
        0xA1, 0x02,                      /*     Collection (Logical)*/                                                                              \
        0x09, 0x79,                      /*       Usage (0x79)*/                                                                                    \
        0x09, 0x7A,                      /*       Usage (0x7A)*/                                                                                    \
        0x09, 0x7B,                      /*       Usage (0x7B)*/                                                                                    \
        0x15, 0x01,                      /*       Logical Minimum (1)*/                                                                             \
        0x25, 0x03,                      /*       Logical Maximum (3)*/                                                                             \
        0x75, 0x08,                      /*       Report Size (8)*/                                                                                 \
        0x95, 0x01,                      /*       Report Count (1)*/                                                                                \
        0x91, 0x00,                      /*       Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/            \
        0xC0,                            /*     End Collection*/                                                                                    \
        0x09, 0x7C,                      /*     Usage (0x7C)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x00,                /*     Physical Maximum (255)*/                                                                            \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x90,                      /*   Usage (0x90)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_EFFECT_BLOCK_IDX,   /*     Report ID (11)*/                                                                                    \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x96,                      /*   Usage (0x96)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_DISABLE_ACTUATORS,  /*     Report ID (12)*/                                                                                    \
        0x09, 0x97,                      /*     Usage (0x97)*/                                                                                      \
        0x09, 0x98,                      /*     Usage (0x98)*/                                                                                      \
        0x09, 0x99,                      /*     Usage (0x99)*/                                                                                      \
        0x09, 0x9A,                      /*     Usage (0x9A)*/                                                                                      \
        0x09, 0x9B,                      /*     Usage (0x9B)*/                                                                                      \
        0x09, 0x9C,                      /*     Usage (0x9C)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x06,                      /*     Logical Maximum (6)*/                                                                               \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x00,                      /*     Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/              \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x7D,                      /*   Usage (0x7D)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_PID_POOL,           /*     Report ID (13)*/                                                                                    \
        0x09, 0x7E,                      /*     Usage (0x7E)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0x10, 0x27,                /*     Physical Maximum (10000)*/                                                                          \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x6B,                      /*   Usage (0x6B)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_SET_E,              /*     Report ID (14)*/                                                                                    \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x6D,                      /*     Usage (0x6D)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x00,                /*     Physical Maximum (255)*/                                                                            \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x09, 0x51,                      /*     Usage (0x51)*/                                                                                      \
        0x66, 0x03, 0x10,                /*     Unit (System: English Linear, Time: Seconds)*/                                                      \
        0x55, 0xFD,                      /*     Unit Exponent*/                                                                                     \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x7F,                /*     Logical Maximum (32767)*/                                                                           \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x7F,                /*     Physical Maximum (32767)*/                                                                          \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x91, 0x02,                      /*     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/                \
        0x55, 0x00,                      /*     Unit Exponent (0)*/                                                                                 \
        0x66, 0x00, 0x00,                /*     Unit (None)*/                                                                                       \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0xAB,                      /*   Usage (0xAB)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, REPORT_CREATE_EFFECT,      /*     Report ID (9)*/                                                                                     \
        0x09, 0x25,                      /*     Usage (0x25)*/                                                                                      \
        0xA1, 0x02,                      /*     Collection (Logical)*/                                                                              \
        0x09, 0x26,                      /*       Usage (0x26)*/                                                                                    \
        0x09, 0x27,                      /*       Usage (0x27)*/                                                                                    \
        0x09, 0x30,                      /*       Usage (0x30)*/                                                                                    \
        0x09, 0x31,                      /*       Usage (0x31)*/                                                                                    \
        0x09, 0x32,                      /*       Usage (0x32)*/                                                                                    \
        0x09, 0x33,                      /*       Usage (0x33)*/                                                                                    \
        0x09, 0x34,                      /*       Usage (0x34)*/                                                                                    \
        0x09, 0x40,                      /*       Usage (0x40)*/                                                                                    \
        0x09, 0x41,                      /*       Usage (0x41)*/                                                                                    \
        0x09, 0x42,                      /*       Usage (0x42)*/                                                                                    \
        0x09, 0x43,                      /*       Usage (0x43)*/                                                                                    \
        0x09, 0x28,                      /*       Usage (0x28)*/                                                                                    \
        0x25, 0x0C,                      /*       Logical Maximum (12)*/                                                                            \
        0x15, 0x01,                      /*       Logical Minimum (1)*/                                                                             \
        0x35, 0x01,                      /*       Physical Minimum (1)*/                                                                            \
        0x45, 0x0C,                      /*       Physical Maximum (12)*/                                                                           \
        0x75, 0x08,                      /*       Report Size (8)*/                                                                                 \
        0x95, 0x01,                      /*       Report Count (1)*/                                                                                \
        0xB1, 0x00,                      /*       Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/           \
        0xC0,                            /*     End Collection*/                                                                                    \
        0x05, 0x01,                      /*     Usage Page (Generic Desktop Ctrls)*/                                                                \
        0x09, 0x3B,                      /*     Usage (Byte Count)*/                                                                                \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x26, 0xFF, 0x01,                /*     Logical Maximum (511)*/                                                                             \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x46, 0xFF, 0x01,                /*     Physical Maximum (511)*/                                                                            \
        0x75, 0x0A,                      /*     Report Size (10)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0xB1, 0x02,                      /*     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/               \
        0x75, 0x06,                      /*     Report Size (6)*/                                                                                   \
        0xB1, 0x01,                      /*     Feature (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/            \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x05, 0x0F,                      /*   Usage Page (PID Page)*/                                                                               \
        0x09, 0x89,                      /*   Usage (0x89)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, PID_BLOCK_LOAD_REPORT,     /*     Report ID (2)*/                                                                                     \
        0x09, 0x22,                      /*     Usage (0x22)*/                                                                                      \
        0x25, 0x28,                      /*     Logical Maximum (40)*/                                                                              \
        0x15, 0x01,                      /*     Logical Minimum (1)*/                                                                               \
        0x35, 0x01,                      /*     Physical Minimum (1)*/                                                                              \
        0x45, 0x28,                      /*     Physical Maximum (40)*/                                                                             \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0xB1, 0x02,                      /*     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/               \
        0x09, 0x8B,                      /*     Usage (0x8B)*/                                                                                      \
        0xA1, 0x02,                      /*     Collection (Logical)*/                                                                              \
        0x09, 0x8C,                      /*       Usage (0x8C)*/                                                                                    \
        0x09, 0x8D,                      /*       Usage (0x8D)*/                                                                                    \
        0x09, 0x8E,                      /*       Usage (0x8E)*/                                                                                    \
        0x25, 0x03,                      /*       Logical Maximum (3)*/                                                                             \
        0x15, 0x01,                      /*       Logical Minimum (1)*/                                                                             \
        0x35, 0x01,                      /*       Physical Minimum (1)*/                                                                            \
        0x45, 0x03,                      /*       Physical Maximum (3)*/                                                                            \
        0x75, 0x08,                      /*       Report Size (8)*/                                                                                 \
        0x95, 0x01,                      /*       Report Count (1)*/                                                                                \
        0xB1, 0x00,                      /*       Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/           \
        0xC0,                            /*     End Collection*/                                                                                    \
        0x09, 0xAC,                      /*     Usage (0xAC)*/                                                                                      \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x27, 0xFF, 0xFF, 0x00, 0x00,    /*     Logical Maximum (65534)*/                                                                           \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x47, 0xFF, 0xFF, 0x00, 0x00,    /*     Physical Maximum (65534)*/                                                                          \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0xB1, 0x00,                      /*     Feature (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/             \
        0xC0,                            /*   End Collection*/                                                                                      \
        0x09, 0x7F,                      /*   Usage (0x7F)*/                                                                                        \
        0xA1, 0x02,                      /*   Collection (Logical)*/                                                                                \
        0x85, PID_SIMULTANEOUS_MAX,      /*     Report ID (3)*/                                                                                     \
        0x09, 0x80,                      /*     Usage (0x80)*/                                                                                      \
        0x75, 0x10,                      /*     Report Size (16)*/                                                                                  \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x27, 0xFF, 0xFF, 0x00, 0x00,    /*     Logical Maximum (65534)*/                                                                           \
        0x47, 0xFF, 0xFF, 0x00, 0x00,    /*     Physical Maximum (65534)*/                                                                          \
        0xB1, 0x02,                      /*     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/               \
        0x09, 0x83,                      /*     Usage (0x83)*/                                                                                      \
        0x26, 0xFF, 0x00,                /*     Logical Maximum (255)*/                                                                             \
        0x46, 0xFF, 0x00,                /*     Physical Maximum (255)*/                                                                            \
        0x75, 0x08,                      /*     Report Size (8)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0xB1, 0x02,                      /*     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/               \
        0x09, 0xA9,                      /*     Usage (0xA9)*/                                                                                      \
        0x09, 0xAA,                      /*     Usage (0xAA)*/                                                                                      \
        0x75, 0x01,                      /*     Report Size (1)*/                                                                                   \
        0x95, 0x02,                      /*     Report Count (2)*/                                                                                  \
        0x15, 0x00,                      /*     Logical Minimum (0)*/                                                                               \
        0x25, 0x01,                      /*     Logical Maximum (1)*/                                                                               \
        0x35, 0x00,                      /*     Physical Minimum (0)*/                                                                              \
        0x45, 0x01,                      /*     Physical Maximum (1)*/                                                                              \
        0xB1, 0x02,                      /*     Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/               \
        0x75, 0x06,                      /*     Report Size (6)*/                                                                                   \
        0x95, 0x01,                      /*     Report Count (1)*/                                                                                  \
        0xB1, 0x03,                      /*     Feature (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)*/              \
        0xC0                             /*   End Collection*/

#define TUD_HID_REPORT_DESC_GAME_CONTROLLER(rid)               \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                    \
        HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                  \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),            \
        rid                                                    \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_REPORT_SIZE(8),                                    \
        HID_REPORT_COUNT(1),                                   \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),               \
        HID_LOGICAL_MAX(7),                                    \
        HID_PHYSICAL_MAX_N(315, 2),                            \
        HID_REPORT_COUNT(1),                                   \
        HID_REPORT_SIZE(4),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                 \
        HID_LOGICAL_MIN(0),                                    \
        HID_LOGICAL_MAX(1),                                    \
        HID_PHYSICAL_MIN(0),                                   \
        HID_PHYSICAL_MAX(1),                                   \
        HID_REPORT_COUNT(12),                                  \
        HID_REPORT_SIZE(1),                                    \
        HID_USAGE(BTN_START),                                  \
        HID_USAGE(BTN_SELECT),                                 \
        HID_USAGE(BTN_THUMBL),                                 \
        HID_USAGE(BTN_THUMBR),                                 \
        HID_USAGE(BTN_TL),                                     \
        HID_USAGE(BTN_TR),                                     \
        HID_USAGE(BTN_GUIDE),                                  \
        HID_USAGE(BTN_C),                                      \
        HID_USAGE(BTN_A),                                      \
        HID_USAGE(BTN_B),                                      \
        HID_USAGE(BTN_X),                                      \
        HID_USAGE(BTN_Y),                                      \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                        \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                       \
        HID_LOGICAL_MAX_N(0xff, 2),                            \
        HID_PHYSICAL_MAX_N(0xff, 2),                           \
        HID_REPORT_COUNT(2),                                   \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE(HID_USAGE_DESKTOP_X),                        \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                        \
        HID_USAGE(HID_USAGE_DESKTOP_RX),                       \
        HID_USAGE(HID_USAGE_DESKTOP_RY),                       \
        HID_LOGICAL_MIN_N(INT16_MIN, 2),                       \
        HID_PHYSICAL_MIN_N(INT16_MIN, 2),                      \
        HID_LOGICAL_MAX_N(INT16_MAX, 2),                       \
        HID_PHYSICAL_MAX_N(INT16_MAX, 2),                      \
        HID_REPORT_COUNT(4),                                   \
        HID_REPORT_SIZE(16),                                   \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),            \
        HID_USAGE(0x20),                                       \
        HID_USAGE(0x21),                                       \
        HID_USAGE(0x22),                                       \
        HID_USAGE(0x23),                                       \
        HID_USAGE(0x24),                                       \
        HID_USAGE(0x25),                                       \
        HID_REPORT_COUNT(0x06),                                \
        HID_LOGICAL_MIN(0x00),                                 \
        HID_LOGICAL_MAX_N(0xff, 2),                            \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        TUD_HID_REPORT_DESC_PS3_4_5(),                         \
        TUD_HID_REPORT_DESC_SANTROLLER_DESC(),                 \
        HID_COLLECTION_END

#define TUD_HID_REPORT_DESC_PS3_THIRDPARTY_GAMEPAD()           \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                    \
        HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                  \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),            \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                 \
        HID_LOGICAL_MIN(0),                                    \
        HID_LOGICAL_MAX(1),                                    \
        HID_PHYSICAL_MIN(0),                                   \
        HID_PHYSICAL_MAX(1),                                   \
        HID_REPORT_COUNT(13),                                  \
        HID_REPORT_SIZE(1),                                    \
        HID_USAGE_MIN(0x01),                                   \
        HID_USAGE_MAX(13),                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_REPORT_COUNT(3),                                   \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),               \
        HID_LOGICAL_MIN(1),                                    \
        HID_LOGICAL_MAX(8),                                    \
        HID_PHYSICAL_MAX_N(315, 2),                            \
        HID_REPORT_COUNT(1),                                   \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_USAGE(HID_USAGE_DESKTOP_X),                        \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                        \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                        \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                       \
        HID_LOGICAL_MIN(0x00),                                 \
        HID_LOGICAL_MAX(0x01),                                 \
        HID_REPORT_COUNT(4),                                   \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        TUD_HID_REPORT_DESC_PS3_VENDOR(),                      \
        TUD_HID_REPORT_DESC_PS3(),                             \
        HID_USAGE(0x2C),                                       \
        HID_USAGE(0x2D),                                       \
        HID_USAGE(0x2E),                                       \
        HID_USAGE(0x2F),                                       \
        HID_REPORT_COUNT(4),                                   \
        HID_LOGICAL_MAX_N(0x3ff, 2),                           \
        HID_PHYSICAL_MAX_N(0x3ff, 2),                          \
        HID_REPORT_SIZE(16),                                   \
        HID_REPORT_COUNT(4),                                   \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_COLLECTION_END

#define TUD_HID_REPORT_DESC_PS3_FIRSTPARTY_GAMEPAD(rid)        \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                    \
        HID_USAGE(HID_USAGE_DESKTOP_JOYSTICK),                 \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),            \
        HID_COLLECTION(HID_COLLECTION_LOGICAL),                \
        rid                                                    \
            HID_REPORT_SIZE(8),                                \
        HID_REPORT_COUNT(1),                                   \
        HID_LOGICAL_MIN(0),                                    \
        HID_LOGICAL_MAX_N(255, 2),                             \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE), \
        HID_REPORT_SIZE(1),                                    \
        HID_REPORT_COUNT(19),                                  \
        HID_LOGICAL_MIN(0),                                    \
        HID_LOGICAL_MAX(1),                                    \
        HID_PHYSICAL_MIN(0),                                   \
        HID_PHYSICAL_MAX(1),                                   \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                 \
        HID_USAGE_MIN(1),                                      \
        HID_USAGE_MAX(19),                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_REPORT_COUNT(13),                                  \
        HID_REPORT_SIZE(1),                                    \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),            \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_USAGE(HID_USAGE_DESKTOP_POINTER),                  \
        HID_COLLECTION(HID_COLLECTION_PHYSICAL),               \
        HID_USAGE(HID_USAGE_DESKTOP_X),                        \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                        \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                        \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                       \
        HID_REPORT_COUNT(4),                                   \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_COLLECTION_END,                                    \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_REPORT_COUNT(19),                                  \
        HID_USAGE(HID_USAGE_DESKTOP_POINTER),                  \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_REPORT_COUNT(12),                                  \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE), \
        HID_LOGICAL_MAX_N(0x3ff, 2),                           \
        HID_PHYSICAL_MAX_N(0x3ff, 2),                          \
        HID_REPORT_SIZE(16),                                   \
        HID_REPORT_COUNT(4),                                   \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_COLLECTION_END,                                    \
        HID_COLLECTION(HID_COLLECTION_LOGICAL),                \
        HID_REPORT_ID(ReportIdPs302)                           \
            HID_REPORT_SIZE(8),                                \
        HID_REPORT_COUNT(48),                                  \
        HID_USAGE(HID_USAGE_DESKTOP_POINTER),                  \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_COLLECTION_END,                                    \
        HID_COLLECTION(HID_COLLECTION_LOGICAL),                \
        HID_REPORT_ID(ReportIdPs3EE)                           \
            HID_REPORT_SIZE(8),                                \
        HID_REPORT_COUNT(48),                                  \
        HID_USAGE(HID_USAGE_DESKTOP_POINTER),                  \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_COLLECTION_END,                                    \
        HID_COLLECTION(HID_COLLECTION_LOGICAL),                \
        HID_REPORT_ID(ReportIdPs3EF)                           \
            HID_REPORT_SIZE(8),                                \
        HID_REPORT_COUNT(48),                                  \
        HID_USAGE(HID_USAGE_DESKTOP_POINTER),                  \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_COLLECTION_END,                                    \
        HID_COLLECTION_END

#define TUD_HID_REPORT_DESC_PS4_THIRDPARTY_GAMEPAD(rid)                          \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                      \
        HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                                    \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
        rid                                                                      \
        HID_USAGE(HID_USAGE_DESKTOP_X),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                                         \
        HID_LOGICAL_MIN(0x00),                                                   \
        HID_LOGICAL_MAX_N(0xFF, 2),                                              \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(4),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),                                 \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(7),                                                      \
        HID_PHYSICAL_MIN(0),                                                     \
        HID_PHYSICAL_MAX_N(315, 2),                                              \
        HID_UNIT(0x14), /*Unit (System: English Rotation, Length: Centimeter) */ \
        HID_REPORT_SIZE(4),                                                      \
        HID_REPORT_COUNT(1),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_NULL_STATE),      \
        HID_UNIT(0),                                                             \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                                   \
        HID_USAGE_MIN(0x01),                                                     \
        HID_USAGE_MAX(14),                                                       \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(1),                                                      \
        HID_REPORT_SIZE(1),                                                      \
        HID_REPORT_COUNT(14),                                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),                              \
        HID_USAGE(0x20),                                                         \
        HID_REPORT_SIZE(6),                                                      \
        HID_REPORT_COUNT(1),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                  \
        HID_USAGE(HID_USAGE_DESKTOP_RX),                                         \
        HID_USAGE(HID_USAGE_DESKTOP_RY),                                         \
        HID_LOGICAL_MIN(0x00),                                                   \
        HID_LOGICAL_MAX_N(0xFF, 2),                                              \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(2),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),                              \
        HID_USAGE(0x21),                                                         \
        HID_REPORT_COUNT(54),                                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_REPORT_ID(ReportIdPs405)                                             \
            HID_USAGE(0x22),                                                     \
        HID_REPORT_COUNT(31),                                                    \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                      \
        HID_REPORT_ID(ReportIdPs4Feature)                                        \
            HID_USAGE_N(0x2721, 2),                                              \
        HID_REPORT_COUNT(47),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_COLLECTION_END,                                                      \
        HID_USAGE_PAGE_N(0xFFF0, 2),                                             \
        HID_USAGE(0x40),                                                         \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
        HID_REPORT_ID(ReportIdPs4SetChallenge)                                   \
            HID_USAGE(0x47),                                                     \
        HID_REPORT_COUNT(63),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_REPORT_ID(ReportIdPs4GetResponse)                                    \
            HID_USAGE(0x48),                                                     \
        HID_REPORT_COUNT(63),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_REPORT_ID(ReportIdPs4GetAuthStatus)                                  \
            HID_USAGE(0x49),                                                     \
        HID_REPORT_COUNT(15),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_REPORT_ID(ReportIdPs4GetAuthPageSize)                                \
            HID_USAGE_N(0x4701, 2),                                              \
        HID_REPORT_COUNT(7),                                                     \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_COLLECTION_END

#define TUD_HID_REPORT_DESC_PS5_THIRDPARTY_GAMEPAD(rid)                          \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                      \
        HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                                    \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
        rid                                                                      \
        HID_USAGE(HID_USAGE_DESKTOP_X),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                                          \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                                         \
        HID_USAGE(HID_USAGE_DESKTOP_RX),                                         \
        HID_USAGE(HID_USAGE_DESKTOP_RY),                                         \
        HID_LOGICAL_MIN(0x00),                                                   \
        HID_LOGICAL_MAX_N(0xFF, 2),                                              \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(6),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),                              \
        HID_USAGE(0x20),                                                         \
        HID_REPORT_COUNT(1),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                  \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),                                 \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(7),                                                      \
        HID_PHYSICAL_MIN(0),                                                     \
        HID_PHYSICAL_MAX_N(315, 2),                                              \
        HID_UNIT(0x14), /*Unit (System: English Rotation, Length: Centimeter) */ \
        HID_REPORT_SIZE(4),                                                      \
        HID_REPORT_COUNT(1),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_NULL_STATE),      \
        HID_UNIT(0),                                                             \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                                   \
        HID_USAGE_MIN(1),                                                        \
        HID_USAGE_MAX(14),                                                       \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(1),                                                      \
        HID_REPORT_SIZE(1),                                                      \
        HID_REPORT_COUNT(14),                                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),                              \
        HID_USAGE(0x21),                                                         \
        HID_REPORT_COUNT(14),                                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),                              \
        HID_USAGE(0x22),                                                         \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX_N(255, 2),                                               \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(52),                                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_REPORT_ID(ReportIdPs502)                                             \
            HID_USAGE(0x23),                                                     \
        HID_REPORT_COUNT(47),                                                    \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                      \
        HID_REPORT_ID(ReportIdPs5Feature)                                        \
            HID_USAGE_N(0x2821, 2),                                              \
        HID_REPORT_COUNT(47),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_USAGE_PAGE_N(0xFF80, 2),                                             \
        HID_REPORT_ID(ReportIdPs5E0)                                             \
            HID_USAGE(0x57),                                                     \
        HID_REPORT_COUNT(2),                                                     \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_COLLECTION_END,                                                      \
        HID_USAGE_PAGE_N(0xFFF0, 2),                                             \
        HID_USAGE(0x40),                                                         \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
        HID_REPORT_ID(ReportIdPs5SetChallenge)                                   \
            HID_USAGE(0x47),                                                     \
        HID_REPORT_COUNT(63),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_REPORT_ID(ReportIdPs5GetResponse)                                    \
            HID_USAGE(0x48),                                                     \
        HID_REPORT_COUNT(63),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_REPORT_ID(ReportIdPs5GetAuthStatus)                                  \
            HID_USAGE(0x49),                                                     \
        HID_REPORT_COUNT(15),                                                    \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                     \
        HID_COLLECTION_END

#define TUD_HID_REPORT_SWITCH()                                                  \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                      \
        HID_LOGICAL_MIN(0x00),                                                   \
        HID_USAGE(HID_USAGE_DESKTOP_JOYSTICK),                                   \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
        HID_REPORT_ID(ReportSwitchOutput30)                                      \
            HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                              \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                                   \
        HID_USAGE_MIN(0x01),                                                     \
        HID_USAGE_MAX(0x0A),                                                     \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(1),                                                      \
        HID_REPORT_SIZE(1),                                                      \
        HID_REPORT_COUNT(10),                                                    \
        HID_UNIT(0),                                                             \
        HID_UNIT_EXPONENT(0),                                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                                   \
        HID_USAGE_MIN(0x0B),                                                     \
        HID_USAGE_MAX(0x0E),                                                     \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(1),                                                      \
        HID_REPORT_SIZE(1),                                                      \
        HID_REPORT_COUNT(4),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_REPORT_SIZE(1),                                                      \
        HID_REPORT_COUNT(2),                                                     \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                   \
        HID_USAGE_N(0x010001, 3),                                                \
        HID_COLLECTION(HID_COLLECTION_PHYSICAL),                                 \
        HID_USAGE_N(0x010030, 3),                                                \
        HID_USAGE_N(0x010031, 3),                                                \
        HID_USAGE_N(0x010032, 3),                                                \
        HID_USAGE_N(0x010035, 3),                                                \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX_N(65534, 3),                                             \
        HID_REPORT_SIZE(16),                                                     \
        HID_REPORT_COUNT(4),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_COLLECTION_END,                                                      \
        HID_USAGE_N(0x010039, 3),                                                \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(7),                                                      \
        HID_PHYSICAL_MAX_N(315, 2),                                              \
        HID_UNIT(0x14), /*Unit (System: English Rotation, Length: Centimeter) */ \
        HID_REPORT_SIZE(4),                                                      \
        HID_REPORT_COUNT(1),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                                   \
        HID_LOGICAL_MIN(0),                                                      \
        HID_LOGICAL_MAX(1),                                                      \
        HID_USAGE_MIN(0x0F),                                                     \
        HID_USAGE_MAX(0x12),                                                     \
        HID_REPORT_SIZE(1),                                                      \
        HID_REPORT_COUNT(4),                                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                       \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(52),                                                    \
        HID_OUTPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                  \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),                              \
        HID_REPORT_ID(ReportSwitchOutput21)                                      \
            HID_USAGE(0x01),                                                     \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(63),                                                    \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                   \
        HID_REPORT_ID(ReportSwitchInput)                                         \
            HID_USAGE(0x02),                                                     \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(63),                                                    \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                   \
        HID_REPORT_ID(ReportSwitchFeature)                                       \
            HID_USAGE(0x03),                                                     \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(63),                                                    \
        HID_OUTPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                  \
        HID_REPORT_ID(ReportSwitchOutput10)                                      \
            HID_USAGE(0x04),                                                     \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(63),                                                    \
        HID_OUTPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                  \
        HID_REPORT_ID(ReportSwitchConfiguration)                                 \
            HID_USAGE(0x05),                                                     \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(63),                                                    \
        HID_OUTPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                  \
        HID_REPORT_ID(ReportSwitchOutput82)                                      \
            HID_USAGE(0x06),                                                     \
        HID_REPORT_SIZE(8),                                                      \
        HID_REPORT_COUNT(63),                                                    \
        HID_OUTPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                  \
        HID_COLLECTION_END
