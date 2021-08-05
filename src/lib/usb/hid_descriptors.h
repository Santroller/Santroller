#pragma once
#include "std_descriptors.h"

#define TCONCAT(x, y) x##y
#define TCONCAT_EXPANDED(x, y) TCONCAT(x, y)
/* Macros: */
#define THID_RI_DATA_SIZE_MASK 0x03
#define THID_RI_TYPE_MASK 0x0C
#define THID_RI_TAG_MASK 0xF0

#define THID_RI_TYPE_MAIN 0x00
#define THID_RI_TYPE_GLOBAL 0x04
#define THID_RI_TYPE_LOCAL 0x08

#define THID_RI_DATA_BITS_0 0x00
#define THID_RI_DATA_BITS_8 0x01
#define THID_RI_DATA_BITS_16 0x02
#define THID_RI_DATA_BITS_32 0x03
#define THID_RI_DATA_BITS(DataBits) TCONCAT_EXPANDED(THID_RI_DATA_BITS_, DataBits)

#define _THID_RI_ENCODE_0(Data)
#define _THID_RI_ENCODE_8(Data) , (Data & 0xFF)
#define _THID_RI_ENCODE_16(Data) \
    _THID_RI_ENCODE_8(Data)      \
    _THID_RI_ENCODE_8(Data >> 8)
#define _THID_RI_ENCODE_32(Data) \
    _THID_RI_ENCODE_16(Data)     \
    _THID_RI_ENCODE_16(Data >> 16)
#define _THID_RI_ENCODE(DataBits, ...) TCONCAT_EXPANDED(_THID_RI_ENCODE_, DataBits(__VA_ARGS__))

#define _THID_RI_ENTRY(Type, Tag, DataBits, ...) (Type | Tag | THID_RI_DATA_BITS(DataBits)) _THID_RI_ENCODE(DataBits, (__VA_ARGS__))

/* Public Interface - May be used in end-application: */
/* Macros: */
/** \name HID Input, Output and Feature Report Descriptor Item Flags */
/**@{*/
#define THID_IOF_CONSTANT (1 << 0)
#define THID_IOF_DATA (0 << 0)
#define THID_IOF_VARIABLE (1 << 1)
#define THID_IOF_ARRAY (0 << 1)
#define THID_IOF_RELATIVE (1 << 2)
#define THID_IOF_ABSOLUTE (0 << 2)
#define THID_IOF_WRAP (1 << 3)
#define THID_IOF_NO_WRAP (0 << 3)
#define THID_IOF_NON_LINEAR (1 << 4)
#define THID_IOF_LINEAR (0 << 4)
#define THID_IOF_NO_PREFERRED_STATE (1 << 5)
#define THID_IOF_PREFERRED_STATE (0 << 5)
#define THID_IOF_NULLSTATE (1 << 6)
#define THID_IOF_NO_NULL_POSITION (0 << 6)
#define THID_IOF_VOLATILE (1 << 7)
#define THID_IOF_NON_VOLATILE (0 << 7)
#define THID_IOF_BUFFERED_BYTES (1 << 8)
#define THID_IOF_BITFIELD (0 << 8)
/**@}*/

/** \name HID Report Descriptor Item Macros */
/**@{*/
#define THID_RI_INPUT(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_MAIN, 0x80, DataBits, __VA_ARGS__)
#define THID_RI_OUTPUT(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_MAIN, 0x90, DataBits, __VA_ARGS__)
#define THID_RI_COLLECTION(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_MAIN, 0xA0, DataBits, __VA_ARGS__)
#define THID_RI_FEATURE(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_MAIN, 0xB0, DataBits, __VA_ARGS__)
#define THID_RI_END_COLLECTION(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_MAIN, 0xC0, DataBits, __VA_ARGS__)
#define THID_RI_USAGE_PAGE(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x00, DataBits, __VA_ARGS__)
#define THID_RI_LOGICAL_MINIMUM(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x10, DataBits, __VA_ARGS__)
#define THID_RI_LOGICAL_MAXIMUM(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x20, DataBits, __VA_ARGS__)
#define THID_RI_PHYSICAL_MINIMUM(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x30, DataBits, __VA_ARGS__)
#define THID_RI_PHYSICAL_MAXIMUM(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x40, DataBits, __VA_ARGS__)
#define THID_RI_UNIT_EXPONENT(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x50, DataBits, __VA_ARGS__)
#define THID_RI_UNIT(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x60, DataBits, __VA_ARGS__)
#define THID_RI_REPORT_SIZE(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x70, DataBits, __VA_ARGS__)
#define THID_RI_REPORT_ID(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x80, DataBits, __VA_ARGS__)
#define THID_RI_REPORT_COUNT(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0x90, DataBits, __VA_ARGS__)
#define THID_RI_PUSH(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0xA0, DataBits, __VA_ARGS__)
#define THID_RI_POP(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_GLOBAL, 0xB0, DataBits, __VA_ARGS__)
#define THID_RI_USAGE(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_LOCAL, 0x00, DataBits, __VA_ARGS__)
#define THID_RI_USAGE_MINIMUM(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_LOCAL, 0x10, DataBits, __VA_ARGS__)
#define THID_RI_USAGE_MAXIMUM(DataBits, ...) _THID_RI_ENTRY(THID_RI_TYPE_LOCAL, 0x20, DataBits, __VA_ARGS__)
/**@}*/

/** @} */

