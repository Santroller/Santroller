#include "report_descriptors.h"
typedef enum {
    THID_USAGE_POINTER = 0x01,
    THID_USAGE_MOUSE = 0x02,
    THID_USAGE_GAMEPAD = 0x05,
    THID_USAGE_KEYBOARD = 0x06,
    THID_USAGE_HAT_SWITCH = 0x39,
    THID_USAGE_X = 0x30,
    THID_USAGE_Y,
    THID_USAGE_Z,
    THID_USAGE_Rx,
    THID_USAGE_Ry,
    THID_USAGE_Rz,
    THID_USAGE_Slider,
    THID_USAGE_Dial,
    THID_USAGE_Wheel,
    THID_USAGE_COUNTED_BUFFER = 0x3A,
    THID_USAGE_CONSUMER_AC_PAN = 0x0238
} THID_Usage;

typedef enum {
    THID_USAGE_PAGE_GENERIC_DESKTOP = 0x01,
    THID_USAGE_PAGE_KEYBOARD = 0x07,
    THID_USAGE_PAGE_LED = 0x08,
    THID_USAGE_PAGE_BUTTON = 0x09,
    THID_USAGE_PAGE_CONSUMER = 0x0c,
} THID_Usage_Page;

typedef enum {
    THID_COLLECTION_PHYSICAL = 0x00,
    THID_COLLECTION_APPLICATION = 0x01,
    THID_COLLECTION_LOGICAL = 0x02,
} THID_Collection;

typedef enum {
    THID_UNIT_NONE,
    THID_UNIT_DEGREES = 0x14
} THID_Unit;

const TUSB_Descriptor_HIDReport_Datatype_t ps3_report_descriptor[] = {
    // Controller
    THID_RI_USAGE_PAGE(8, THID_USAGE_PAGE_GENERIC_DESKTOP),
    THID_RI_USAGE(8, THID_USAGE_GAMEPAD),
    THID_RI_COLLECTION(8, THID_COLLECTION_APPLICATION),
    THID_RI_LOGICAL_MINIMUM(8, 0),
    THID_RI_LOGICAL_MAXIMUM(8, 1),
    THID_RI_PHYSICAL_MINIMUM(8, 0),
    THID_RI_PHYSICAL_MAXIMUM(8, 1),
    THID_RI_REPORT_SIZE(8, 1),
    THID_RI_REPORT_COUNT(8, 13),
    THID_RI_USAGE_PAGE(8, THID_USAGE_PAGE_BUTTON),
    THID_RI_USAGE_MINIMUM(8, 0x01),
    THID_RI_USAGE_MAXIMUM(8, 0x0D),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                        THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                        THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION),
    THID_RI_REPORT_COUNT(8, 0x03),
    THID_RI_INPUT(8, THID_IOF_CONSTANT | THID_IOF_ARRAY | THID_IOF_ABSOLUTE |
                        THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                        THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION),
    THID_RI_USAGE_PAGE(8, THID_USAGE_PAGE_GENERIC_DESKTOP),
    THID_RI_LOGICAL_MAXIMUM(8, 7),
    THID_RI_PHYSICAL_MAXIMUM(16, 315),
    THID_RI_REPORT_SIZE(8, 4),
    THID_RI_REPORT_COUNT(8, 0x01),
    THID_RI_UNIT(8, THID_UNIT_DEGREES),
    THID_RI_USAGE(8, THID_USAGE_HAT_SWITCH),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                        THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                        THID_IOF_PREFERRED_STATE | THID_IOF_NULLSTATE),
    THID_RI_UNIT(8, THID_UNIT_NONE),
    THID_RI_REPORT_COUNT(8, 0x01),
    THID_RI_INPUT(8, THID_IOF_CONSTANT | THID_IOF_ARRAY | THID_IOF_ABSOLUTE |
                        THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                        THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION),
    THID_RI_LOGICAL_MAXIMUM(16, 255),
    THID_RI_PHYSICAL_MAXIMUM(16, 255),
    THID_RI_USAGE(8, THID_USAGE_X),
    THID_RI_USAGE(8, THID_USAGE_Y),
    THID_RI_USAGE(8, THID_USAGE_Z),
    THID_RI_USAGE(8, THID_USAGE_Rz),
    THID_RI_REPORT_SIZE(8, 8),
    THID_RI_REPORT_COUNT(8, 4),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                        THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                        THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION),
    THID_RI_USAGE_PAGE(16, 0xFF00),
    THID_RI_USAGE(8, 0x20),
    THID_RI_USAGE(8, 0x21),
    THID_RI_USAGE(8, 0x22),
    THID_RI_USAGE(8, 0x23),
    THID_RI_USAGE(8, 0x24),
    THID_RI_USAGE(8, 0x25),
    THID_RI_USAGE(8, 0x26),
    THID_RI_USAGE(8, 0x27),
    THID_RI_USAGE(8, 0x28),
    THID_RI_USAGE(8, 0x29),
    THID_RI_USAGE(8, 0x2A),
    THID_RI_USAGE(8, 0x2B),
    THID_RI_REPORT_COUNT(8, 12),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                        THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                        THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION),
    THID_RI_USAGE(16, 0x2621),
    THID_RI_REPORT_COUNT(8, 32),
    THID_RI_FEATURE(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                          THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                          THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION | THID_IOF_NON_VOLATILE),
    THID_RI_USAGE(16, 0x2621),
    THID_RI_OUTPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                         THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                         THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION |
                         THID_IOF_NON_VOLATILE),
    THID_RI_LOGICAL_MAXIMUM(16, 1023),
    THID_RI_PHYSICAL_MAXIMUM(16, 1023),
    THID_RI_USAGE(8, 0x2C),
    THID_RI_USAGE(8, 0x2D),
    THID_RI_USAGE(8, 0x2E),
    THID_RI_USAGE(8, 0x2F),
    THID_RI_REPORT_SIZE(8, 16),
    THID_RI_REPORT_COUNT(8, 4),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                        THID_IOF_NO_WRAP | THID_IOF_LINEAR |
                        THID_IOF_PREFERRED_STATE | THID_IOF_NO_NULL_POSITION |
                        THID_IOF_NON_VOLATILE),
    THID_RI_END_COLLECTION(0),
};

