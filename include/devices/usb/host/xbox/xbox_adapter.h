#ifndef XBOX_ADAPTER_H
#define XBOX_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "gip_device.h"

#ifdef __cplusplus
}
#endif

#define XBOX_MAX_CONTROLLERS 8

struct mt76_dev;

// GIP System Commands (from Microsoft spec / SDL)
#define GIP_CMD_PROTO_CONTROL   0x01  // Protocol control
#define GIP_CMD_HELLO_DEVICE    0x02  // Device announcement
#define GIP_CMD_STATUS_DEVICE   0x03  // Device status
#define GIP_CMD_METADATA        0x04  // Device metadata
#define GIP_CMD_SET_DEVICE_STATE 0x05 // Power/state control
#define GIP_CMD_SECURITY        0x06  // Authentication
#define GIP_CMD_GUIDE_BUTTON    0x07  // Guide button
#define GIP_CMD_AUDIO_CONTROL   0x08  // Audio configuration
#define GIP_CMD_DIRECT_MOTOR    0x09  // Rumble/vibration
#define GIP_CMD_LED             0x0A  // LED control
#define GIP_CMD_HID_REPORT      0x0B  // HID report
#define GIP_CMD_FIRMWARE        0x0C  // Firmware update
#define GIP_CMD_EXTENDED        0x1E  // Extended commands
#define GIP_CMD_DEBUG           0x1F  // Debug messages

// GIP Vendor Commands (Navigation/Gamepad)
#define GIP_LL_INPUT_REPORT     0x20  // Low-latency input

// GIP Audio
#define GIP_AUDIO_DATA          0x60  // Audio samples

// GIP Header Flags
#define GIP_FLAG_NEED_ACK       0x10  // Sender requests ACK
#define GIP_FLAG_SYSTEM         0x20  // System/internal packet
#define GIP_FLAG_CHUNK_START    0x40  // First chunk
#define GIP_FLAG_CHUNK          0x80  // Continuation chunk

// Legacy aliases for compatibility
#define GIP_CMD_ANNOUNCE        GIP_CMD_HELLO_DEVICE
#define GIP_CMD_STATUS          GIP_CMD_STATUS_DEVICE
#define GIP_CMD_IDENTIFY        GIP_CMD_METADATA
#define GIP_CMD_POWER           GIP_CMD_SET_DEVICE_STATE
#define GIP_CMD_AUTHENTICATE    GIP_CMD_SECURITY
#define GIP_CMD_VIRTUAL_KEY     GIP_CMD_GUIDE_BUTTON
#define GIP_CMD_INPUT           GIP_LL_INPUT_REPORT

#define GIP_OPT_ACKNOWLEDGE     GIP_FLAG_NEED_ACK
#define GIP_OPT_INTERNAL        GIP_FLAG_SYSTEM

typedef enum {
    XBOX_CONTROLLER_DISCONNECTED,
    XBOX_CONTROLLER_CONNECTED,
    XBOX_CONTROLLER_READY
} xbox_controller_status_t;

typedef enum {
    XBOX_DEVICE_UNKNOWN = -1,
    XBOX_DEVICE_GAMEPAD = 0,
    XBOX_DEVICE_ARCADE_STICK = 1,
    XBOX_DEVICE_WHEEL = 2,
    XBOX_DEVICE_FLIGHT_STICK = 3,
    XBOX_DEVICE_NAVIGATION = 4,
    XBOX_DEVICE_CHATPAD = 5,
    XBOX_DEVICE_HEADSET = 6,
    XBOX_DEVICE_GUITAR = 7,        // Rock Band / Guitar Hero guitar
    XBOX_DEVICE_DRUMS = 8,         // Rock Band drums
} xbox_device_type_t;

typedef struct {
    uint8_t dev_addr;
    uint8_t instance;
    xbox_controller_status_t status;
    gip_device_t gip_device;  // Shared GIP device state
    uint8_t mac_addr[6];   // Controller MAC address
    xbox_device_type_t device_type;  // Controller type (gamepad, guitar, drums, etc.)
} xbox_controller_t;

#ifdef __cplusplus
extern "C" {
#endif

bool xbox_adapter_mounted(void);
xbox_controller_t* xbox_get_controller(struct mt76_dev *dev, uint8_t index);
void xbox_create_controller(struct mt76_dev *dev, uint8_t index);
void xbox_remove_controller(struct mt76_dev *dev, uint8_t index);
void xbox_adapter_mount(uint8_t dev_addr, uint8_t instance);
void xbox_adapter_unmount(uint8_t dev_addr);
void xbox_adapter_process_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
void xbox_adapter_process_gip_data(struct mt76_dev *dev, uint8_t wcid, const uint8_t *data, uint16_t len);
int xbox_adapter_send_gip_packet(uint8_t controller_idx, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
