#include "mt76.h"
#include "mt76_defs.h"
#include "ieee80211.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "host/usbh.h"

// Microsoft-specific firmware commands (Xbox wireless protocol)
#define XONE_MT_SET_MAC_ADDRESS     0x00
#define XONE_MT_ADD_CLIENT          0x01
#define XONE_MT_REMOVE_CLIENT       0x02
#define XONE_MT_SET_IDLE_TIME       0x05
#define XONE_MT_SET_CHAN_CANDIDATES 0x07

// MCU Command Functions

int mt76_select_function(struct mt76_dev *dev, enum mt76_mcu_function func, uint32_t val) {
    uint8_t data[8];
    
    data[0] = func & 0xff;
    data[1] = (func >> 8) & 0xff;
    data[2] = (func >> 16) & 0xff;
    data[3] = (func >> 24) & 0xff;
    data[4] = val & 0xff;
    data[5] = (val >> 8) & 0xff;
    data[6] = (val >> 16) & 0xff;
    data[7] = (val >> 24) & 0xff;
    
    return mt76_send_command(dev, data, sizeof(data), MT_CMD_FUN_SET_OP);
}

int mt76_set_power_mode(struct mt76_dev *dev, enum mt76_mcu_power_mode mode) {
    uint8_t data[4];
    
    data[0] = mode & 0xff;
    data[1] = (mode >> 8) & 0xff;
    data[2] = (mode >> 16) & 0xff;
    data[3] = (mode >> 24) & 0xff;
    
    return mt76_send_command(dev, data, sizeof(data), MT_CMD_POWER_SAVING_OP);
}

int mt76_load_cr(struct mt76_dev *dev, enum mt76_mcu_cr_mode mode) {
    uint8_t data[4] = {0};
    
    data[0] = mode;
    
    return mt76_send_command(dev, data, sizeof(data), MT_CMD_LOAD_CR);
}

int mt76_calibrate(struct mt76_dev *dev, enum mt76_mcu_calibration calib, uint32_t val) {
    uint8_t data[8];
    
    data[0] = calib & 0xff;
    data[1] = (calib >> 8) & 0xff;
    data[2] = (calib >> 16) & 0xff;
    data[3] = (calib >> 24) & 0xff;
    data[4] = val & 0xff;
    data[5] = (val >> 8) & 0xff;
    data[6] = (val >> 16) & 0xff;
    data[7] = (val >> 24) & 0xff;
    
    return mt76_send_command(dev, data, sizeof(data), MT_CMD_CALIBRATION_OP);
}

int mt76_write_burst(struct mt76_dev *dev, uint32_t offset, const void *data, int len) {
    uint8_t buf[256];
    
    if (len + 4 > sizeof(buf)) {
        printf("MT76: Burst write too large\n");
        return -1;
    }
    
    // Write register offset
    uint32_t reg_offset = offset + MT_MCU_MEMMAP_WLAN;
    buf[0] = reg_offset & 0xff;
    buf[1] = (reg_offset >> 8) & 0xff;
    buf[2] = (reg_offset >> 16) & 0xff;
    buf[3] = (reg_offset >> 24) & 0xff;
    
    memcpy(buf + 4, data, len);
    
    return mt76_send_command(dev, buf, len + 4, MT_CMD_BURST_WRITE);
}

// Register Initialization