/* Macros: */
/** \name Keyboard Standard Report Modifier Masks */
/**@{*/
/** Constant for a keyboard report modifier byte, indicating that the keyboard's left control key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_LEFTCTRL (1 << 0)

/** Constant for a keyboard report modifier byte, indicating that the keyboard's left shift key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_LEFTSHIFT (1 << 1)

/** Constant for a keyboard report modifier byte, indicating that the keyboard's left alt key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_LEFTALT (1 << 2)

/** Constant for a keyboard report modifier byte, indicating that the keyboard's left GUI key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_LEFTGUI (1 << 3)

/** Constant for a keyboard report modifier byte, indicating that the keyboard's right control key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_RIGHTCTRL (1 << 4)

/** Constant for a keyboard report modifier byte, indicating that the keyboard's right shift key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_RIGHTSHIFT (1 << 5)

/** Constant for a keyboard report modifier byte, indicating that the keyboard's right alt key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_RIGHTALT (1 << 6)

/** Constant for a keyboard report modifier byte, indicating that the keyboard's right GUI key is currently pressed. */
#define THID_KEYBOARD_MODIFIER_RIGHTGUI (1 << 7)
/**@}*/

/** \name Keyboard Standard Report LED Masks */
/**@{*/
/** Constant for a keyboard output report LED byte, indicating that the host's NUM LOCK mode is currently set. */
#define THID_KEYBOARD_LED_NUMLOCK (1 << 0)

/** Constant for a keyboard output report LED byte, indicating that the host's CAPS LOCK mode is currently set. */
#define THID_KEYBOARD_LED_CAPSLOCK (1 << 1)

/** Constant for a keyboard output report LED byte, indicating that the host's SCROLL LOCK mode is currently set. */
#define THID_KEYBOARD_LED_SCROLLLOCK (1 << 2)

/** Constant for a keyboard output report LED byte, indicating that the host's COMPOSE mode is currently set. */
#define THID_KEYBOARD_LED_COMPOSE (1 << 3)

/** Constant for a keyboard output report LED byte, indicating that the host's KANA mode is currently set. */
#define THID_KEYBOARD_LED_KANA (1 << 4)
/**@}*/

