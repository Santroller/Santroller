#pragma once
enum endpoint_dir_t {
    ENDPOINT_OUT = 0x00,
    ENDPOINT_IN = 0x80
};

// By using these endpoints, we can double buffer!
enum endpoints_t {
    CDC_NOTIFICATION = ENDPOINT_IN | 1,
    DEVICE_EPADDR_IN = ENDPOINT_IN | 3,
    DEVICE_EPADDR_OUT = ENDPOINT_OUT | 4,
    XINPUT_MIC_IN = ENDPOINT_IN | 5,
    XINPUT_AUDIO_OUT = ENDPOINT_OUT | 6,
    XINPUT_UNK_IN = ENDPOINT_IN | 7,
    XINPUT_UNK_OUT = ENDPOINT_OUT | 8,
    XINPUT_PLUGIN_MODULE_IN = ENDPOINT_IN | 9
};

#define SERIAL_TX_SIZE 32
#define SERIAL_RX_SIZE 32
#define SERIAL_NOTIFICATION_SIZE 8
#define ENDPOINT_SIZE 64
#define VENDOR_EPSIZE 64

#define ARDWIINO_VID 0x1209
#define ARDWIINO_PID 0x2882
#define ARDWIINO_PID_BLE 0x2885
#define HORI_VID 0x0f0d
#define HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID 0x0092
#define NINTENDO_VID 0x057E
#define SWITCH_PID 0x2009
#define REDOCTANE_VID 0x12ba
#define PS4_VID 0x03EB
#define PS4_PID 0x2043
#define SONY_VID 0x054c
#define SONY_DS3_PID 0x0268
#define PS4_DS_PID_1 0x05c4
#define PS4_DS_PID_2 0x09cc
#define PS4_DS_PID_3 0x0ba0
#define PS4_STRAT_VID 0x0738
#define PS4_STRAT_PID 0x8261
#define PS4_JAG_VID 0x0E6F
#define PS4_JAG_PID 0x0173
#define PS4_RIFFMASTER_VID 0x0E6F
#define PS4_RIFFMASTER_PID 0x024A
#define PS5_DS_PID 0x0ce6
#define PS5_RIFFMASTER_VID 0x0E6F
#define PS5_RIFFMASTER_PID 0x0249
#define PS3_GH_GUITAR_PID 0x0100
#define PS3_GH_DRUM_PID 0x0120
#define PS3_RB_GUITAR_PID 0x0200
#define PS3_RB_DRUM_PID 0x0210
#define PS3_DJ_TURNTABLE_PID 0x0140
#define PS3_MUSTANG_PID 0x2430
#define PS3_SQUIRE_PID 0x2530
#define PS3_DRUMS_PID 0x0210
#define PS3_KEYBOARD_PID 0x2330
#define PS3WIIU_GHLIVE_DONGLE_PID 0x074b
#define PS4_GHLIVE_DONGLE_PID 0x07BB
#define WII_RB_VID 0x1bad
#define WII_RB_GUITAR_PID 0x0004
#define WII_RB_GUITAR_2_PID 0x3010
#define WII_RB_DRUM_PID 0x0005
#define WII_RB_DRUM_2_PID 0x3110
#define WII_MUSTANG_PID 0x3430
#define WII_SQUIRE_PID 0x3530
#define WII_DRUMS_PID 0x3130
#define WII_KEYBOARD_PID 0x3330
#define MAD_CATZ_VID 0x0738
#define XBOX_ONE_RB_DRUM_PID 0x4262
#define XBOX_ONE_RB_GUITAR_PID 0x4161
#define GHLIVE_DONGLE_VID 0x1430
#define XBOX_ONE_GHLIVE_DONGLE_VID 0x1430
#define XBOX_ONE_GHLIVE_DONGLE_PID 0x079B
#define XBOX_360_GHLIVE_DONGLE_VID 0x1430
#define XBOX_360_GHLIVE_DONGLE_PID 0x070B
#define XBOX_360_WT_KIOSK_VID 0x1430
#define XBOX_360_WT_KIOSK_PID 0x4734
#define XBOX_ONE_CONTROLLER_VID 0x045e
#define XBOX_ONE_CONTROLLER_PID 0x02ea
#define NINDENDO_VID 0x057E
#define PRO_CONTROLLER_PID 0x2009
#define GUITAR_PRAISE_VID 0x0314
#define GUITAR_PRAISE_PID 0x0830
#define RAPHNET_VID 0x289b
#define XBOX_ONE_JAG_VID 0x0e6f
#define XBOX_ONE_JAG_PID 0x0170
#define MAGICBOOTS_PS4_VID 0x0079
#define MAGICBOOTS_PS4_PID 0x1893

#define STREAM_DECK_INPUT_REPORT_ID 1
#define STREAM_DECK_VID 0x0fd9
#define STREAM_DECK_OG_PID 0x0060
#define STREAM_DECK_MINI_PID 0x0063
#define STREAM_DECK_XL_PID 0x006c
#define STREAM_DECK_V2_PID 0x006d
#define STREAM_DECK_MK2_PID 0x0080
#define STREAM_DECK_PLUS_PID 0x0084
#define STREAM_DECK_PEDAL_PID 0x0086
#define STREAM_DECK_XLV2_PID 0x008f
#define STREAM_DECK_MINIV2_PID 0x0090
#define STREAM_DECK_NEO_PID 0x009a

#define SIMULTANEOUS_KEYS 6

#define Buttons 4
#define MinAxisVal -127
#define MaxAxisVal 127
#define MinPhysicalVal -127
#define MaxPhysicalVal 128
#define AbsoluteCoords false

enum descriptors_t {
    DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR = 0x0004,
    DESC_EXTENDED_PROPERTIES_DESCRIPTOR = 0x0005,
};

enum requests_t {
    REQ_GET_OS_FEATURE_DESCRIPTOR = 0x20
};


enum interfaces_t {
    INTERFACE_ID_Device = 0, /**< XInput interface descriptor ID */
    INTERFACE_ID_Padding = 1,
    INTERFACE_ID_Config = 2, /**< Config interface descriptor ID */
    INTERFACE_ID_XBOX_Security = 3,
    INTERFACE_ID_Xone_Device = 4,
    INTERFACE_ID_ControlStream =
        0, /**< MIDI Control Stream interface descriptor ID */
    INTERFACE_ID_AudioStream =
        2, /**< MIDI Audio Stream interface descriptor ID */
};


#ifdef __cplusplus
extern "C" {
#endif
void reboot(void);
void bootloader(void);
#ifdef __cplusplus
}
#endif