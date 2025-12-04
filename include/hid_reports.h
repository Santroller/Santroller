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

#define TUD_HID_REPORT_DESC_GENERIC_INFEATURE(report_size, ...)          \
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

#define TUD_HID_REPORT_DESC_PS3_4_5()                        \
    HID_USAGE_N(0x2621, 2),                                  \
        HID_REPORT_SIZE(8),                                  \
        HID_REPORT_COUNT(8),                                 \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),  \
        HID_REPORT_ID(ReportIdPs4Feature)                    \
            HID_USAGE_N(0x2721, 2),                          \
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
        HID_REPORT_COUNT(0x0A),                 \
        HID_LOGICAL_MIN(0x00),                  \
        HID_LOGICAL_MAX_N(0xff, 2),             \
        HID_REPORT_SIZE(8),                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE)

#define TUD_HID_REPORT_DESC_GAME_CONTROLLER(rid)                                                                       \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                                                            \
        HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                                                                          \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),                                                                    \
        rid                                                                                                            \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), /* 8 bit DPad/Hat Button Map  */                                       \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),                                                                       \
        HID_LOGICAL_MAX(7),                                                                                            \
        HID_PHYSICAL_MAX_N(315, 2),                                                                                    \
        HID_REPORT_COUNT(1),                                                                                           \
        HID_REPORT_SIZE(8),                                                                                            \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                                                             \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                                                                         \
        HID_LOGICAL_MIN(0),                                                                                            \
        HID_LOGICAL_MAX(1),                                                                                            \
        HID_PHYSICAL_MIN(0),                                                                                           \
        HID_PHYSICAL_MAX(1),                                                                                           \
        HID_REPORT_COUNT(15),                                                                                          \
        HID_REPORT_SIZE(1),                                                                                            \
        HID_USAGE(BTN_START),                                                                                          \
        HID_USAGE(BTN_SELECT),                                                                                         \
        HID_USAGE(BTN_THUMBL),                                                                                         \
        HID_USAGE(BTN_THUMBR),                                                                                         \
        HID_USAGE(BTN_TL),                                                                                             \
        HID_USAGE(BTN_TR),                                                                                             \
        HID_USAGE(BTN_GUIDE),                                                                                          \
        HID_USAGE(BTN_C),                                                                                              \
        HID_USAGE(BTN_A),                                                                                              \
        HID_USAGE(BTN_B),                                                                                              \
        HID_USAGE(BTN_X),                                                                                              \
        HID_USAGE(BTN_Y),                                                                                              \
        HID_USAGE(BTN_TL2),                                                                                            \
        HID_USAGE(BTN_TR2),                                                                                            \
        HID_USAGE(BTN_Z),                                                                                              \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                                                             \
        HID_REPORT_COUNT(1),                                                                                           \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                                                         \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                                                        \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                                                                                \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                                                                               \
        HID_LOGICAL_MAX_N(0xff, 2),                                                                                    \
        HID_PHYSICAL_MAX_N(0xff, 2),                                                                                   \
        HID_REPORT_COUNT(2),                                                                                           \
        HID_REPORT_SIZE(8),                                                                                            \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                                                             \
        HID_USAGE(HID_USAGE_DESKTOP_X),                                                                                \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                                                                                \
        HID_USAGE(HID_USAGE_DESKTOP_RX),                                                                               \
        HID_USAGE(HID_USAGE_DESKTOP_RY),                                                                               \
        HID_LOGICAL_MIN_N(INT16_MIN, 2),                                                                               \
        HID_PHYSICAL_MIN_N(INT16_MIN, 2),                                                                              \
        HID_LOGICAL_MAX_N(INT16_MAX, 2),                                                                               \
        HID_PHYSICAL_MAX_N(INT16_MAX, 2),                                                                              \
        HID_REPORT_COUNT(4),                                                                                           \
        HID_REPORT_SIZE(16),                                                                                           \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),                                                             \
        TUD_HID_REPORT_DESC_PS3_VENDOR(),                                                                              \
        TUD_HID_REPORT_DESC_PS3_4_5(),                                                                                 \
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
        HID_REPORT_COUNT(13),                                  \
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

#define TUD_HID_REPORT_DESC_PS4_FIRSTPARTY_GAMEPAD(rid)        \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                    \
        HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                  \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),            \
        rid                                                    \
        HID_USAGE(HID_USAGE_DESKTOP_X),                        \
        HID_USAGE(HID_USAGE_DESKTOP_Y),                        \
        HID_USAGE(HID_USAGE_DESKTOP_Z),                        \
        HID_USAGE(HID_USAGE_DESKTOP_RZ),                       \
        HID_LOGICAL_MIN(0x00),                                 \
        HID_LOGICAL_MAX_N(0xFF, 2),                            \
        HID_REPORT_COUNT(4),                                   \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),               \
        HID_LOGICAL_MIN(0),                                    \
        HID_LOGICAL_MAX(7),                                    \
        HID_PHYSICAL_MAX_N(315, 2),                            \
        HID_REPORT_COUNT(1),                                   \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),                 \
        HID_LOGICAL_MIN(0),                                    \
        HID_LOGICAL_MAX(1),                                    \
        HID_REPORT_COUNT(14),                                  \
        HID_REPORT_SIZE(1),                                    \
        HID_USAGE_MIN(0x01),                                   \
        HID_USAGE_MAX(14),                                     \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),            \
        HID_USAGE(0x20),                                       \
        HID_REPORT_SIZE(6),                                    \
        HID_REPORT_COUNT(1),                                   \
        HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                \
        HID_USAGE(HID_USAGE_DESKTOP_RX),                       \
        HID_USAGE(HID_USAGE_DESKTOP_RY),                       \
        HID_LOGICAL_MIN(0x00),                                 \
        HID_LOGICAL_MAX_N(0xFF, 2),                            \
        HID_REPORT_COUNT(2),                                   \
        HID_REPORT_SIZE(8),                                    \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),            \
        HID_USAGE(0x21),                                       \
        HID_REPORT_COUNT(54),                                  \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),     \
        HID_REPORT_ID(ReportIdPs405)                           \
            HID_USAGE(0x22),                                   \
        HID_REPORT_COUNT(31),                                  \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),    \
        HID_REPORT_ID(ReportIdPs4Feature)                      \
            HID_USAGE_N(0x2721, 2),                            \
        HID_REPORT_COUNT(47),                                  \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_COLLECTION_END,                                    \
        HID_USAGE_PAGE_N(0xFFF0, 2),                           \
        HID_USAGE(0x40),                                       \
        HID_COLLECTION(HID_COLLECTION_APPLICATION),            \
        HID_REPORT_ID(ReportIdPs4SetChallenge)                 \
            HID_USAGE(0x47),                                   \
        HID_REPORT_COUNT(63),                                  \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_REPORT_ID(ReportIdPs4GetResponse)                  \
            HID_USAGE(0x48),                                   \
        HID_REPORT_COUNT(63),                                  \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_REPORT_ID(ReportIdPs4GetAuthStatus)                \
            HID_USAGE(0x49),                                   \
        HID_REPORT_COUNT(15),                                  \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_REPORT_ID(ReportIdPs4GetAuthPageSize)              \
            HID_USAGE_N(0x4701, 2),                            \
        HID_REPORT_COUNT(7),                                   \
        HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
        HID_COLLECTION_END