/** \name Keyboard Standard Report Key Scan-codes */
/**@{*/
#define THID_KEYBOARD_SC_RESERVED 0x00
#define THID_KEYBOARD_SC_ERROR_ROLLOVER 0x01
#define THID_KEYBOARD_SC_POST_FAIL 0x02
#define THID_KEYBOARD_SC_ERROR_UNDEFINED 0x03
#define THID_KEYBOARD_SC_A 0x04
#define THID_KEYBOARD_SC_B 0x05
#define THID_KEYBOARD_SC_C 0x06
#define THID_KEYBOARD_SC_D 0x07
#define THID_KEYBOARD_SC_E 0x08
#define THID_KEYBOARD_SC_F 0x09
#define THID_KEYBOARD_SC_G 0x0A
#define THID_KEYBOARD_SC_H 0x0B
#define THID_KEYBOARD_SC_I 0x0C
#define THID_KEYBOARD_SC_J 0x0D
#define THID_KEYBOARD_SC_K 0x0E
#define THID_KEYBOARD_SC_L 0x0F
#define THID_KEYBOARD_SC_M 0x10
#define THID_KEYBOARD_SC_N 0x11
#define THID_KEYBOARD_SC_O 0x12
#define THID_KEYBOARD_SC_P 0x13
#define THID_KEYBOARD_SC_Q 0x14
#define THID_KEYBOARD_SC_R 0x15
#define THID_KEYBOARD_SC_S 0x16
#define THID_KEYBOARD_SC_T 0x17
#define THID_KEYBOARD_SC_U 0x18
#define THID_KEYBOARD_SC_V 0x19
#define THID_KEYBOARD_SC_W 0x1A
#define THID_KEYBOARD_SC_X 0x1B
#define THID_KEYBOARD_SC_Y 0x1C
#define THID_KEYBOARD_SC_Z 0x1D
#define THID_KEYBOARD_SC_1_AND_EXCLAMATION 0x1E
#define THID_KEYBOARD_SC_2_AND_AT 0x1F
#define THID_KEYBOARD_SC_3_AND_HASHMARK 0x20
#define THID_KEYBOARD_SC_4_AND_DOLLAR 0x21
#define THID_KEYBOARD_SC_5_AND_PERCENTAGE 0x22
#define THID_KEYBOARD_SC_6_AND_CARET 0x23
#define THID_KEYBOARD_SC_7_AND_AMPERSAND 0x24
#define THID_KEYBOARD_SC_8_AND_ASTERISK 0x25
#define THID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS 0x26
#define THID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS 0x27
#define THID_KEYBOARD_SC_ENTER 0x28
#define THID_KEYBOARD_SC_ESCAPE 0x29
#define THID_KEYBOARD_SC_BACKSPACE 0x2A
#define THID_KEYBOARD_SC_TAB 0x2B
#define THID_KEYBOARD_SC_SPACE 0x2C
#define THID_KEYBOARD_SC_MINUS_AND_UNDERSCORE 0x2D
#define THID_KEYBOARD_SC_EQUAL_AND_PLUS 0x2E
#define THID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE 0x2F
#define THID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE 0x30
#define THID_KEYBOARD_SC_BACKSLASH_AND_PIPE 0x31
#define THID_KEYBOARD_SC_NON_US_HASHMARK_AND_TILDE 0x32
#define THID_KEYBOARD_SC_SEMICOLON_AND_COLON 0x33
#define THID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE 0x34
#define THID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE 0x35
#define THID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN 0x36
#define THID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN 0x37
#define THID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK 0x38
#define THID_KEYBOARD_SC_CAPS_LOCK 0x39
#define THID_KEYBOARD_SC_F1 0x3A
#define THID_KEYBOARD_SC_F2 0x3B
#define THID_KEYBOARD_SC_F3 0x3C
#define THID_KEYBOARD_SC_F4 0x3D
#define THID_KEYBOARD_SC_F5 0x3E
#define THID_KEYBOARD_SC_F6 0x3F
#define THID_KEYBOARD_SC_F7 0x40
#define THID_KEYBOARD_SC_F8 0x41
#define THID_KEYBOARD_SC_F9 0x42
#define THID_KEYBOARD_SC_F10 0x43
#define THID_KEYBOARD_SC_F11 0x44
#define THID_KEYBOARD_SC_F12 0x45
#define THID_KEYBOARD_SC_PRINT_SCREEN 0x46
#define THID_KEYBOARD_SC_SCROLL_LOCK 0x47
#define THID_KEYBOARD_SC_PAUSE 0x48
#define THID_KEYBOARD_SC_INSERT 0x49
#define THID_KEYBOARD_SC_HOME 0x4A
#define THID_KEYBOARD_SC_PAGE_UP 0x4B
#define THID_KEYBOARD_SC_DELETE 0x4C
#define THID_KEYBOARD_SC_END 0x4D
#define THID_KEYBOARD_SC_PAGE_DOWN 0x4E
#define THID_KEYBOARD_SC_RIGHT_ARROW 0x4F
#define THID_KEYBOARD_SC_LEFT_ARROW 0x50
#define THID_KEYBOARD_SC_DOWN_ARROW 0x51
#define THID_KEYBOARD_SC_UP_ARROW 0x52
#define THID_KEYBOARD_SC_NUM_LOCK 0x53
#define THID_KEYBOARD_SC_KEYPAD_SLASH 0x54
#define THID_KEYBOARD_SC_KEYPAD_ASTERISK 0x55
#define THID_KEYBOARD_SC_KEYPAD_MINUS 0x56
#define THID_KEYBOARD_SC_KEYPAD_PLUS 0x57
#define THID_KEYBOARD_SC_KEYPAD_ENTER 0x58
#define THID_KEYBOARD_SC_KEYPAD_1_AND_END 0x59
#define THID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW 0x5A
#define THID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN 0x5B
#define THID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW 0x5C
#define THID_KEYBOARD_SC_KEYPAD_5 0x5D
#define THID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW 0x5E
#define THID_KEYBOARD_SC_KEYPAD_7_AND_HOME 0x5F
#define THID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW 0x60
#define THID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP 0x61
#define THID_KEYBOARD_SC_KEYPAD_0_AND_INSERT 0x62
#define THID_KEYBOARD_SC_KEYPAD_DOT_AND_DELETE 0x63
#define THID_KEYBOARD_SC_NON_US_BACKSLASH_AND_PIPE 0x64
#define THID_KEYBOARD_SC_APPLICATION 0x65
#define THID_KEYBOARD_SC_POWER 0x66
#define THID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN 0x67
#define THID_KEYBOARD_SC_F13 0x68
#define THID_KEYBOARD_SC_F14 0x69
#define THID_KEYBOARD_SC_F15 0x6A
#define THID_KEYBOARD_SC_F16 0x6B
#define THID_KEYBOARD_SC_F17 0x6C
#define THID_KEYBOARD_SC_F18 0x6D
#define THID_KEYBOARD_SC_F19 0x6E
#define THID_KEYBOARD_SC_F20 0x6F
#define THID_KEYBOARD_SC_F21 0x70
#define THID_KEYBOARD_SC_F22 0x71
#define THID_KEYBOARD_SC_F23 0x72
#define THID_KEYBOARD_SC_F24 0x73
#define THID_KEYBOARD_SC_EXECUTE 0x74
#define THID_KEYBOARD_SC_HELP 0x75
#define THID_KEYBOARD_SC_MENU 0x76
#define THID_KEYBOARD_SC_SELECT 0x77
#define THID_KEYBOARD_SC_STOP 0x78
#define THID_KEYBOARD_SC_AGAIN 0x79
#define THID_KEYBOARD_SC_UNDO 0x7A
#define THID_KEYBOARD_SC_CUT 0x7B
#define THID_KEYBOARD_SC_COPY 0x7C
#define THID_KEYBOARD_SC_PASTE 0x7D
#define THID_KEYBOARD_SC_FIND 0x7E
#define THID_KEYBOARD_SC_MUTE 0x7F
#define THID_KEYBOARD_SC_VOLUME_UP 0x80
#define THID_KEYBOARD_SC_VOLUME_DOWN 0x81
#define THID_KEYBOARD_SC_LOCKING_CAPS_LOCK 0x82
#define THID_KEYBOARD_SC_LOCKING_NUM_LOCK 0x83
#define THID_KEYBOARD_SC_LOCKING_SCROLL_LOCK 0x84
#define THID_KEYBOARD_SC_KEYPAD_COMMA 0x85
#define THID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN_AS400 0x86
#define THID_KEYBOARD_SC_INTERNATIONAL1 0x87
#define THID_KEYBOARD_SC_INTERNATIONAL2 0x88
#define THID_KEYBOARD_SC_INTERNATIONAL3 0x89
#define THID_KEYBOARD_SC_INTERNATIONAL4 0x8A
#define THID_KEYBOARD_SC_INTERNATIONAL5 0x8B
#define THID_KEYBOARD_SC_INTERNATIONAL6 0x8C
#define THID_KEYBOARD_SC_INTERNATIONAL7 0x8D
#define THID_KEYBOARD_SC_INTERNATIONAL8 0x8E
#define THID_KEYBOARD_SC_INTERNATIONAL9 0x8F
#define THID_KEYBOARD_SC_LANG1 0x90
#define THID_KEYBOARD_SC_LANG2 0x91
#define THID_KEYBOARD_SC_LANG3 0x92
#define THID_KEYBOARD_SC_LANG4 0x93
#define THID_KEYBOARD_SC_LANG5 0x94
#define THID_KEYBOARD_SC_LANG6 0x95
#define THID_KEYBOARD_SC_LANG7 0x96
#define THID_KEYBOARD_SC_LANG8 0x97
#define THID_KEYBOARD_SC_LANG9 0x98
#define THID_KEYBOARD_SC_ALTERNATE_ERASE 0x99
#define THID_KEYBOARD_SC_SYSREQ 0x9A
#define THID_KEYBOARD_SC_CANCEL 0x9B
#define THID_KEYBOARD_SC_CLEAR 0x9C
#define THID_KEYBOARD_SC_PRIOR 0x9D
#define THID_KEYBOARD_SC_RETURN 0x9E
#define THID_KEYBOARD_SC_SEPARATOR 0x9F
#define THID_KEYBOARD_SC_OUT 0xA0
#define THID_KEYBOARD_SC_OPER 0xA1
#define THID_KEYBOARD_SC_CLEAR_AND_AGAIN 0xA2
#define THID_KEYBOARD_SC_CRSEL_AND_PROPS 0xA3
#define THID_KEYBOARD_SC_EXSEL 0xA4
#define THID_KEYBOARD_SC_KEYPAD_00 0xB0
#define THID_KEYBOARD_SC_KEYPAD_000 0xB1
#define THID_KEYBOARD_SC_THOUSANDS_SEPARATOR 0xB2
#define THID_KEYBOARD_SC_DECIMAL_SEPARATOR 0xB3
#define THID_KEYBOARD_SC_CURRENCY_UNIT 0xB4
#define THID_KEYBOARD_SC_CURRENCY_SUB_UNIT 0xB5
#define THID_KEYBOARD_SC_KEYPAD_OPENING_PARENTHESIS 0xB6
#define THID_KEYBOARD_SC_KEYPAD_CLOSING_PARENTHESIS 0xB7
#define THID_KEYBOARD_SC_KEYPAD_OPENING_BRACE 0xB8
#define THID_KEYBOARD_SC_KEYPAD_CLOSING_BRACE 0xB9
#define THID_KEYBOARD_SC_KEYPAD_TAB 0xBA
#define THID_KEYBOARD_SC_KEYPAD_BACKSPACE 0xBB
#define THID_KEYBOARD_SC_KEYPAD_A 0xBC
#define THID_KEYBOARD_SC_KEYPAD_B 0xBD
#define THID_KEYBOARD_SC_KEYPAD_C 0xBE
#define THID_KEYBOARD_SC_KEYPAD_D 0xBF
#define THID_KEYBOARD_SC_KEYPAD_E 0xC0
#define THID_KEYBOARD_SC_KEYPAD_F 0xC1
#define THID_KEYBOARD_SC_KEYPAD_XOR 0xC2
#define THID_KEYBOARD_SC_KEYPAD_CARET 0xC3
#define THID_KEYBOARD_SC_KEYPAD_PERCENTAGE 0xC4
#define THID_KEYBOARD_SC_KEYPAD_LESS_THAN_SIGN 0xC5
#define THID_KEYBOARD_SC_KEYPAD_GREATER_THAN_SIGN 0xC6
#define THID_KEYBOARD_SC_KEYPAD_AMP 0xC7
#define THID_KEYBOARD_SC_KEYPAD_AMP_AMP 0xC8
#define THID_KEYBOARD_SC_KEYPAD_PIPE 0xC9
#define THID_KEYBOARD_SC_KEYPAD_PIPE_PIPE 0xCA
#define THID_KEYBOARD_SC_KEYPAD_COLON 0xCB
#define THID_KEYBOARD_SC_KEYPAD_HASHMARK 0xCC
#define THID_KEYBOARD_SC_KEYPAD_SPACE 0xCD
#define THID_KEYBOARD_SC_KEYPAD_AT 0xCE
#define THID_KEYBOARD_SC_KEYPAD_EXCLAMATION_SIGN 0xCF
#define THID_KEYBOARD_SC_KEYPAD_MEMORY_STORE 0xD0
#define THID_KEYBOARD_SC_KEYPAD_MEMORY_RECALL 0xD1
#define THID_KEYBOARD_SC_KEYPAD_MEMORY_CLEAR 0xD2
#define THID_KEYBOARD_SC_KEYPAD_MEMORY_ADD 0xD3
#define THID_KEYBOARD_SC_KEYPAD_MEMORY_SUBTRACT 0xD4
#define THID_KEYBOARD_SC_KEYPAD_MEMORY_MULTIPLY 0xD5
#define THID_KEYBOARD_SC_KEYPAD_MEMORY_DIVIDE 0xD6
#define THID_KEYBOARD_SC_KEYPAD_PLUS_AND_MINUS 0xD7
#define THID_KEYBOARD_SC_KEYPAD_CLEAR 0xD8
#define THID_KEYBOARD_SC_KEYPAD_CLEAR_ENTRY 0xD9
#define THID_KEYBOARD_SC_KEYPAD_BINARY 0xDA
#define THID_KEYBOARD_SC_KEYPAD_OCTAL 0xDB
#define THID_KEYBOARD_SC_KEYPAD_DECIMAL 0xDC
#define THID_KEYBOARD_SC_KEYPAD_HEXADECIMAL 0xDD
#define THID_KEYBOARD_SC_LEFT_CONTROL 0xE0
#define THID_KEYBOARD_SC_LEFT_SHIFT 0xE1
#define THID_KEYBOARD_SC_LEFT_ALT 0xE2
#define THID_KEYBOARD_SC_LEFT_GUI 0xE3
#define THID_KEYBOARD_SC_RIGHT_CONTROL 0xE4
#define THID_KEYBOARD_SC_RIGHT_SHIFT 0xE5
#define THID_KEYBOARD_SC_RIGHT_ALT 0xE6
#define THID_KEYBOARD_SC_RIGHT_GUI 0xE7
#define THID_KEYBOARD_SC_MEDIA_PLAY 0xE8
#define THID_KEYBOARD_SC_MEDIA_STOP 0xE9
#define THID_KEYBOARD_SC_MEDIA_PREVIOUS_TRACK 0xEA
#define THID_KEYBOARD_SC_MEDIA_NEXT_TRACK 0xEB
#define THID_KEYBOARD_SC_MEDIA_EJECT 0xEC
#define THID_KEYBOARD_SC_MEDIA_VOLUME_UP 0xED
#define THID_KEYBOARD_SC_MEDIA_VOLUME_DOWN 0xEE
#define THID_KEYBOARD_SC_MEDIA_MUTE 0xEF
#define THID_KEYBOARD_SC_MEDIA_WWW 0xF0
#define THID_KEYBOARD_SC_MEDIA_BACKWARD 0xF1
#define THID_KEYBOARD_SC_MEDIA_FORWARD 0xF2
#define THID_KEYBOARD_SC_MEDIA_CANCEL 0xF3
#define THID_KEYBOARD_SC_MEDIA_SEARCH 0xF4
#define THID_KEYBOARD_SC_MEDIA_SLEEP 0xF8
#define THID_KEYBOARD_SC_MEDIA_LOCK 0xF9
#define THID_KEYBOARD_SC_MEDIA_RELOAD 0xFA
#define THID_KEYBOARD_SC_MEDIA_CALCULATOR 0xFB
/**@}*/