void mt76_init_registers(struct mt76_dev *dev) {
    printf("MT76: Initializing registers\n");
    
    // Reset MAC and BBP
    mt76_write_register(dev, MT_MAC_SYS_CTRL,
                       MT_MAC_SYS_CTRL_RESET_BBP | MT_MAC_SYS_CTRL_RESET_CSR);
    mt76_write_register(dev, MT_USB_DMA_CFG, 0);
    mt76_write_register(dev, MT_MAC_SYS_CTRL, 0);
    mt76_write_register(dev, MT_PWR_PIN_CFG, 0);
    
    // LDO and WPDMA configuration
    mt76_write_register(dev, MT_LDO_CTRL_1, 0x6b006464);
    mt76_write_register(dev, MT_WPDMA_GLO_CFG, 0x70);
    
    // WMM parameters
    mt76_write_register(dev, MT_WMM_AIFSN, 0x2273);
    mt76_write_register(dev, MT_WMM_CWMIN, 0x2344);
    mt76_write_register(dev, MT_WMM_CWMAX, 0x34aa);
    
    // FCE and PBF configuration
    mt76_write_register(dev, MT_FCE_DMA_ADDR, 0x041200);
    mt76_write_register(dev, MT_TSO_CTRL, 0);
    mt76_write_register(dev, MT_PBF_SYS_CTRL, 0x080c00);
    mt76_write_register(dev, MT_PBF_TX_MAX_PCNT, 0x1fbf1f1f);
    mt76_write_register(dev, MT_FCE_PSE_CTRL, 0x01);
    
    // Enable MAC TX/RX
    mt76_write_register(dev, MT_MAC_SYS_CTRL,
                       MT_MAC_SYS_CTRL_ENABLE_RX | MT_MAC_SYS_CTRL_ENABLE_TX);
    
    // Auto response and max length
    mt76_write_register(dev, MT_AUTO_RSP_CFG, 0x13);
    mt76_write_register(dev, MT_MAX_LEN_CFG, 0x3e3fff);
    
    // AMPDU configuration
    mt76_write_register(dev, MT_AMPDU_MAX_LEN_20M1S, 0xfffc9855);
    mt76_write_register(dev, MT_AMPDU_MAX_LEN_20M2S, 0xff);
    
    // Backoff slot configuration
    mt76_write_register(dev, MT_BKOFF_SLOT_CFG, 0x0109);
    mt76_write_register(dev, MT_PWR_PIN_CFG, 0);
    
    // EDCA parameters for each AC
    mt76_write_register(dev, MT_EDCA_CFG_AC(0), 0x064320);
    mt76_write_register(dev, MT_EDCA_CFG_AC(1), 0x0a4700);
    mt76_write_register(dev, MT_EDCA_CFG_AC(2), 0x043238);
    mt76_write_register(dev, MT_EDCA_CFG_AC(3), 0x03212f);
    
    // TX pin configuration
    mt76_write_register(dev, MT_TX_PIN_CFG, 0x150f0f);
    mt76_write_register(dev, MT_TX_SW_CFG0, 0x101001);
    mt76_write_register(dev, MT_TX_SW_CFG1, 0x010000);
    
    // TXOP and timeout configuration
    mt76_write_register(dev, MT_TXOP_CTRL_CFG, 0x10583f);
    mt76_write_register(dev, MT_TX_TIMEOUT_CFG, 0x0a0f90);
    mt76_write_register(dev, MT_TX_RETRY_CFG, 0x47d01f0f);
    
    // Protection configuration
    mt76_write_register(dev, MT_CCK_PROT_CFG, 0x03f40003);
    mt76_write_register(dev, MT_OFDM_PROT_CFG, 0x03f40003);
    mt76_write_register(dev, MT_MM20_PROT_CFG, 0x01742004);
    mt76_write_register(dev, MT_GF20_PROT_CFG, 0x01742004);
    mt76_write_register(dev, MT_GF40_PROT_CFG, 0x03f42084);
    
    // ACK time and TX ALC
    mt76_write_register(dev, MT_EXP_ACK_TIME, 0x2c00dc);
    mt76_write_register(dev, MT_TX_ALC_CFG_2, 0x22160a00);
    mt76_write_register(dev, MT_TX_ALC_CFG_3, 0x22160a76);
    mt76_write_register(dev, MT_TX_ALC_CFG_0, 0x3f3f1818);
    mt76_write_register(dev, MT_TX_ALC_CFG_4, 0x0606);
    
    // PIFS and RX filter
    mt76_write_register(dev, MT_PIFS_TX_CFG, 0x060fff);
    mt76_write_register(dev, MT_RX_FILTR_CFG, 0x017f17);
    
    // Basic rates
    mt76_write_register(dev, MT_LEGACY_BASIC_RATE, 0x017f);
    mt76_write_register(dev, MT_HT_BASIC_RATE, 0x8003);
    
    // Misc configuration
    mt76_write_register(dev, MT_PN_PAD_MODE, 0x02);
    mt76_write_register(dev, MT_TXOP_HLDR_ET, 0x02);
    
    // TX protection configuration
    mt76_write_register(dev, MT_TX_PROT_CFG6, 0xe3f42004);
    mt76_write_register(dev, MT_TX_PROT_CFG7, 0xe3f42084);
    mt76_write_register(dev, MT_TX_PROT_CFG8, 0xe3f42104);
    
    // DAC clock and RF PA mode
    mt76_write_register(dev, MT_DACCLK_EN_DLY_CFG, 0);
    mt76_write_register(dev, MT_RF_PA_MODE_ADJ0, 0xee000000);
    mt76_write_register(dev, MT_RF_PA_MODE_ADJ1, 0xee000000);
    
    // TX RF gain correction
    mt76_write_register(dev, MT_TX0_RF_GAIN_CORR, 0x0f3c3c3c);
    mt76_write_register(dev, MT_TX1_RF_GAIN_CORR, 0x0f3c3c3c);
    
    // PBF and pause control
    mt76_write_register(dev, MT_PBF_CFG, 0x1efebcf5);
    mt76_write_register(dev, MT_PAUSE_ENABLE_CONTROL1, 0x0a);
    
    // RF bypass and setting
    mt76_write_register(dev, MT_RF_BYPASS_0, 0x7f000000);
    mt76_write_register(dev, MT_RF_SETTING_0, 0x1a800000);
    
    // XIFS time and FCE
    mt76_write_register(dev, MT_XIFS_TIME_CFG, 0x33a40e0a);
    mt76_write_register(dev, MT_FCE_L2_STUFF, 0x03ff0223);
    
    // TX RTS and beacon
    mt76_write_register(dev, MT_TX_RTS_CFG, 0);
    mt76_write_register(dev, MT_BEACON_TIME_CFG, 0x0640);
    
    // Extended CCA and channel time
    mt76_write_register(dev, MT_EXT_CCA_CFG, 0xf0e4);
    mt76_write_register(dev, MT_CH_TIME_CFG, 0x015f);
    
    printf("MT76: Register initialization complete\n");
}

// Crystal Calibration

