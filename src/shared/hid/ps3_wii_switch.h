#include <stdint.h>

#include "config.h"
#include "progmem.h"
#if DEVICE_TYPE != GUITAR_PRAISE_GUITAR
const uint8_t PROGMEM ps3_instrument_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x01,        //   Physical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0E,        //   Report Count (14)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0E,        //   Usage Maximum (0x0E)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,        //   Usage (Hat switch)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x46, 0xFF, 0x00,  //   Physical Maximum (255)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x09, 0x21,        //   Usage (0x21)
    0x09, 0x22,        //   Usage (0x22)
    0x09, 0x23,        //   Usage (0x23)
    0x09, 0x24,        //   Usage (0x24)
    0x09, 0x25,        //   Usage (0x25)
    0x09, 0x26,        //   Usage (0x26)
    0x09, 0x27,        //   Usage (0x27)
    0x09, 0x28,        //   Usage (0x28)
    0x09, 0x29,        //   Usage (0x29)
    0x09, 0x2A,        //   Usage (0x2A)
    0x09, 0x2B,        //   Usage (0x2B)
    0x95, 0x0C,        //   Report Count (12)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x95, 0x20,        //   Report Count (32)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x46, 0xFF, 0x03,  //   Physical Maximum (1023)
    0x09, 0x2C,        //   Usage (0x2C)
    0x09, 0x2D,        //   Usage (0x2D)
    0x09, 0x2E,        //   Usage (0x2E)
    0x09, 0x2F,        //   Usage (0x2F)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
};
#if DEVICE_TYPE_IS_INSTRUMENT
const uint8_t PROGMEM pc_descriptor[] = {
    0x05, 0x01,              // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,              // Usage (Game Pad)
    0xA1, 0x01,              // Collection (Application)
    0x85, 0x01,              //   Report ID (1)
    0x15, 0x00,              //   Logical Minimum (0)
    0x25, 0x01,              //   Logical Maximum (1)
    0x35, 0x00,              //   Physical Minimum (0)
    0x45, 0x01,              //   Physical Maximum (1)
    0x75, 0x01,              //   Report Size (1)
    0x95, HID_BUTTON_COUNT,  //   Report Count (HID_BUTTON_COUNT)
    0x05, 0x09,              //   Usage Page (Button)
    0x19, 0x01,              //   Usage Minimum (0x01)
    0x29, HID_BUTTON_COUNT,  //   Usage Maximum (HID_BUTTON_COUNT)
    0x81, 0x02,              //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
#if HID_BUTTON_PADDING
    0x95, HID_BUTTON_PADDING,  //   Report Count (HID_BUTTON_PADDING)
    0x81, 0x01,                //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
#endif
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,        //   Usage (Hat switch)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x46, 0xFF, 0x00,  //   Physical Maximum (255)
#if HID_AXIS_COUNT >= 1
    0x09, 0x30,  //   Usage (X)
#endif
#if HID_AXIS_COUNT >= 2
    0x09, 0x31,  //   Usage (X)
#endif
#if HID_AXIS_COUNT >= 3
    0x09, 0x32,  //   Usage (X)
#endif
#if HID_AXIS_COUNT >= 4
    0x09, 0x33,  //   Usage (X)
#endif
#if HID_AXIS_COUNT >= 5
    0x09, 0x34,  //   Usage (X)
#endif
#if HID_AXIS_COUNT >= 6
    0x09, 0x35,  //   Usage (X)
#endif
#if HID_AXIS_COUNT >= 7
    0x09, 0x36,  //   Usage (X)
#endif
    0x75, 0x08,            //   Report Size (8)
    0x95, HID_AXIS_COUNT,  //   Report Count (4)
    0x81, 0x02,            //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,      //   Usage Page (Vendor Defined 0xFF00)
    0x85, 0x03,            //   Report ID (3)
    0x0A, 0x21, 0x27,      //   Usage (0x2721)
    0x95, 0x2F,            //   Report Count (47)
    0xB1, 0x02,            //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x0A, 0x21, 0x26,      //   Usage (0x2621)
    0x95, 0x20,            //   Report Count (32)
    0xB1, 0x02,            //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                  // End Collection
};
#elif DEVICE_TYPE == STAGE_KIT
const uint8_t PROGMEM pc_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x01,        //   Physical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x08,        //   Usage Maximum (0x08)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,        //   Usage (Hat switch)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x03,        //   Report ID (3)
    0x0A, 0x21, 0x27,  //   Usage (0x2721)
    0x95, 0x2F,        //   Report Count (47)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x95, 0x20,        //   Report Count (32)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};