/** \name Common HID Device Report Descriptors */
/**@{*/
/** \hideinitializer
		 *  A list of HID report item array elements that describe a typical HID USB Joystick. The resulting report
		 *  descriptor is structured according to the following layout:
		 *
		 *  \code
		 *  struct
		 *  {
		 *      intA_t X; // Signed X axis value
		 *      intA_t Y; // Signed Y axis value
		 *      intA_t Z; // Signed Z axis value
		 *      uintB_t Buttons; // Pressed buttons bitmask
		 *  } Joystick_Report;
		 *  \endcode
		 *
		 *  Where \c uintA_t is a type large enough to hold the ranges of the signed \c MinAxisVal and \c MaxAxisVal values,
		 *  and \c intB_t is a type large enough to hold one bit per button.
		 *
		 *  \param[in] MinAxisVal      Minimum logical axis value (16-bit).
		 *  \param[in] MaxAxisVal      Maximum logical axis value (16-bit).
		 *  \param[in] MinPhysicalVal  Minimum physical axis value, for movement resolution calculations (16-bit).
		 *  \param[in] MaxPhysicalVal  Maximum physical axis value, for movement resolution calculations (16-bit).
		 *  \param[in] Buttons         Total number of buttons in the device (8-bit).
		 */
#define THID_DESCRIPTOR_JOYSTICK(MinAxisVal, MaxAxisVal, MinPhysicalVal, MaxPhysicalVal, Buttons) \
    THID_RI_USAGE_PAGE(8, 0x01),                                                                  \
        THID_RI_USAGE(8, 0x04),                                                                   \
        THID_RI_COLLECTION(8, 0x01),                                                              \
        THID_RI_USAGE(8, 0x01),                                                                   \
        THID_RI_COLLECTION(8, 0x00),                                                              \
        THID_RI_USAGE(8, 0x30),                                                                   \
        THID_RI_USAGE(8, 0x31),                                                                   \
        THID_RI_USAGE(8, 0x32),                                                                   \
        THID_RI_LOGICAL_MINIMUM(16, MinAxisVal),                                                  \
        THID_RI_LOGICAL_MAXIMUM(16, MaxAxisVal),                                                  \
        THID_RI_PHYSICAL_MINIMUM(16, MinPhysicalVal),                                             \
        THID_RI_PHYSICAL_MAXIMUM(16, MaxPhysicalVal),                                             \
        THID_RI_REPORT_COUNT(8, 3),                                                               \
        THID_RI_REPORT_SIZE(8, (((MinAxisVal >= -128) && (MaxAxisVal <= 127)) ? 8 : 16)),         \
        THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),                  \
        THID_RI_END_COLLECTION(0),                                                                \
        THID_RI_USAGE_PAGE(8, 0x09),                                                              \
        THID_RI_USAGE_MINIMUM(8, 0x01),                                                           \
        THID_RI_USAGE_MAXIMUM(8, Buttons),                                                        \
        THID_RI_LOGICAL_MINIMUM(8, 0x00),                                                         \
        THID_RI_LOGICAL_MAXIMUM(8, 0x01),                                                         \
        THID_RI_REPORT_SIZE(8, 0x01),                                                             \
        THID_RI_REPORT_COUNT(8, Buttons),                                                         \
        THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),                  \
        THID_RI_REPORT_SIZE(8, (Buttons % 8) ? (8 - (Buttons % 8)) : 0),                          \
        THID_RI_REPORT_COUNT(8, 0x01),                                                            \
        THID_RI_INPUT(8, THID_IOF_CONSTANT),                                                      \
        THID_RI_END_COLLECTION(0)

