#ifndef MT76_H
#define MT76_H

#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"
#include "mt76_defs.h"

#define MT76_MAC_ADDR_LEN 6
#define MT76_NUM_CHANNELS 12
#define MT76_MAX_CLIENTS 8
#define MT76_EVENT_QUEUE_SIZE 16

enum mt76_wireless_event_type {
    MT76_EVENT_ASSOCIATION,
    MT76_EVENT_DISASSOCIATION,
    MT76_EVENT_CLIENT_LOST,
    MT76_EVENT_CLIENT_COMMAND,
};

struct mt76_client {
    bool used;
    uint8_t addr[MT76_MAC_ADDR_LEN];
    uint8_t key[16];
    bool encryption_enabled;
};

struct mt76_wireless_event {
    enum mt76_wireless_event_type type;
    uint8_t wcid;
    uint8_t command;
    uint8_t addr[MT76_MAC_ADDR_LEN];
};

struct mt76_channel {
    uint8_t index;
    uint8_t band;
    uint8_t bandwidth;
    uint8_t band_5g;
    bool primary;
    int8_t power;
    bool scan;
};

struct mt76_dev {
    uint8_t dev_addr;
    uint8_t mac_address[MT76_MAC_ADDR_LEN];
    void *owner;
    uint32_t control_data;
    bool initialized;
    struct mt76_channel channels[MT76_NUM_CHANNELS];
    struct mt76_channel *current_channel;
    CFG_TUSB_MEM_ALIGN uint8_t bulk_buffer[MT_FW_CHUNK_SIZE + MT_CMD_HDR_LEN * 2];
    CFG_TUSB_MEM_ALIGN uint8_t tx_buffer[512];
    struct mt76_client clients[MT76_MAX_CLIENTS];
    struct mt76_wireless_event event_queue[MT76_EVENT_QUEUE_SIZE];
    volatile uint8_t event_queue_head;
    volatile uint8_t event_queue_tail;
    uint32_t pairing_start_time;
    bool pairing_active;
    volatile int8_t pending_pairing;
};

bool mt76_init(struct mt76_dev *dev, uint8_t dev_addr);
void mt76_deinit(struct mt76_dev *dev);

uint32_t mt76_read_register(struct mt76_dev *dev, uint32_t addr);
void mt76_write_register(struct mt76_dev *dev, uint32_t addr, uint32_t val);
bool mt76_poll(struct mt76_dev *dev, uint32_t offset, uint32_t mask, uint32_t val);

int mt76_load_firmware(struct mt76_dev *dev, const uint8_t *fw_data, uint32_t fw_len);
int mt76_load_ivb(struct mt76_dev *dev);

int mt76_send_command(struct mt76_dev *dev, const uint8_t *data, uint16_t len, enum mt76_mcu_cmd cmd);
int mt76_send_wlan(struct mt76_dev *dev, const uint8_t *data, uint16_t len);

int mt76_read_efuse(struct mt76_dev *dev, uint16_t addr, void *data, int len);

// MCU command functions
int mt76_select_function(struct mt76_dev *dev, enum mt76_mcu_function func, uint32_t val);
int mt76_set_power_mode(struct mt76_dev *dev, enum mt76_mcu_power_mode mode);
int mt76_load_cr(struct mt76_dev *dev, enum mt76_mcu_cr_mode mode);
int mt76_calibrate(struct mt76_dev *dev, enum mt76_mcu_calibration calib, uint32_t val);
int mt76_write_burst(struct mt76_dev *dev, uint32_t offset, const void *data, int len);

// Initialization functions
void mt76_init_registers(struct mt76_dev *dev);
int mt76_calibrate_crystal(struct mt76_dev *dev);
int mt76_calibrate_radio(struct mt76_dev *dev);
int mt76_init_address(struct mt76_dev *dev);
int mt76_init_radio(struct mt76_dev *dev);

// Channel management functions
int mt76_switch_channel(struct mt76_dev *dev, struct mt76_channel *chan);
int mt76_get_channel_power(struct mt76_dev *dev, struct mt76_channel *chan);
int mt76_evaluate_channels(struct mt76_dev *dev);
int mt76_set_channel_candidates(struct mt76_dev *dev);
int mt76_init_channels(struct mt76_dev *dev);

// LED control
int mt76_set_led_mode(struct mt76_dev *dev, uint8_t mode);

// Pairing functions
int mt76_set_pairing(struct mt76_dev *dev, bool enable);

// Client management
int mt76_add_client(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr);
int mt76_remove_client(struct mt76_dev *dev, uint8_t wcid);
int mt76_send_pair_response(struct mt76_dev *dev, const uint8_t *addr);
int mt76_set_client_key(struct mt76_dev *dev, uint8_t wcid, const uint8_t *key, uint16_t key_len);
int mt76_send_client_command(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr, 
                             uint8_t cmd, const uint8_t *data, uint16_t len);

// Wireless TX
int mt76_send_gip_data(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr,
                       const uint8_t *gip_data, uint16_t gip_len);
int mt76_usb_bulk_out(struct mt76_dev *dev, const uint8_t *data, uint16_t len);

// Channel scanning
void mt76_scan_channels(struct mt76_dev *dev);

#endif