const TUSB_Descriptor_HIDReport_Datatype_t kbd_report_descriptor[] = {
    THID_RI_USAGE_PAGE(8, THID_USAGE_PAGE_GENERIC_DESKTOP),
    THID_RI_USAGE(8, THID_USAGE_KEYBOARD),
    THID_RI_COLLECTION(8, 0),
    THID_RI_REPORT_ID(8, REPORT_ID_KBD),
    THID_RI_USAGE_PAGE(8, 0),
    THID_RI_USAGE_MINIMUM(8, 0xE0),
    THID_RI_USAGE_MAXIMUM(8, 0xE7),
    THID_RI_LOGICAL_MINIMUM(8, 0x00),
    THID_RI_LOGICAL_MAXIMUM(8, 0x01),
    THID_RI_REPORT_SIZE(8, 0x01),
    THID_RI_REPORT_COUNT(8, 0x08),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),
    THID_RI_REPORT_COUNT(8, 0x01),
    THID_RI_REPORT_SIZE(8, 0x08),
    THID_RI_INPUT(8, THID_IOF_CONSTANT),
    THID_RI_USAGE_PAGE(8, 0),
    THID_RI_USAGE_MINIMUM(8, 0x01),
    THID_RI_USAGE_MAXIMUM(8, 0x05),
    THID_RI_REPORT_COUNT(8, 0x05),
    THID_RI_REPORT_SIZE(8, 0x01),
    THID_RI_OUTPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE |
                         THID_IOF_NON_VOLATILE),
    THID_RI_REPORT_COUNT(8, 0x01),
    THID_RI_REPORT_SIZE(8, 0x03),
    THID_RI_OUTPUT(8, THID_IOF_CONSTANT),
    THID_RI_LOGICAL_MINIMUM(8, 0x00),
    THID_RI_LOGICAL_MAXIMUM(16, 0xFF),
    THID_RI_USAGE_PAGE(8, 0x07),
    THID_RI_USAGE_MINIMUM(8, 0x00),
    THID_RI_USAGE_MAXIMUM(8, 0xFF),
    THID_RI_REPORT_COUNT(8, SIMULTANEOUS_KEYS),
    THID_RI_REPORT_SIZE(8, 0x08),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_ARRAY | THID_IOF_ABSOLUTE),
    THID_RI_END_COLLECTION(0),
    THID_RI_USAGE_PAGE(8, THID_USAGE_PAGE_GENERIC_DESKTOP),
    THID_RI_USAGE(8, THID_USAGE_MOUSE),
    THID_RI_COLLECTION(8, 0),
    THID_RI_REPORT_ID(8, REPORT_ID_MOUSE),
    THID_RI_USAGE(8, THID_USAGE_POINTER),
    THID_RI_COLLECTION(8, 0),
    THID_RI_USAGE_PAGE(8, 0),
    THID_RI_USAGE_MINIMUM(8, 0x01),
    THID_RI_USAGE_MAXIMUM(8, Buttons),
    THID_RI_LOGICAL_MINIMUM(8, 0x00),
    THID_RI_LOGICAL_MAXIMUM(8, 0x01),
    THID_RI_REPORT_COUNT(8, Buttons),
    THID_RI_REPORT_SIZE(8, 0x01),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),
    THID_RI_REPORT_SIZE(8, (8 - (Buttons % 8))),
    THID_RI_REPORT_COUNT(8, 0x01),
    THID_RI_INPUT(8, THID_IOF_CONSTANT | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),
    THID_RI_USAGE_PAGE(8, THID_USAGE_PAGE_GENERIC_DESKTOP),
    THID_RI_USAGE(8, THID_USAGE_X),
    THID_RI_USAGE(8, THID_USAGE_Y),
    THID_RI_USAGE(8, THID_USAGE_Wheel),
    THID_RI_LOGICAL_MINIMUM(8, -127),
    THID_RI_LOGICAL_MAXIMUM(8, 127),
    THID_RI_REPORT_COUNT(8, 0x03),
    THID_RI_REPORT_SIZE(8, 8),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE |
                        (AbsoluteCoords ? THID_IOF_ABSOLUTE : THID_IOF_RELATIVE)),
    THID_RI_USAGE_PAGE(8, 0),
    THID_RI_USAGE(16, 0),
    THID_RI_LOGICAL_MINIMUM(16, -127),
    THID_RI_LOGICAL_MAXIMUM(16, 127),
    THID_RI_REPORT_COUNT(8, 0x01),
    THID_RI_REPORT_SIZE(8, 8),
    THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_RELATIVE),
    THID_RI_END_COLLECTION(0),
    THID_RI_END_COLLECTION(0),
};