/** \hideinitializer
		 *  A list of HID report item array elements that describe a typical HID USB keyboard. The resulting report descriptor
		 *  is compatible with \ref TUSB_KeyboardReport_Data_t when \c MaxKeys is equal to 6. For other values, the report will
		 *  be structured according to the following layout:
		 *
		 *  \code
		 *  struct
		 *  {
		 *      uint8_t Modifier; // Keyboard modifier byte indicating pressed modifier keys (\c THID_KEYBOARD_MODIFER_* masks)
		 *      uint8_t Reserved; // Reserved for OEM use, always set to 0.
		 *      uint8_t KeyCode[MaxKeys]; // Length determined by the number of keys that can be reported
		 *  } Keyboard_Report;
		 *  \endcode
		 *
		 *  \param[in] MaxKeys  Number of simultaneous keys that can be reported at the one time (8-bit).
		 */
#define THID_DESCRIPTOR_KEYBOARD(MaxKeys)                                                                 \
    THID_RI_USAGE_PAGE(8, 0x01),                                                                          \
        THID_RI_USAGE(8, 0x06),                                                                           \
        THID_RI_COLLECTION(8, 0x01),                                                                      \
        THID_RI_USAGE_PAGE(8, 0x07),                                                                      \
        THID_RI_USAGE_MINIMUM(8, 0xE0),                                                                   \
        THID_RI_USAGE_MAXIMUM(8, 0xE7),                                                                   \
        THID_RI_LOGICAL_MINIMUM(8, 0x00),                                                                 \
        THID_RI_LOGICAL_MAXIMUM(8, 0x01),                                                                 \
        THID_RI_REPORT_SIZE(8, 0x01),                                                                     \
        THID_RI_REPORT_COUNT(8, 0x08),                                                                    \
        THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),                          \
        THID_RI_REPORT_COUNT(8, 0x01),                                                                    \
        THID_RI_REPORT_SIZE(8, 0x08),                                                                     \
        THID_RI_INPUT(8, THID_IOF_CONSTANT),                                                              \
        THID_RI_USAGE_PAGE(8, 0x08),                                                                      \
        THID_RI_USAGE_MINIMUM(8, 0x01),                                                                   \
        THID_RI_USAGE_MAXIMUM(8, 0x05),                                                                   \
        THID_RI_REPORT_COUNT(8, 0x05),                                                                    \
        THID_RI_REPORT_SIZE(8, 0x01),                                                                     \
        THID_RI_OUTPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE | THID_IOF_NON_VOLATILE), \
        THID_RI_REPORT_COUNT(8, 0x01),                                                                    \
        THID_RI_REPORT_SIZE(8, 0x03),                                                                     \
        THID_RI_OUTPUT(8, THID_IOF_CONSTANT),                                                             \
        THID_RI_LOGICAL_MINIMUM(8, 0x00),                                                                 \
        THID_RI_LOGICAL_MAXIMUM(16, 0xFF),                                                                \
        THID_RI_USAGE_PAGE(8, 0x07),                                                                      \
        THID_RI_USAGE_MINIMUM(8, 0x00),                                                                   \
        THID_RI_USAGE_MAXIMUM(8, 0xFF),                                                                   \
        THID_RI_REPORT_COUNT(8, MaxKeys),                                                                 \
        THID_RI_REPORT_SIZE(8, 0x08),                                                                     \
        THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_ARRAY | THID_IOF_ABSOLUTE),                             \
        THID_RI_END_COLLECTION(0)