#else
const uint8_t PROGMEM pc_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x01,        //   Physical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0E,        //   Report Count (14)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0E,        //   Usage Maximum (0x0E)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,        //   Usage (Hat switch)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x46, 0xFF, 0x00,  //   Physical Maximum (255)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (X)
    0x09, 0x32,        //   Usage (X)
    0x09, 0x33,        //   Usage (X)
    0x09, 0x34,        //   Usage (X)
    0x09, 0x35,        //   Usage (X)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x06,        //   Report Count (6)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (X)
    0x09, 0x21,        //   Usage (X)
    0x09, 0x22,        //   Usage (X)
    0x09, 0x23,        //   Usage (X)
    0x09, 0x24,        //   Usage (X)
    0x09, 0x25,        //   Usage (X)
    0x09, 0x26,        //   Usage (X)
    0x09, 0x27,        //   Usage (X)
    0x09, 0x28,        //   Usage (X)
    0x09, 0x29,        //   Usage (X)
    0x09, 0x2A,        //   Usage (X)
    0x09, 0x2B,        //   Usage (X)
    0x95, 0x0A,        //   Report Count (10)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x03,        //   Report ID (3)
    0x0A, 0x21, 0x27,  //   Usage (0x2721)
    0x95, 0x2F,        //   Report Count (47)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x95, 0x20,        //   Report Count (32)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};
#endif

const uint8_t PROGMEM ps3_descriptor[] = {
    0x05, 0x01,       /*  Usage Page (Desktop),               */
    0x09, 0x04,       /*  Usage (Joystik),                    */
    0xA1, 0x01,       /*  Collection (Application),           */
    0xA1, 0x02,       /*      Collection (Logical),           */
    0x85, 0x01,       /*          Report ID (1),              */
    0x75, 0x08,       /*          Report Size (8),            */
    0x95, 0x01,       /*          Report Count (1),           */
    0x15, 0x00,       /*          Logical Minimum (0),        */
    0x26, 0xFF, 0x00, /*          Logical Maximum (255),      */
    0x81, 0x03,       /*          Input (Constant, Variable), */
    0x75, 0x01,       /*          Report Size (1),            */
    0x95, 0x13,       /*          Report Count (19),          */
    0x15, 0x00,       /*          Logical Minimum (0),        */
    0x25, 0x01,       /*          Logical Maximum (1),        */
    0x35, 0x00,       /*          Physical Minimum (0),       */
    0x45, 0x01,       /*          Physical Maximum (1),       */
    0x05, 0x09,       /*          Usage Page (Button),        */
    0x19, 0x01,       /*          Usage Minimum (01h),        */
    0x29, 0x13,       /*          Usage Maximum (13h),        */
    0x81, 0x02,       /*          Input (Variable),           */
    0x75, 0x01,       /*          Report Size (1),            */
    0x95, 0x0D,       /*          Report Count (13),          */
    0x06, 0x00, 0xFF, /*          Usage Page (FF00h),         */
    0x81, 0x03,       /*          Input (Constant, Variable), */
    0x15, 0x00,       /*          Logical Minimum (0),        */
    0x26, 0xFF, 0x00, /*          Logical Maximum (255),      */
    0x05, 0x01,       /*          Usage Page (Desktop),       */
    0x09, 0x01,       /*          Usage (Pointer),            */
    0xA1, 0x00,       /*          Collection (Physical),      */
    0x75, 0x08,       /*              Report Size (8),        */
    0x95, 0x04,       /*              Report Count (4),       */
    0x35, 0x00,       /*              Physical Minimum (0),   */
    0x46, 0xFF, 0x00, /*              Physical Maximum (255), */
    0x09, 0x30,       /*              Usage (X),              */
    0x09, 0x31,       /*              Usage (Y),              */
    0x09, 0x32,       /*              Usage (Z),              */
    0x09, 0x35,       /*              Usage (Rz),             */
    0x81, 0x02,       /*              Input (Variable),       */
    0xC0,             /*          End Collection,             */
    0x05, 0x01,       /*          Usage Page (Desktop),       */
    0x95, 0x13,       /*          Report Count (19),          */
    0x09, 0x01,       /*          Usage (Pointer),            */
    0x81, 0x02,       /*          Input (Variable),           */
    0x95, 0x0C,       /*          Report Count (12),          */
    0x81, 0x01,       /*          Input (Constant),           */
    0x75, 0x10,       /*          Report Size (16),           */
    0x95, 0x04,       /*          Report Count (4),           */
    0x26, 0xFF, 0x03, /*          Logical Maximum (1023),     */
    0x46, 0xFF, 0x03, /*          Physical Maximum (1023),    */
    0x09, 0x01,       /*          Usage (Pointer),            */
    0x81, 0x02,       /*          Input (Variable),           */
    0xC0,             /*      End Collection,                 */
    0xA1, 0x02,       /*      Collection (Logical),           */
    0x85, 0x02,       /*          Report ID (2),              */
    0x75, 0x08,       /*          Report Size (8),            */
    0x95, 0x30,       /*          Report Count (48),          */
    0x09, 0x01,       /*          Usage (Pointer),            */
    0xB1, 0x02,       /*          Feature (Variable),         */
    0xC0,             /*      End Collection,                 */
    0xA1, 0x02,       /*      Collection (Logical),           */
    0x85, 0xEE,       /*          Report ID (238),            */
    0x75, 0x08,       /*          Report Size (8),            */
    0x95, 0x30,       /*          Report Count (48),          */
    0x09, 0x01,       /*          Usage (Pointer),            */
    0xB1, 0x02,       /*          Feature (Variable),         */
    0xC0,             /*      End Collection,                 */
    0xA1, 0x02,       /*      Collection (Logical),           */
    0x85, 0xEF,       /*          Report ID (239),            */
    0x75, 0x08,       /*          Report Size (8),            */
    0x95, 0x30,       /*          Report Count (48),          */
    0x09, 0x01,       /*          Usage (Pointer),            */
    0xB1, 0x02,       /*          Feature (Variable),         */
    0xC0,             /*      End Collection,                 */
    0xC0              /*  End Collection                      */
};