const TUSB_Descriptor_HIDReport_Datatype_t switch_report_descriptor[] = {
    THID_RI_USAGE_PAGE(8,1),                         // Generic desktop controls
    THID_RI_LOGICAL_MINIMUM(8,0),                    // Logical Minimum (0)
    THID_RI_USAGE(8,4),                              // Joystick
    THID_RI_COLLECTION(8,1),                         // Application

    THID_RI_REPORT_ID(8,48),                         // Report ID (48)
    THID_RI_USAGE_PAGE(8,1),                         // Generic desktop controls
    THID_RI_USAGE_PAGE(8,9),                         // Button
    THID_RI_USAGE_MINIMUM(8,1),                      // Usage Minimum (0x01)
    THID_RI_USAGE_MAXIMUM(8,0x0A),                   // Usage Maximum (0x0A)
    THID_RI_LOGICAL_MINIMUM(8,0),                    // button off state
    THID_RI_LOGICAL_MAXIMUM(8,1),                    // button on state
    THID_RI_REPORT_SIZE(8,1),                        // 1 bit per report field
    THID_RI_REPORT_COUNT(8,10),                      // 10 report fields (10 buttons)
    THID_RI_UNIT_EXPONENT(8,0),                      //
    THID_RI_UNIT(8,0),                               // no unit
    THID_RI_INPUT(8,2),                              // Variable input
    THID_RI_USAGE_PAGE(8,9),                         // Button
    THID_RI_USAGE_MINIMUM(8,0x0B),                   // Usage Minimum (0x0B)
    THID_RI_USAGE_MAXIMUM(8,0x0E),                   // Usage Maximum (0x0E)
    THID_RI_LOGICAL_MINIMUM(8,0),                    // button off state
    THID_RI_LOGICAL_MAXIMUM(8,1),                    // button on state
    THID_RI_REPORT_SIZE(8,1),                        // 1 bit per report field
    THID_RI_REPORT_COUNT(8,4),                       // 4 report fields (4 buttons)
    THID_RI_INPUT(8,2),                              // Variable input
    THID_RI_REPORT_SIZE(8,1),                        //
    THID_RI_REPORT_COUNT(8,2),                       // 2 empty bits?
    THID_RI_INPUT(8,3),                              // Abs input?

    /*
     * Probably joystick data (16 bits per axis)
     */
    THID_RI_USAGE(32,0x010001),                      // Generic Desktop: Pointer
    THID_RI_COLLECTION(8,0),                         // Physical
    THID_RI_USAGE(32,0x010030),                      // Axis 0
    THID_RI_USAGE(32,0x010031),                      // Axis 1
    THID_RI_USAGE(32,0x010032),                      // Axis 2
    THID_RI_USAGE(32,0x010035),                      // Axis 3
    THID_RI_LOGICAL_MINIMUM(8,0),
    THID_RI_LOGICAL_MAXIMUM(32,65535),
    THID_RI_REPORT_SIZE(8,16),
    THID_RI_REPORT_COUNT(8,4),
    THID_RI_INPUT(8,2),                              // Variable input
    THID_RI_END_COLLECTION(0),

    /*
     * DPAD (HAT)
     */
    THID_RI_USAGE(32,0x010039),
    THID_RI_LOGICAL_MINIMUM(8,0),
    THID_RI_LOGICAL_MAXIMUM(8,7),                    // 8 valid HAT states, sending 0x08 = nothing pressed
    THID_RI_PHYSICAL_MINIMUM(8,0),
    THID_RI_PHYSICAL_MAXIMUM(16,315),                // HAT "rotation"
    THID_RI_UNIT(8,0x14),                            // System: English Rotation, Length: Centimeter
    THID_RI_REPORT_SIZE(8,4),                        // 4 bits per report field
    THID_RI_REPORT_COUNT(8,1),                       // 1 report field (a nibble containing entire HAT state)
    THID_RI_INPUT(8,2),                              // Variable input

    /*
     * Four unrecognized buttons
     */
    THID_RI_USAGE_PAGE(8,9),                         // Button
    THID_RI_USAGE_MINIMUM(8,0x0F),                   // Usage Minimum (0x0F)
    THID_RI_USAGE_MAXIMUM(8,0x12),                   // Usage Maximum (0x12)
    THID_RI_LOGICAL_MINIMUM(8,0),                    // button off state
    THID_RI_LOGICAL_MAXIMUM(8,1),                    // button on state
    THID_RI_REPORT_SIZE(8,1),                        // 1 bit per report field
    THID_RI_REPORT_COUNT(8,4),                       // 4 report fields (4 buttons)
    THID_RI_INPUT(8,2),                              // Variable input
    THID_RI_REPORT_SIZE(8,8),                        //
    THID_RI_REPORT_COUNT(8,52),                      //
    THID_RI_INPUT(8,3),                              // Input

    /*
     * Vendor defined
     */
    THID_RI_USAGE_PAGE(16,0xFF00),

    THID_RI_REPORT_ID(8,33),                         // Report ID (33)
    THID_RI_USAGE(8,1),                              // Vendor defined: 1
    THID_RI_REPORT_SIZE(8,8),
    THID_RI_REPORT_COUNT(8,63),
    THID_RI_INPUT(8,3),                              // Input

    THID_RI_REPORT_ID(8,0x81),                       // Report ID (-127)
    THID_RI_USAGE(8,2),                              // Vendor defined: 2
    THID_RI_REPORT_SIZE(8,8),
    THID_RI_REPORT_COUNT(8,63),
    THID_RI_INPUT(8,3),                              // Input

    THID_RI_REPORT_ID(8,1),                          // Report ID (1)
    THID_RI_USAGE(8,3),                              // Vendor defined: 3
    THID_RI_REPORT_SIZE(8,8),
    THID_RI_REPORT_COUNT(8,63),
    THID_RI_OUTPUT(8,0x83),                          // Output

    THID_RI_REPORT_ID(8,16),                         // Report ID (16)
    THID_RI_USAGE(8,4),                              // Vendor defined: 4
    THID_RI_REPORT_SIZE(8,8),
    THID_RI_REPORT_COUNT(8,63),
    THID_RI_OUTPUT(8,0x83),                          // Output

    THID_RI_REPORT_ID(8,0x80),                       // Report ID (-128)
    THID_RI_USAGE(8,5),                              // Vendor defined: 5
    THID_RI_REPORT_SIZE(8,8),
    THID_RI_REPORT_COUNT(8,63),
    THID_RI_OUTPUT(8,0x83),                          // Output

    THID_RI_REPORT_ID(8,0x82),                       // Report ID (-126)
    THID_RI_USAGE(8,6),                              // Vendor defined: 6
    THID_RI_REPORT_SIZE(8,8),
    THID_RI_REPORT_COUNT(8,63),
    THID_RI_OUTPUT(8,0x83),                          // Output

    THID_RI_END_COLLECTION(0),
};