int mt76_calibrate_crystal(struct mt76_dev *dev) {
    uint8_t trim[4];
    uint16_t val;
    int8_t offset;
    uint32_t ctrl;
    int err;
    
    printf("MT76: Calibrating crystal\n");
    
    // Read crystal trim from EFUSE
    err = mt76_read_efuse(dev, MT_EE_XTAL_TRIM_2, trim, sizeof(trim));
    if (err) {
        printf("MT76: Failed to read XTAL trim\n");
        return err;
    }
    
    val = (trim[3] << 8) | trim[2];
    offset = val & 0x7f;
    
    if ((val & 0xff) == 0xff) {
        offset = 0;
    } else if (val & BIT(7)) {
        offset = -offset;
    }
    
    val >>= 8;
    if (!val || val == 0xff) {
        err = mt76_read_efuse(dev, MT_EE_XTAL_TRIM_1, trim, sizeof(trim));
        if (err) {
            printf("MT76: Failed to read XTAL trim 1\n");
            return err;
        }
        
        val = (trim[3] << 8) | trim[2];
        val &= 0xff;
        if (!val || val == 0xff) {
            val = 0x14;
        }
    }
    
    val = (val & 0x7f) + offset;
    
    // Configure crystal oscillator
    ctrl = mt76_read_register(dev, MT_XO_CTRL5 | MT_VEND_TYPE_CFG);
    ctrl &= ~MT_XO_CTRL5_C2_VAL;
    ctrl |= (val << 8) & MT_XO_CTRL5_C2_VAL;
    mt76_write_register(dev, MT_XO_CTRL5 | MT_VEND_TYPE_CFG, ctrl);
    
    mt76_write_register(dev, MT_XO_CTRL6 | MT_VEND_TYPE_CFG, MT_XO_CTRL6_C2_CTRL);
    mt76_write_register(dev, MT_CMB_CTRL, 0x0091a7ff);
    
    printf("MT76: Crystal calibration complete (trim=0x%02x)\n", val);
    return 0;
}

// Radio Calibration

int mt76_calibrate_radio(struct mt76_dev *dev) {
    int err;
    
    printf("MT76: Calibrating radio\n");
    
    // Configure AGC
    mt76_write_register(dev, MT_BBP(AGC, 8), 0x18365efa);
    mt76_write_register(dev, MT_BBP(AGC, 9), 0x18365efa);
    
    // Reset for reliable WLAN associations
    mt76_write_register(dev, MT_MAC_SYS_CTRL, 0);
    mt76_write_register(dev, MT_RF_BYPASS_0, 0);
    mt76_write_register(dev, MT_RF_SETTING_0, 0);
    
    err = mt76_calibrate(dev, MT_MCU_CAL_TEMP_SENSOR, 0);
    if (err) {
        printf("MT76: Temperature sensor calibration failed\n");
        return err;
    }
    
    // RXDCOC calibration
    err = mt76_calibrate(dev, MT_MCU_CAL_RXDCOC, 1);
    if (err) {
        printf("MT76: RXDCOC calibration failed\n");
        return err;
    }
    
    // RC calibration
    err = mt76_calibrate(dev, MT_MCU_CAL_RC, 0);
    if (err) {
        printf("MT76: RC calibration failed\n");
        return err;
    }
    
    // Enable MAC TX/RX
    mt76_write_register(dev, MT_MAC_SYS_CTRL,
                       MT_MAC_SYS_CTRL_ENABLE_RX | MT_MAC_SYS_CTRL_ENABLE_TX);
    
    printf("MT76: Radio calibration complete\n");
    return 0;
}

// Microsoft-specific command wrapper (for Xbox firmware)
static int mt76_send_ms_command(struct mt76_dev *dev, uint8_t cmd, const void *data, int len) {
    static uint8_t buffer[256];
    uint32_t *ptr = (uint32_t *)buffer;
    
    // MS command format: cmd_id (4 bytes) + data
    *ptr++ = cmd;
    
    if (data && len > 0) {
        memcpy(ptr, data, len);
    }
    
    return mt76_send_command(dev, buffer, sizeof(uint32_t) + len, MT_CMD_INIT_GAIN_OP);
}

// Send association response frame
int mt76_send_assoc_response(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr) {
    static uint8_t frame[256];
    int offset = 0;
    
    // TXWI header (32 bytes)
    struct mt76_txwi txwi = {0};
    txwi.flags = 0x0008;  // MT_TXWI_FLAGS_MPDU_DENSITY = 4
    txwi.rate = 0x0000;   // MT_PHY_TYPE_OFDM
    txwi.ack_ctl = 0x01;  // MT_TXWI_ACK_CTL_REQ
    txwi.wcid = 0xff;     // Broadcast WCID
    txwi.len_ctl = 32 + 8; // 802.11 header + assoc_resp + padding
    
    memcpy(&frame[offset], &txwi, sizeof(txwi));
    offset += sizeof(txwi);
    
    // 802.11 management frame header (24 bytes)
    uint16_t frame_control = IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ASSOC_RESP;
    memcpy(&frame[offset], &frame_control, 2); offset += 2;
    
    uint16_t duration = 0;
    memcpy(&frame[offset], &duration, 2); offset += 2;
    
    memcpy(&frame[offset], addr, 6); offset += 6;  // DA (destination)
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6;  // SA (source)
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6;  // BSSID
    
    uint16_t seq_ctrl = 0;
    memcpy(&frame[offset], &seq_ctrl, 2); offset += 2;
    
    // Association response body (6 bytes)
    uint16_t cap_info = 0;
    memcpy(&frame[offset], &cap_info, 2); offset += 2;
    
    uint16_t status_code = 0x0110;  // Original from xone
    memcpy(&frame[offset], &status_code, 2); offset += 2;
    
    uint16_t aid = 0x0f00;  // Association ID from xone
    memcpy(&frame[offset], &aid, 2); offset += 2;
    
    // 8 bytes padding
    memset(&frame[offset], 0, 8); offset += 8;
    
    // Prepare message header
    uint32_t info = (3 << 27) |  // MT_WLAN_PORT
                    (2 << 16) |  // MT_QSEL_EDCA
                    (1 << 15) |  // MT_TXD_INFO_WIV
                    (1 << 14);   // MT_TXD_INFO_80211
    
    static uint8_t msg[512];
    memcpy(msg, &info, 4);
    memcpy(msg + 4, frame, offset);
    
    return mt76_send_command(dev, msg, offset + 4, 0);  // Send as raw USB transfer
}