const uint8_t PROGMEM ps4_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x09, 0x39,        //   Usage (Hat switch)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)

    0x65, 0x00,  //   Unit (None)
    0x05, 0x09,  //   Usage Page (Button)
    0x19, 0x01,  //   Usage Minimum (0x01)
    0x29, 0x0E,  //   Usage Maximum (0x0E)
    0x15, 0x00,  //   Logical Minimum (0)
    0x25, 0x01,  //   Logical Maximum (1)
    0x75, 0x01,  //   Report Size (1)
    0x95, 0x0E,  //   Report Count (14)
    0x81, 0x02,  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x75, 0x06,        //   Report Size (6)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x21,        //   Usage (0x21)
    0x95, 0x36,        //   Report Count (54)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

    0x85, 0x05,  //   Report ID (5)
    0x09, 0x22,  //   Usage (0x22)
    0x95, 0x1F,  //   Report Count (31)
    0x91, 0x02,  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

    0x85, 0x03,        //   Report ID (3)
    0x0A, 0x21, 0x27,  //   Usage (0x2721)
    0x95, 0x2F,        //   Report Count (47)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection

    0x06, 0xF0, 0xFF,  // Usage Page (Vendor Defined 0xFFF0)
    0x09, 0x40,        // Usage (0x40)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0xF0,        //   Report ID (-16)
    0x09, 0x47,        //   Usage (0x47)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF1,        //   Report ID (-15)
    0x09, 0x48,        //   Usage (0x48)
    0x95, 0x3F,        //   Report Count (63)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF2,        //   Report ID (-14)
    0x09, 0x49,        //   Usage (0x49)
    0x95, 0x0F,        //   Report Count (15)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0xF3,        //   Report ID (-13)
    0x0A, 0x01, 0x47,  //   Usage (0x4701)
    0x95, 0x07,        //   Report Count (7)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};
#else

const uint8_t PROGMEM ps3_instrument_descriptor[]{
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04,        // Usage (Joystick)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x00,        //   Usage (Undefined)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x32,        //     Usage (Z)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0xFF,        //     Logical Maximum (-1)
    0x35, 0x00,        //     Physical Minimum (0)
    0x45, 0xFF,        //     Physical Maximum (-1)
    0x65, 0x00,        //     Unit (None)
    0x55, 0x00,        //     Unit Exponent (0)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x35,        //     Usage (Rz)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x30,        //     Usage (X)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x39,        //     Usage (Hat switch)
    0x25, 0x07,        //     Logical Maximum (7)
    0x46, 0x3B, 0x01,  //     Physical Maximum (315)
    0x65, 0x14,        //     Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,        //     Report Size (4)
    0x81, 0x42,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x08,        //     Usage Maximum (0x08)
    0x25, 0x01,        //     Logical Maximum (1)
    0x45, 0x01,        //     Physical Maximum (1)
    0x65, 0x00,        //     Unit (None)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x08,        //     Report Count (8)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x0C,        //     Report Count (12)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC1, 0x00,        //   End Collection
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x00,        //   Usage (0x00)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x02,        //     Usage (0x02)
    0x25, 0xFF,        //     Logical Maximum (-1)
    0x45, 0xFF,        //     Physical Maximum (-1)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x04,        //     Report Count (4)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC1, 0x00,        //   End Collection
    0xC1, 0x00,        // End Collection
};
#endif