/** \hideinitializer
		 *  A list of HID report item array elements that describe a typical HID USB mouse. The resulting report descriptor
		 *  is compatible with \ref TUSB_MouseReport_Data_t if the \c MinAxisVal and \c MaxAxisVal values fit within a \c int8_t range
		 *  and the number of Buttons is less than 8. For other values, the report is structured according to the following layout:
		 *
		 *  \code
		 *  struct
		 *  {
		 *      uintA_t Buttons; // Pressed buttons bitmask
		 *      intB_t X; // X axis value
		 *      intB_t Y; // Y axis value
		 *  } Mouse_Report;
		 *  \endcode
		 *
		 *  Where \c intA_t is a type large enough to hold one bit per button, and \c intB_t is a type large enough to hold the
		 *  ranges of the signed \c MinAxisVal and \c MaxAxisVal values.
		 *
		 *  \param[in] MinAxisVal      Minimum X/Y logical axis value (16-bit).
		 *  \param[in] MaxAxisVal      Maximum X/Y logical axis value (16-bit).
		 *  \param[in] MinPhysicalVal  Minimum X/Y physical axis value, for movement resolution calculations (16-bit).
		 *  \param[in] MaxPhysicalVal  Maximum X/Y physical axis value, for movement resolution calculations (16-bit).
		 *  \param[in] Buttons         Total number of buttons in the device (8-bit).
		 *  \param[in] AbsoluteCoords  Boolean \c true to use absolute X/Y coordinates (e.g. touchscreen).
		 */
