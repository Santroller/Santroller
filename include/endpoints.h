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

#define HID_REPORT_TYPE_INPUT 0x01
#define HID_REPORT_TYPE_OUTPUT 0x02
#define HID_REPORT_TYPE_FEATURE 0x03



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