// Send pairing response to controller
int mt76_send_pair_response(struct mt76_dev *dev, const uint8_t *addr) {
    printf("MT76: Sending pairing response to controller\n");
    
    // Build 802.11 management frame with pairing response
    static uint8_t frame[256];
    int offset = 0;
    
    // TXWI header
    struct mt76_txwi txwi = {0};
    txwi.flags = 0x0008;
    txwi.rate = 0x0000;
    txwi.ack_ctl = 0x01;
    txwi.wcid = 0xff;
    txwi.len_ctl = 24 + 2 + 9;  // hdr + reserved + data
    
    memcpy(&frame[offset], &txwi, sizeof(txwi));
    offset += sizeof(txwi);
    
    // 802.11 header
    uint16_t frame_control = IEEE80211_FTYPE_MGMT | XONE_MT_WLAN_RESERVED;
    memcpy(&frame[offset], &frame_control, 2); offset += 2;
    memset(&frame[offset], 0, 2); offset += 2;  // duration
    memcpy(&frame[offset], addr, 6); offset += 6;  // addr1
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6;  // addr2
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6;  // addr3
    memset(&frame[offset], 0, 2); offset += 2;  // seq_ctrl
    
    // Pairing response data (from xone)
    frame[offset++] = XONE_MT_WLAN_RESERVED & 0xFF;
    frame[offset++] = 0x03;  // XONE_MT_CLIENT_PAIR_RESP
    
    // Pairing response data
    uint8_t pair_data[] = { 0x00, 0x45, 0x55, 0x01, 0x0f, 0x8f, 0xff, 0x87, 0x1f };
    memcpy(&frame[offset], pair_data, sizeof(pair_data));
    offset += sizeof(pair_data);
    
    // Send via USB
    uint32_t info = (3 << 27) | (2 << 16) | (1 << 15) | (1 << 14);
    static uint8_t msg[512];
    memcpy(msg, &info, 4);
    memcpy(msg + 4, frame, offset);
    
    return mt76_send_command(dev, msg, offset + 4, 0);
}

// Set encryption key for client
int mt76_set_client_key(struct mt76_dev *dev, uint8_t wcid, const uint8_t *key, uint16_t key_len) {
    int err;
    
    if (key_len != 16) {  // XONE_MT_WCID_KEY_LEN
        printf("MT76: Invalid key length %d (expected 16)\n", key_len);
        return -1;
    }
    
    printf("MT76: Setting encryption key for WCID %d\n", wcid);
    
    // Write key to WCID key table
    err = mt76_write_burst(dev, MT_WCID_KEY(wcid), key, key_len);
    if (err) {
        printf("MT76: Failed to write WCID key\n");
        return err;
    }
    
    // Write IV
    uint8_t iv[] = { 0x01, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00 };
    err = mt76_write_burst(dev, MT_WCID_IV(wcid), iv, sizeof(iv));
    if (err) {
        printf("MT76: Failed to write WCID IV\n");
        return err;
    }
    
    // Set WCID attributes (AES-CCMP encryption, pairwise)
    uint32_t attr = (7 << 1) | 1;  // MT_CIPHER_AES_CCMP=7, MT_WCID_ATTR_PAIRWISE=1
    err = mt76_write_burst(dev, MT_WCID_ATTR(wcid), (uint8_t*)&attr, sizeof(attr));
    if (err) {
        printf("MT76: Failed to write WCID attributes\n");
        return err;
    }
    
    printf("MT76: Encryption enabled for WCID %d\n", wcid);
    return 0;
}

// Send client command (for encryption acknowledgment)
int mt76_send_client_command(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr,
                             uint8_t cmd, const uint8_t *data, uint16_t len) {
    static uint8_t frame[512];
    uint16_t offset = 0;
    
    // TX info header (8 bytes)
    uint32_t info = (MT_WLAN_PORT << 27) | (MT_QSEL_EDCA << 16);
    memcpy(&frame[offset], &info, 4); offset += 4;
    memset(&frame[offset], 0, 4); offset += 4;
    
    // TXWI header
    struct mt76_txwi txwi = {0};
    txwi.flags = (4 << 0);  // MPDU_DENSITY_4
    txwi.rate = (0 << 0);   // PHY_TYPE_OFDM
    txwi.ack_ctl = 1;       // Request ACK
    txwi.wcid = wcid - 1;
    txwi.len_ctl = sizeof(struct ieee80211_hdr) + 2 + len;
    memcpy(&frame[offset], &txwi, sizeof(txwi));
    offset += sizeof(txwi);
    
    // 802.11 header
    uint16_t frame_control = IEEE80211_FTYPE_MGMT | XONE_MT_WLAN_RESERVED;
    memcpy(&frame[offset], &frame_control, 2); offset += 2;
    memset(&frame[offset], 0, 2); offset += 2;
    memcpy(&frame[offset], addr, 6); offset += 6;
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6;
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6;
    memset(&frame[offset], 0, 2); offset += 2;
    
    // Command data
    frame[offset++] = XONE_MT_WLAN_RESERVED;
    frame[offset++] = cmd;
    if (data && len > 0) {
        memcpy(&frame[offset], data, len);
        offset += len;
    }
    
    printf("MT76: Sending client command 0x%02X to WCID %d, total_len=%d\n", cmd, wcid, offset);
    
    // Send via USB bulk OUT (not MCU command)
    return mt76_usb_bulk_out(dev, frame, offset);
}