#define THID_DESCRIPTOR_MOUSE(MinAxisVal, MaxAxisVal, MinPhysicalVal, MaxPhysicalVal, Buttons, AbsoluteCoords)          \
    THID_RI_USAGE_PAGE(8, 0x01),                                                                                        \
        THID_RI_USAGE(8, 0x02),                                                                                         \
        THID_RI_COLLECTION(8, 0x01),                                                                                    \
        THID_RI_USAGE(8, 0x01),                                                                                         \
        THID_RI_COLLECTION(8, 0x00),                                                                                    \
        THID_RI_USAGE_PAGE(8, 0x09),                                                                                    \
        THID_RI_USAGE_MINIMUM(8, 0x01),                                                                                 \
        THID_RI_USAGE_MAXIMUM(8, Buttons),                                                                              \
        THID_RI_LOGICAL_MINIMUM(8, 0x00),                                                                               \
        THID_RI_LOGICAL_MAXIMUM(8, 0x01),                                                                               \
        THID_RI_REPORT_COUNT(8, Buttons),                                                                               \
        THID_RI_REPORT_SIZE(8, 0x01),                                                                                   \
        THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),                                        \
        THID_RI_REPORT_COUNT(8, 0x01),                                                                                  \
        THID_RI_REPORT_SIZE(8, (Buttons % 8) ? (8 - (Buttons % 8)) : 0),                                                \
        THID_RI_INPUT(8, THID_IOF_CONSTANT),                                                                            \
        THID_RI_USAGE_PAGE(8, 0x01),                                                                                    \
        THID_RI_USAGE(8, 0x30),                                                                                         \
        THID_RI_USAGE(8, 0x31),                                                                                         \
        THID_RI_LOGICAL_MINIMUM(16, MinAxisVal),                                                                        \
        THID_RI_LOGICAL_MAXIMUM(16, MaxAxisVal),                                                                        \
        THID_RI_PHYSICAL_MINIMUM(16, MinPhysicalVal),                                                                   \
        THID_RI_PHYSICAL_MAXIMUM(16, MaxPhysicalVal),                                                                   \
        THID_RI_REPORT_COUNT(8, 0x02),                                                                                  \
        THID_RI_REPORT_SIZE(8, (((MinAxisVal >= -128) && (MaxAxisVal <= 127)) ? 8 : 16)),                               \
        THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | (AbsoluteCoords ? THID_IOF_ABSOLUTE : THID_IOF_RELATIVE)), \
        THID_RI_END_COLLECTION(0),                                                                                      \
        THID_RI_END_COLLECTION(0)

/** \hideinitializer
		 *  A list of HID report item array elements that describe a typical Vendor Defined byte array HID report descriptor,
		 *  used for transporting arbitrary data between the USB host and device via HID reports. The resulting report should be
		 *  a \c uint8_t byte array of the specified length in both Device to Host (IN) and Host to Device (OUT) directions.
		 *
		 *  \param[in] VendorPageNum    Vendor Defined HID Usage Page index, ranging from 0x00 to 0xFF.
		 *  \param[in] CollectionUsage  Vendor Usage for the encompassing report IN and OUT collection, ranging from 0x00 to 0xFF.
		 *  \param[in] DataINUsage      Vendor Usage for the IN report data, ranging from 0x00 to 0xFF.
		 *  \param[in] DataOUTUsage     Vendor Usage for the OUT report data, ranging from 0x00 to 0xFF.
		 *  \param[in] NumBytes         Length of the data IN and OUT reports.
		 */
#define THID_DESCRIPTOR_VENDOR(VendorPageNum, CollectionUsage, DataINUsage, DataOUTUsage, NumBytes)       \
    THID_RI_USAGE_PAGE(16, (0xFF00 | VendorPageNum)),                                                     \
        THID_RI_USAGE(8, CollectionUsage),                                                                \
        THID_RI_COLLECTION(8, 0x01),                                                                      \
        THID_RI_USAGE(8, DataINUsage),                                                                    \
        THID_RI_LOGICAL_MINIMUM(8, 0x00),                                                                 \
        THID_RI_LOGICAL_MAXIMUM(8, 0xFF),                                                                 \
        THID_RI_REPORT_SIZE(8, 0x08),                                                                     \
        THID_RI_REPORT_COUNT(8, NumBytes),                                                                \
        THID_RI_INPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE),                          \
        THID_RI_USAGE(8, DataOUTUsage),                                                                   \
        THID_RI_LOGICAL_MINIMUM(8, 0x00),                                                                 \
        THID_RI_LOGICAL_MAXIMUM(8, 0xFF),                                                                 \
        THID_RI_REPORT_SIZE(8, 0x08),                                                                     \
        THID_RI_REPORT_COUNT(8, NumBytes),                                                                \
        THID_RI_OUTPUT(8, THID_IOF_DATA | THID_IOF_VARIABLE | THID_IOF_ABSOLUTE | THID_IOF_NON_VOLATILE), \
        THID_RI_END_COLLECTION(0)
/**@}*/

/* Type Defines: */
/** Enum for possible Class, Subclass and Protocol values of device and interface descriptors relating to the HID
		 *  device class.
		 */
enum THID_Descriptor_ClassSubclassProtocol_t {
    THID_CSCP_HIDClass = 0x03,             /**< Descriptor Class value indicating that the device or interface
			                                       *   belongs to the HID class.
			                                       */
    THID_CSCP_NonBootSubclass = 0x00,      /**< Descriptor Subclass value indicating that the device or interface
			                                       *   does not implement a HID boot protocol.
			                                       */
    THID_CSCP_BootSubclass = 0x01,         /**< Descriptor Subclass value indicating that the device or interface
			                                       *   implements a HID boot protocol.
			                                       */
    THID_CSCP_NonBootProtocol = 0x00,      /**< Descriptor Protocol value indicating that the device or interface
			                                       *   does not belong to a HID boot protocol.
			                                       */
    THID_CSCP_KeyboardBootProtocol = 0x01, /**< Descriptor Protocol value indicating that the device or interface
			                                       *   belongs to the Keyboard HID boot protocol.
			                                       */
    THID_CSCP_MouseBootProtocol = 0x02,    /**< Descriptor Protocol value indicating that the device or interface
			                                       *   belongs to the Mouse HID boot protocol.
			                                       */
};