int mt76_send_gip_data(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr,
                       const uint8_t *gip_data, uint16_t gip_len) {
    static uint8_t frame[512];
    uint16_t offset = 0;
    
    // TX info header (8 bytes)
    uint32_t info = (MT_WLAN_PORT << 27) | (MT_QSEL_EDCA << 16) | (1 << 15) | (1 << 7);  // WIV | 80211
    memcpy(&frame[offset], &info, 4); offset += 4;
    memset(&frame[offset], 0, 4); offset += 4;
    
    // TXWI header
    struct mt76_txwi txwi = {0};
    txwi.flags = (4 << 0);  // MPDU_DENSITY_4
    txwi.rate = (0 << 0);   // PHY_TYPE_OFDM
    txwi.ack_ctl = 1;       // Request ACK
    txwi.wcid = wcid - 1;
    txwi.len_ctl = 26 + gip_len;  // 802.11 QoS header (26 bytes) + GIP data
    memcpy(&frame[offset], &txwi, sizeof(txwi));
    offset += sizeof(txwi);
    
    // 802.11 QoS Data header (26 bytes)
    uint16_t frame_control = IEEE80211_FTYPE_DATA | IEEE80211_STYPE_QOS_DATA;
    memcpy(&frame[offset], &frame_control, 2); offset += 2;  // Frame control
    memset(&frame[offset], 0, 2); offset += 2;                // Duration
    memcpy(&frame[offset], addr, 6); offset += 6;             // Address 1 (destination)
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6; // Address 2 (source)
    memcpy(&frame[offset], dev->mac_address, 6); offset += 6; // Address 3 (BSSID)
    memset(&frame[offset], 0, 2); offset += 2;                // Sequence control
    memset(&frame[offset], 0, 2); offset += 2;                // QoS control
    
    // GIP data payload
    memcpy(&frame[offset], gip_data, gip_len);
    offset += gip_len;
    
    printf("MT76: Sending GIP data to WCID %d, gip_len=%d, total_len=%d\n", wcid, gip_len, offset);
    
    // Send via USB bulk OUT (not MCU command)
    return mt76_usb_bulk_out(dev, frame, offset);
}

// Remove client (controller disconnection)
int mt76_remove_client(struct mt76_dev *dev, uint8_t wcid) {
    int err;
    
    printf("MT76: Removing client wcid=%d\n", wcid);
    
    // Clear WCID address
    uint8_t zero_addr[6] = {0};
    err = mt76_write_burst(dev, MT_WCID_ADDR(wcid), zero_addr, 6);
    if (err) {
        printf("MT76: Failed to clear WCID address\n");
        return err;
    }
    
    // Send REMOVE_CLIENT command to firmware
    // Data format: [wcid-1, 0x00, 0x00, 0x00]
    uint8_t data[4] = { wcid - 1, 0x00, 0x00, 0x00 };
    err = mt76_send_ms_command(dev, XONE_MT_REMOVE_CLIENT, data, sizeof(data));
    if (err) {
        printf("MT76: Failed to send REMOVE_CLIENT command\n");
        return err;
    }
    
    printf("MT76: Client removed successfully\n");
    return 0;
}