/** Enum for the HID class specific control requests that can be issued by the USB bus host. */
enum THID_ClassRequests_t {
    THID_REQ_GetReport = 0x01,   /**< HID class-specific Request to get the current HID report from the device. */
    THID_REQ_GetIdle = 0x02,     /**< HID class-specific Request to get the current device idle count. */
    THID_REQ_GetProtocol = 0x03, /**< HID class-specific Request to get the current HID report protocol mode. */
    THID_REQ_SetReport = 0x09,   /**< HID class-specific Request to set the current HID report to the device. */
    THID_REQ_SetIdle = 0x0A,     /**< HID class-specific Request to set the device's idle count. */
    THID_REQ_SetProtocol = 0x0B, /**< HID class-specific Request to set the current HID report protocol mode. */
};

/** Enum for the HID class specific descriptor types. */
enum THID_DescriptorTypes_t {
    THID_DTYPE_HID = 0x21,    /**< Descriptor header type value, to indicate a HID class HID descriptor. */
    THID_DTYPE_Report = 0x22, /**< Descriptor header type value, to indicate a HID class HID report descriptor. */
};

/** Enum for the different types of HID reports. */
enum THID_ReportItemTypes_t {
    THID_REPORT_ITEM_In = 0,      /**< Indicates that the item is an IN report type. */
    THID_REPORT_ITEM_Out = 1,     /**< Indicates that the item is an OUT report type. */
    THID_REPORT_ITEM_Feature = 2, /**< Indicates that the item is a FEATURE report type. */
};

/** \brief HID class-specific HID Descriptor (LUFA naming conventions).
		 *
		 *  Type define for the HID class-specific HID descriptor, to describe the HID device's specifications. Refer to the HID
		 *  specification for details on the structure elements.
		 *
		 *  \see \ref TUSB_THID_StdDescriptor_THID_t for the version of this type with standard element names.
		 *
		 *  \note Regardless of CPU architecture, these values should be stored as little endian.
		 */
typedef struct
{
    TUSB_Descriptor_Header_t Header; /**< Regular descriptor header containing the descriptor's type and length. */

    uint16_t HIDSpec;    /**< BCD encoded version that the HID descriptor and device complies to.
			                                  *
			                                  *   \see \ref VERSION_BCD() utility macro.
			                                  */
    uint8_t CountryCode; /**< Country code of the localized device, or zero if universal. */

    uint8_t TotalReportDescriptors; /**< Total number of HID report descriptors for the interface. */

    uint8_t HIDReportType;    /**< Type of HID report, set to \ref THID_DTYPE_Report. */
    uint16_t HIDReportLength; /**< Length of the associated HID report descriptor, in bytes. */
} TATTR_PACKED TUSB_THID_Descriptor_THID_t;

/** \brief HID class-specific HID Descriptor (USB-IF naming conventions).
		 *
		 *  Type define for the HID class-specific HID descriptor, to describe the HID device's specifications. Refer to the HID
		 *  specification for details on the structure elements.
		 *
		 *  \see \ref TUSB_THID_Descriptor_THID_t for the version of this type with non-standard LUFA specific
		 *       element names.
		 *
		 *  \note Regardless of CPU architecture, these values should be stored as little endian.
		 */
typedef struct
{
    uint8_t bLength;         /**< Size of the descriptor, in bytes. */
    uint8_t bDescriptorType; /**< Type of the descriptor, either a value in \ref TUSB_DescriptorTypes_t or a value
			                           *   given by the specific class.
			                           */

    uint16_t bcdHID;      /**< BCD encoded version that the HID descriptor and device complies to.
			                  *
			                  *   \see \ref VERSION_BCD() utility macro.
			                  */
    uint8_t bCountryCode; /**< Country code of the localized device, or zero if universal. */

    uint8_t bNumDescriptors; /**< Total number of HID report descriptors for the interface. */

    uint8_t bDescriptorType2;   /**< Type of HID report, set to \ref THID_DTYPE_Report. */
    uint16_t wDescriptorLength; /**< Length of the associated HID report descriptor, in bytes. */
} TATTR_PACKED TUSB_THID_StdDescriptor_THID_t;

/** \brief Standard HID Boot Protocol Keyboard Report.
		 *
		 *  Type define for a standard Boot Protocol Keyboard report
		 */
typedef struct
{
    uint8_t rid;
    uint8_t Modifier; /**< Keyboard modifier byte, indicating pressed modifier keys (a combination of
			                   *   \c THID_KEYBOARD_MODIFER_* masks).
			                   */
                            //   Indicates pressed keys (a combination of THID_KEYBOARD_SC_ masks)
    uint8_t KeyCodeFlags[(THID_KEYBOARD_SC_KEYPAD_EQUAL_SIGN + 1) / 8];  //Bitmask of all pressed keys
} TATTR_PACKED TUSB_KeyboardReport_Data_t;
typedef struct
{
    uint8_t rid;
    uint8_t Button; /**< Button mask for currently pressed buttons in the mouse. */
    int8_t X;       /**< Current delta X movement of the mouse. */
    int8_t Y;       /**< Current delta Y movement on the mouse. */
    int8_t ScrollX;
    int8_t ScrollY;

} TATTR_PACKED TUSB_MouseReport_Data_t;

/** Type define for the data type used to store HID report descriptor elements. */
typedef uint8_t TUSB_Descriptor_HIDReport_Datatype_t;