// Add client (controller association)
int mt76_add_client(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr) {
    int err;
    
    printf("MT76: Adding client wcid=%d, addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
           wcid, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    
    // Write client MAC address to WCID table
    err = mt76_write_burst(dev, MT_WCID_ADDR(wcid), addr, 6);
    if (err) {
        printf("MT76: Failed to write WCID address\n");
        return err;
    }
    
    // Send ADD_CLIENT command to firmware
    // Data format: [wcid-1, 0x00, 0x00, 0x00, 0x40, 0x1f, 0x00, 0x00]
    uint8_t data[8] = { wcid - 1, 0x00, 0x00, 0x00, 0x40, 0x1f, 0x00, 0x00 };
    err = mt76_send_ms_command(dev, XONE_MT_ADD_CLIENT, data, sizeof(data));
    if (err) {
        printf("MT76: Failed to send ADD_CLIENT command\n");
        return err;
    }
    
    // Send association response frame
    printf("MT76: Sending association response\n");
    err = mt76_send_assoc_response(dev, wcid, addr);
    if (err) {
        printf("MT76: Failed to send association response\n");
        return err;
    }
    
    printf("MT76: Client added successfully\n");
    return 0;
}

// MAC Address Initialization

int mt76_init_address(struct mt76_dev *dev) {
    int err;
    
    printf("MT76: Initializing MAC address\n");
    
    // Read MAC address from EFUSE
    err = mt76_read_efuse(dev, MT_EE_MAC_ADDR, dev->mac_address, MT76_MAC_ADDR_LEN);
    if (err) {
        printf("MT76: Failed to read MAC address from EFUSE\n");
        return err;
    }
    
    printf("MT76: EFUSE MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           dev->mac_address[0], dev->mac_address[1], dev->mac_address[2],
           dev->mac_address[3], dev->mac_address[4], dev->mac_address[5]);
    
    // Xbox adapters need specific MAC prefix (62:45:bd)
    if (dev->mac_address[0] != 0x62) {
        dev->mac_address[0] = 0x62;
        dev->mac_address[1] = 0x45;
        dev->mac_address[2] = 0xbd;
        printf("MT76: Adjusted MAC to Xbox format\n");
    }
    
    // Write MAC address to registers
    err = mt76_write_burst(dev, MT_MAC_ADDR_DW0, dev->mac_address, MT76_MAC_ADDR_LEN);
    if (err) {
        printf("MT76: Failed to write MAC_ADDR\n");
        return err;
    }
    
    err = mt76_write_burst(dev, MT_MAC_BSSID_DW0, dev->mac_address, MT76_MAC_ADDR_LEN);
    if (err) {
        printf("MT76: Failed to write BSSID\n");
        return err;
    }
    
    // Send MAC address to firmware
    err = mt76_send_ms_command(dev, XONE_MT_SET_MAC_ADDRESS, dev->mac_address, MT76_MAC_ADDR_LEN);
    if (err) {
        printf("MT76: Failed to send MAC address to firmware\n");
        return err;
    }
    
    printf("MT76: MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           dev->mac_address[0], dev->mac_address[1], dev->mac_address[2],
           dev->mac_address[3], dev->mac_address[4], dev->mac_address[5]);
    
    return 0;
}

// Main Radio Initialization

int mt76_init_radio(struct mt76_dev *dev) {
    int err;
    
    printf("MT76: Starting radio initialization\n");
    
    // Give device significant time to boot after firmware load
    // The MT76 chip may reset after IVB load
    printf("MT76: Waiting for device to boot with new firmware...\n");
    sleep_ms(1000);
    
    // 1. Select function (Q_SELECT = 1)
    err = mt76_select_function(dev, MT_Q_SELECT, 1);
    if (err) {
        printf("MT76: Function select failed\n");
        return err;
    }
    
    // 2. Set power mode ON
    err = mt76_set_power_mode(dev, MT_RADIO_ON);
    if (err) {
        printf("MT76: Power mode set failed\n");
        return err;
    }
    
    // 3. Load CR (calibration registers)
    err = mt76_load_cr(dev, MT_RF_BBP_CR);
    if (err) {
        printf("MT76: Load CR failed\n");
        return err;
    }
    
    // 4. Initialize registers
    mt76_init_registers(dev);
    
    // 5. Calibrate crystal
    err = mt76_calibrate_crystal(dev);
    if (err) {
        printf("MT76: Crystal calibration failed\n");
        return err;
    }
    
    // 6. Initialize MAC address
    err = mt76_init_address(dev);
    if (err) {
        printf("MT76: MAC address init failed\n");
        return err;
    }
    
    // 7. Set idle time to prevent disconnections
    printf("MT76: Setting idle time\n");
    uint32_t idle_time = 64;
    err = mt76_send_ms_command(dev, XONE_MT_SET_IDLE_TIME, &idle_time, sizeof(idle_time));
    if (err) {
        printf("MT76: Set idle time failed\n");
        return err;
    }
    
    // 8. Calibrate radio
    err = mt76_calibrate_radio(dev);
    if (err) {
        printf("MT76: Radio calibration failed\n");
        return err;
    }
    
    dev->initialized = true;
    printf("MT76: Radio initialization complete!\n");
    
    return 0;
}

// Channel Management Functions

// Default channel configuration from xone driver
static const struct mt76_channel default_channels[MT76_NUM_CHANNELS] = {
    { 0x01, MT_CH_2G_LOW, MT_PHY_BW_20, 0, true, 0, false },
    { 0x06, MT_CH_2G_MID, MT_PHY_BW_20, 0, true, 0, false },
    { 0x0b, MT_CH_2G_HIGH, MT_PHY_BW_20, 0, true, 0, false },
    { 0x24, MT_CH_5G_LOW, MT_PHY_BW_40, MT_CH_5G_UNII_1, true, 0, false },
    { 0x28, MT_CH_5G_LOW, MT_PHY_BW_40, MT_CH_5G_UNII_1, false, 0, false },
    { 0x2c, MT_CH_5G_HIGH, MT_PHY_BW_40, MT_CH_5G_UNII_1, true, 0, false },
    { 0x30, MT_CH_5G_HIGH, MT_PHY_BW_40, MT_CH_5G_UNII_1, false, 0, false },
    { 0x95, MT_CH_5G_LOW, MT_PHY_BW_80, MT_CH_5G_UNII_3, true, 0, false },
    { 0x99, MT_CH_5G_LOW, MT_PHY_BW_80, MT_CH_5G_UNII_3, false, 0, false },
    { 0x9d, MT_CH_5G_HIGH, MT_PHY_BW_80, MT_CH_5G_UNII_3, true, 0, false },
    { 0xa1, MT_CH_5G_HIGH, MT_PHY_BW_80, MT_CH_5G_UNII_3, false, 0, false },
};

int mt76_switch_channel(struct mt76_dev *dev, struct mt76_channel *chan) {
    uint8_t data[8];
    int err;
    
    printf("MT76: Switching to channel 0x%02X (band=%d, bw=%d, scan=%d)\n",
           chan->index, chan->band, chan->bandwidth, chan->scan);
    
    memset(data, 0, sizeof(data));
    data[0] = chan->index;
    data[1] = chan->scan ? 1 : 0;
    data[2] = chan->band;
    data[3] = chan->bandwidth;
    data[4] = chan->band_5g;
    data[5] = chan->primary ? 1 : 0;
    
    err = mt76_send_command(dev, data, sizeof(data), MT_CMD_SWITCH_CHANNEL_OP);
    if (err) {
        printf("MT76: Channel switch failed\n");
        return err;
    }
    
    dev->current_channel = chan;
    return 0;
}

int mt76_get_channel_power(struct mt76_dev *dev, struct mt76_channel *chan) {
    uint8_t data[4];
    int err;
    
    data[0] = chan->index;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    
    err = mt76_send_command(dev, data, sizeof(data), MT_CMD_GET_CHANNEL_POWER);
    if (err) {
        return err;
    }
    
    // Power value would be returned via MCU response
    // Setting to 0 is fine - actual power is managed by MT76 firmware
    chan->power = 0;
    return 0;
}

int mt76_evaluate_channels(struct mt76_dev *dev) {
    int i;
    int8_t best_power = -128;
    struct mt76_channel *best_chan = &dev->channels[0];
    
    printf("MT76: Evaluating channels\n");
    
    for (i = 0; i < MT76_NUM_CHANNELS; i++) {
        struct mt76_channel *chan = &dev->channels[i];
        
        // Get channel power
        mt76_get_channel_power(dev, chan);
        
        // Increase power for certain 5GHz channels (0x24 to 0x30)
        if (chan->index >= 0x24 && chan->index <= 0x30) {
            chan->power += 10;
        }
        
        printf("MT76: Channel 0x%02X power: %d\n", chan->index, chan->power);
        
        if (chan->power > best_power) {
            best_power = chan->power;
            best_chan = chan;
        }
    }
    
    printf("MT76: Best channel: 0x%02X (power=%d)\n", best_chan->index, best_power);
    
    // Switch to best channel with scanning enabled
    best_chan->scan = true;
    return mt76_switch_channel(dev, best_chan);
}

int mt76_set_channel_candidates(struct mt76_dev *dev) {
    uint32_t data[64];  // Enough for header + all channels
    int count = 0;
    uint8_t best_chan = dev->current_channel ? dev->current_channel->index : 0x01;
    
    printf("MT76: Setting channel candidates\n");
    
    // Data format from xone: [1, best_chan, num_other_channels, channel_list...]
    data[count++] = 1;  // Constant
    data[count++] = best_chan;  // Best channel
    data[count++] = MT76_NUM_CHANNELS - 1;  // Number of other channels
    
    // Channel list (excluding best channel)
    for (int i = 0; i < MT76_NUM_CHANNELS; i++) {
        if (dev->channels[i].index != best_chan) {
            data[count++] = dev->channels[i].index;
        }
    }
    
    return mt76_send_ms_command(dev, XONE_MT_SET_CHAN_CANDIDATES, data, count * sizeof(uint32_t));
}

int mt76_init_channels(struct mt76_dev *dev) {
    int err;
    
    printf("MT76: Initializing channels\n");
    
    // Copy default channel configuration
    memcpy(dev->channels, default_channels, sizeof(default_channels));
    
    // Enable promiscuous mode for channel evaluation
    printf("MT76: Enabling promiscuous mode for channel evaluation\n");
    mt76_write_register(dev, MT_RX_FILTR_CFG, 0x014f13);
    
    // Evaluate channels and switch to best one
    err = mt76_evaluate_channels(dev);
    if (err) {
        printf("MT76: Channel evaluation failed\n");
        return err;
    }
    
    // Disable promiscuous mode
    printf("MT76: Disabling promiscuous mode\n");
    mt76_write_register(dev, MT_RX_FILTR_CFG, 0x017f17);
    
    // Set channel candidates
    err = mt76_set_channel_candidates(dev);
    if (err) {
        printf("MT76: Set channel candidates failed\n");
        return err;
    }
    
    printf("MT76: Channel initialization complete\n");
    return 0;
}

// LED Control

int mt76_set_led_mode(struct mt76_dev *dev, uint8_t mode) {
    uint8_t data[4];
    
    data[0] = mode & 0xff;
    data[1] = (mode >> 8) & 0xff;
    data[2] = (mode >> 16) & 0xff;
    data[3] = (mode >> 24) & 0xff;
    
    return mt76_send_command(dev, data, sizeof(data), MT_CMD_LED_MODE_OP);
}

// Beacon Functions

static int mt76_write_beacon(struct mt76_dev *dev, bool pair) {
    uint8_t beacon_data[256];
    int offset = 0;
    
    // TXWI (TX Wireless Info) - 16 bytes
    struct {
        uint16_t flags;
        uint16_t rate;
        uint8_t ack_ctl;
        uint8_t wcid;
        uint16_t len_ctl;
        uint32_t reserved[2];
    } __attribute__((packed)) txwi = {0};
    
    // Generate beacon timestamp, use hardware sequence control
    txwi.flags = MT_TXWI_FLAGS_TS;
    txwi.rate = FIELD_PREP(MT_RXWI_RATE_PHY, MT_PHY_TYPE_OFDM);
    txwi.ack_ctl = MT_TXWI_ACK_CTL_NSEQ;
    
    // IEEE 802.11 management frame header (24 bytes for 3-address)
    struct {
        uint16_t frame_control;
        uint16_t duration;
        uint8_t da[6];      // Destination address (broadcast)
        uint8_t sa[6];      // Source address (our MAC)
        uint8_t bssid[6];   // BSSID (our MAC)
        uint16_t seq_ctrl;
    } __attribute__((packed)) hdr = {0};
    
    hdr.frame_control = (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_BEACON);
    memset(hdr.da, 0xff, 6);  // Broadcast
    memcpy(hdr.sa, dev->mac_address, 6);
    memcpy(hdr.bssid, dev->mac_address, 6);
    
    // Beacon frame body
    struct {
        uint64_t timestamp;
        uint16_t beacon_int;
        uint16_t capab_info;
    } __attribute__((packed)) beacon = {0};
    
    beacon.beacon_int = 100;  // 100 ms
    beacon.capab_info = 0xc631;  // Original capability info from xone
    
    // Microsoft OUI information element
    uint8_t ms_ie[] = {
        0x00, 0x00,  // Timestamp continuation
        0xdd, 0x10,  // Vendor specific IE, length 16
        0x00, 0x50, 0xf2, 0x11,  // Microsoft OUI + type
        0x01, 0x10, pair ? 1 : 0, 0xa5, 0x30, 0x99, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };
    
    int mgmt_len = sizeof(hdr) + sizeof(beacon);
    txwi.len_ctl = mgmt_len + sizeof(ms_ie);
    
    // Build complete beacon packet
    memcpy(beacon_data + offset, &txwi, sizeof(txwi));
    offset += sizeof(txwi);
    
    memcpy(beacon_data + offset, &hdr, sizeof(hdr));
    offset += sizeof(hdr);
    
    memcpy(beacon_data + offset, &beacon, sizeof(beacon));
    offset += sizeof(beacon);
    
    memcpy(beacon_data + offset, ms_ie, sizeof(ms_ie));
    offset += sizeof(ms_ie);
    
    printf("MT76: Writing beacon (%d bytes, pair=%d)\n", offset, pair);
    
    // Write beacon to MT_BEACON_BASE
    return mt76_write_burst(dev, MT_BEACON_BASE, beacon_data, offset);
}

// Pairing Functions

int mt76_set_pairing(struct mt76_dev *dev, bool enable) {
    int err;
    
    printf("MT76: %s pairing mode\n", enable ? "Enabling" : "Disabling");
    
    // Set LED mode: blink when pairing, solid when not
    err = mt76_set_led_mode(dev, enable ? MT_LED_BLINK : MT_LED_ON);
    if (err) {
        printf("MT76: LED mode set failed\n");
        return err;
    }
    // Service USB stack to complete the transfer
    for (int i = 0; i < 20; i++) {
        tuh_task();
        tud_task();
        sleep_ms(10);
    }
    
    // Write beacon frame
    err = mt76_write_beacon(dev, enable);
    if (err) {
        printf("MT76: Beacon write failed\n");
        return err;
    }
    // Service USB stack to complete the transfer
    for (int i = 0; i < 30; i++) {
        tuh_task();
        tud_task();
        sleep_ms(10);
    }
    
    if (dev->current_channel) {
        dev->current_channel->scan = enable;
        printf("MT76: Scan mode %s on channel 0x%02X\n", 
               enable ? "enabled" : "disabled", dev->current_channel->index);
    }
    // Service USB stack to complete the transfer
    for (int i = 0; i < 20; i++) {
        tuh_task();
        tud_task();
        sleep_ms(10);
    }
    
    // Enable RX and TX in MAC system control
    printf("MT76: Enabling MAC RX/TX\n");
    mt76_write_register(dev, MT_MAC_SYS_CTRL, 
                       MT_MAC_SYS_CTRL_ENABLE_RX | MT_MAC_SYS_CTRL_ENABLE_TX);
    sleep_ms(100);
    
    // Enable beacon transmission timer
    // TSF timer, TBTT timer, AP mode, beacon TX, interval = 0x0640 (100ms)
    printf("MT76: Enabling beacon timer\n");
    mt76_write_register(dev, MT_BEACON_TIME_CFG,
                       MT_BEACON_TIME_CFG_BEACON_TX |
                       MT_BEACON_TIME_CFG_TBTT_EN |
                       MT_BEACON_TIME_CFG_SYNC_MODE |
                       MT_BEACON_TIME_CFG_TIMER_EN |
                       FIELD_PREP(MT_BEACON_TIME_CFG_INTVAL, 0x0640));
    
    printf("MT76: Pairing mode %s\n", enable ? "enabled" : "disabled");
    return 0;
}

// Channel Scanning

void mt76_scan_channels(struct mt76_dev *dev) {
    static int current_channel_idx = 0;
    static uint32_t last_scan_time = 0;
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Switch channels every 2 seconds
    if (now - last_scan_time < 2000) {
        return;
    }
    
    last_scan_time = now;
    
    // Move to next channel
    current_channel_idx = (current_channel_idx + 1) % MT76_NUM_CHANNELS;
    
    struct mt76_channel *chan = &dev->channels[current_channel_idx];
    chan->scan = true;
    
    printf("MT76: Scanning channel 0x%02X (%d/%d)\n", 
           chan->index, current_channel_idx + 1, MT76_NUM_CHANNELS);
    
    mt76_switch_channel(dev, chan);
}
