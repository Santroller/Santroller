#include "mt76.h"
#include "tusb.h"
#include "host/usbh.h"
#include <stdio.h>
#include <string.h>

static uint8_t bulk_buffer[MT_FW_CHUNK_SIZE + MT_CMD_HDR_LEN * 2];
static uint8_t tx_buffer[512];  // Buffer for wireless TX packets

bool mt76_init(struct mt76_dev *dev, uint8_t dev_addr) {
    memset(dev, 0, sizeof(*dev));
    dev->dev_addr = dev_addr;
    dev->initialized = false;
    
    printf("MT76: Initializing device at address %d\n", dev_addr);
    
    uint32_t asic_ver = mt76_read_register(dev, MT_ASIC_VERSION);
    printf("MT76: ASIC version: 0x%08lX\n", asic_ver);
    
    if (asic_ver == 0 || asic_ver == 0xFFFFFFFF) {
        printf("MT76: Failed to read ASIC version\n");
        return false;
    }
    
    return true;
}

void mt76_deinit(struct mt76_dev *dev) {
    dev->initialized = false;
}

uint32_t mt76_read_register(struct mt76_dev *dev, uint32_t addr) {
    uint8_t req = MT_VEND_MULTI_READ;
    
    if (addr & MT_VEND_TYPE_CFG) {
        req = MT_VEND_READ_CFG;
        addr &= ~MT_VEND_TYPE_CFG;
    }
    
    tusb_control_request_t const setup = {
        .bmRequestType = 0xC0,  // Device to Host, Vendor, Device
        .bRequest = req,
        .wValue = tu_htole16(addr >> 16),
        .wIndex = tu_htole16(addr & 0xFFFF),
        .wLength = tu_htole16(sizeof(dev->control_data))
    };
    
    tuh_xfer_t xfer = {
        .daddr = dev->dev_addr,
        .ep_addr = 0,
        .setup = &setup,
        .buffer = (uint8_t*)&dev->control_data,
        .complete_cb = NULL,  // Blocking
        .user_data = 0
    };
    
    if (!tuh_control_xfer(&xfer)) {
        printf("MT76: Register read failed at 0x%08lX\n", addr);
        return 0;
    }
    
    if (xfer.result != XFER_RESULT_SUCCESS) {
        printf("MT76: Register read transfer failed at 0x%08lX\n", addr);
        return 0;
    }
    
    return dev->control_data;
}

void mt76_write_register(struct mt76_dev *dev, uint32_t addr, uint32_t val) {
    uint8_t req = MT_VEND_MULTI_WRITE;
    
    if (addr & MT_VEND_TYPE_CFG) {
        req = MT_VEND_WRITE_CFG;
        addr &= ~MT_VEND_TYPE_CFG;
    }
    
    dev->control_data = val;
    
    tusb_control_request_t const setup = {
        .bmRequestType = 0x40,  // Host to Device, Vendor, Device
        .bRequest = req,
        .wValue = tu_htole16(addr >> 16),
        .wIndex = tu_htole16(addr & 0xFFFF),
        .wLength = tu_htole16(sizeof(dev->control_data))
    };
    
    tuh_xfer_t xfer = {
        .daddr = dev->dev_addr,
        .ep_addr = 0,
        .setup = &setup,
        .buffer = (uint8_t*)&dev->control_data,
        .complete_cb = NULL,  // Blocking
        .user_data = 0
    };
    
    if (!tuh_control_xfer(&xfer)) {
        printf("MT76: Register write failed at 0x%08lX\n", addr);
        return;
    }
    
    if (xfer.result != XFER_RESULT_SUCCESS) {
        printf("MT76: Register write transfer failed at 0x%08lX\n", addr);
    }
}

bool mt76_poll(struct mt76_dev *dev, uint32_t offset, uint32_t mask, uint32_t val) {
    uint32_t reg = 0;
    for (int i = 0; i < MT_POLL_RETRIES; i++) {
        reg = mt76_read_register(dev, offset);
        if ((reg & mask) == val) {
            return true;
        }
        sleep_ms(20);
    }
    
    printf("MT76: Poll timeout at 0x%08lX (expected=0x%08lX, got=0x%08lX, masked=0x%08lX)\n", 
           offset, val, reg, reg & mask);
    return false;
}

int mt76_load_ivb(struct mt76_dev *dev) {
    printf("MT76: Loading IVB\n");
    
    tusb_control_request_t const setup = {
        .bmRequestType = 0x40,
        .bRequest = MT_VEND_DEV_MODE,
        .wValue = tu_htole16(MT_FW_LOAD_IVB),
        .wIndex = 0,
        .wLength = 0
    };
    
    tuh_xfer_t xfer = {
        .daddr = dev->dev_addr,
        .ep_addr = 0,
        .setup = &setup,
        .buffer = NULL,
        .complete_cb = NULL,
        .user_data = 0
    };
    
    if (!tuh_control_xfer(&xfer)) {
        printf("MT76: IVB load failed\n");
        return -1;
    }
    
    if (xfer.result != XFER_RESULT_SUCCESS) {
        printf("MT76: IVB load transfer failed\n");
        return -1;
    }
    
    printf("MT76: Waiting for IVB completion...\n");
    sleep_ms(100);  // Give device time to process IVB
    
    // Debug: check what the register contains
    uint32_t reg_val = mt76_read_register(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG);
    printf("MT76: FCE_DMA_ADDR = 0x%08lX\n", reg_val);
    
    if (!mt76_poll(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG, 0x01, 0x01)) {
        printf("MT76: IVB completion timeout\n");
        return -1;
    }
    
    printf("MT76: IVB loaded successfully\n");
    return 0;
}

int mt76_read_efuse(struct mt76_dev *dev, uint16_t addr, void *data, int len) {
    uint32_t ctrl, offset, val;
    int i, remaining;
    
    ctrl = mt76_read_register(dev, MT_EFUSE_CTRL);
    ctrl &= ~(MT_EFUSE_CTRL_AIN | MT_EFUSE_CTRL_MODE);
    ctrl |= MT_EFUSE_CTRL_KICK;
    ctrl |= FIELD_PREP(MT_EFUSE_CTRL_AIN, addr & ~0x0f);
    ctrl |= FIELD_PREP(MT_EFUSE_CTRL_MODE, MT_EE_READ);
    mt76_write_register(dev, MT_EFUSE_CTRL, ctrl);
    
    if (!mt76_poll(dev, MT_EFUSE_CTRL, MT_EFUSE_CTRL_KICK, 0)) {
        return -1;
    }
    
    for (i = 0; i < len; i += sizeof(uint32_t)) {
        offset = (addr & 0x0C) + i;
        val = mt76_read_register(dev, MT_EFUSE_DATA_BASE + offset);
        remaining = (len - i < sizeof(uint32_t)) ? (len - i) : sizeof(uint32_t);
        memcpy((uint8_t *)data + i, &val, remaining);
    }
    
    return 0;
}

static void mt76_prep_message(uint8_t *buffer, uint16_t *len, uint32_t info) {
    int pad;
    int msg_len = *len;
    
    msg_len = (msg_len + 3) & ~3;
    pad = msg_len - *len + MT_CMD_HDR_LEN;
    
    uint32_t header = info | FIELD_PREP(MT_MCU_MSG_LEN, msg_len);
    memcpy(buffer - MT_CMD_HDR_LEN, &header, MT_CMD_HDR_LEN);
    
    memset(buffer + *len, 0, pad);
    *len = msg_len + MT_CMD_HDR_LEN + pad;
}

int mt76_send_command(struct mt76_dev *dev, const uint8_t *data, uint16_t len, enum mt76_mcu_cmd cmd) {
    if (len > sizeof(bulk_buffer) - MT_CMD_HDR_LEN * 2) {
        printf("MT76: Command too large\n");
        return -1;
    }
    
    memcpy(bulk_buffer + MT_CMD_HDR_LEN, data, len);
    
    uint32_t info = MT_MCU_MSG_TYPE_CMD |
                    FIELD_PREP(MT_MCU_MSG_PORT, MT_CPU_TX_PORT) |
                    FIELD_PREP(MT_MCU_MSG_CMD_TYPE, cmd);
    
    uint16_t total_len = len;
    mt76_prep_message(bulk_buffer + MT_CMD_HDR_LEN, &total_len, info);
    
    printf("MT76: Sending command 0x%02X, len=%d (dev_addr=%d, ep=0x%02X)\n", 
           cmd, total_len, dev->dev_addr, MT_EP_OUT);
    
    tuh_xfer_t xfer = {
        .daddr = dev->dev_addr,
        .ep_addr = MT_EP_OUT,
        .buflen = total_len,
        .buffer = bulk_buffer,
        .complete_cb = NULL,  // Blocking
        .user_data = 0
    };
    
    // Retry a few times if endpoint is busy
    int retries = 5;
    while (retries > 0) {
        if (tuh_edpt_xfer(&xfer)) {
            break;  // Success
        }
        
        retries--;
        if (retries > 0) {
            printf("MT76: Endpoint busy, retrying... (%d left)\n", retries);
            // Process USB events to clear any pending state
            for (int i = 0; i < 10; i++) {
                tuh_task();
                tud_task();
                sleep_ms(2);
            }
        } else {
            printf("MT76: Bulk transfer failed (tuh_edpt_xfer returned false)\n");
            printf("MT76: Device may not be ready or endpoint not available\n");
            return -1;
        }
    }
    
    if (xfer.result != XFER_RESULT_SUCCESS) {
        printf("MT76: Bulk transfer completed with error\n");
        return -1;
    }
    
    // Give device time to process command before next one
    sleep_ms(10);
    
    return 0;
}

static int mt76_send_firmware_part(struct mt76_dev *dev, uint32_t base_offset, 
                                    const uint8_t *data, uint32_t len) {
    uint32_t offset = 0;
    
    while (offset < len) {
        uint32_t chunk_size = (len - offset > MT_FW_CHUNK_SIZE) ? 
                              MT_FW_CHUNK_SIZE : (len - offset);
        uint32_t chunk_len_aligned = (chunk_size + 3) & ~3;
        
        printf("MT76: Loading chunk at offset %lu, size %lu (aligned %lu)\n", 
               offset, chunk_size, chunk_len_aligned);
        
        // Write DMA registers BEFORE sending data (like xone)
        mt76_write_register(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG, 
                           base_offset + offset);
        mt76_write_register(dev, MT_FCE_DMA_LEN | MT_VEND_TYPE_CFG, 
                           chunk_len_aligned << 16);
        
        // Send firmware chunk with MCU message header (cmd=0 for firmware data)
        if (mt76_send_command(dev, data + offset, chunk_size, 0) < 0) {
            printf("MT76: Firmware chunk send failed at offset %lu\n", offset);
            return -1;
        }
        
        // Poll for DMA completion
        uint32_t expected = 0xc0000000 | (chunk_len_aligned << 16);
        if (!mt76_poll(dev, MT_FCE_DMA_LEN | MT_VEND_TYPE_CFG, 0xffffffff, expected)) {
            printf("MT76: Firmware chunk completion timeout at offset %lu\n", offset);
            return -1;
        }
        
        offset += chunk_size;
    }
    
    return 0;
}

int mt76_load_firmware(struct mt76_dev *dev, const uint8_t *fw_data, uint32_t fw_len) {
    const struct mt76_fw_header *hdr;
    uint32_t ilm_len, dlm_len;
    
    printf("MT76: Loading firmware (%lu bytes)\n", fw_len);
    
    // Check if firmware is already loaded (warm reboot case)
    uint32_t dma_addr = mt76_read_register(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG);
    if (dma_addr) {
        printf("MT76: Firmware already loaded, resetting...\n");
        
        // Apply power-on RF patch
        uint32_t val = mt76_read_register(dev, MT_RF_PATCH | MT_VEND_TYPE_CFG);
        mt76_write_register(dev, MT_RF_PATCH | MT_VEND_TYPE_CFG, val & ~BIT(19));
        
        // Load IVB to reset
        if (mt76_load_ivb(dev) < 0) {
            printf("MT76: Reset IVB load failed\n");
            return -1;
        }
        
        // Wait for reset completion (different bit pattern for reset)
        if (!mt76_poll(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG, 0x80000000, 0x80000000)) {
            printf("MT76: Reset timeout\n");
            return -1;
        }
        
        printf("MT76: Firmware reset complete\n");
        return 0;
    }
    
    // Parse firmware header
    if (fw_len < sizeof(struct mt76_fw_header)) {
        printf("MT76: Firmware too small\n");
        return -1;
    }
    
    hdr = (const struct mt76_fw_header *)fw_data;
    ilm_len = hdr->ilm_len;
    dlm_len = hdr->dlm_len;
    
    if (fw_len != sizeof(*hdr) + ilm_len + dlm_len) {
        printf("MT76: Firmware size mismatch (expected %lu, got %lu)\n",
               (uint32_t)(sizeof(*hdr) + ilm_len + dlm_len), fw_len);
        return -1;
    }
    
    printf("MT76: Firmware build: %.16s\n", hdr->build_time);
    printf("MT76: ILM length: %lu bytes\n", ilm_len);
    printf("MT76: DLM length: %lu bytes\n", dlm_len);
    
    printf("MT76: Configuring DMA...\n");
    mt76_write_register(dev, MT_USB_U3DMA_CFG | MT_VEND_TYPE_CFG,
                       MT_USB_DMA_CFG_TX_BULK_EN | MT_USB_DMA_CFG_RX_BULK_EN);
    mt76_write_register(dev, MT_FCE_PSE_CTRL, 0x01);
    mt76_write_register(dev, MT_TX_CPU_FROM_FCE_BASE_PTR, 0x00400230);
    mt76_write_register(dev, MT_TX_CPU_FROM_FCE_MAX_COUNT, 0x01);
    mt76_write_register(dev, MT_TX_CPU_FROM_FCE_CPU_DESC_IDX, 0x01);
    mt76_write_register(dev, MT_FCE_PDMA_GLOBAL_CONF, 0x44);
    mt76_write_register(dev, MT_FCE_SKIP_FS, 0x03);
    
    // Send ILM (Instruction Local Memory)
    printf("MT76: Sending ILM...\n");
    if (mt76_send_firmware_part(dev, MT_FW_ILM_OFFSET, 
                                 fw_data + sizeof(*hdr), ilm_len) < 0) {
        printf("MT76: ILM send failed\n");
        return -1;
    }
    
    // Send DLM (Data Local Memory)
    printf("MT76: Sending DLM...\n");
    if (mt76_send_firmware_part(dev, MT_FW_DLM_OFFSET,
                                 fw_data + sizeof(*hdr) + ilm_len, dlm_len) < 0) {
        printf("MT76: DLM send failed\n");
        return -1;
    }
    
    printf("MT76: Firmware data sent, finalizing...\n");
    mt76_write_register(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG, 0);
    
    printf("MT76: Firmware loaded successfully\n");
    return 0;
}

static void bulk_out_complete_cb(tuh_xfer_t* xfer) {
    if (xfer->result == XFER_RESULT_SUCCESS) {
        printf("MT76: Bulk OUT complete, sent %d bytes\n", xfer->actual_len);
    } else {
        printf("MT76: Bulk OUT failed: %d\n", xfer->result);
    }
}

int mt76_usb_bulk_out(struct mt76_dev *dev, const uint8_t *data, uint16_t len) {
    printf("MT76: USB bulk OUT, len=%d\n", len);
    
    if (len > sizeof(tx_buffer)) {
        printf("MT76: TX packet too large: %d\n", len);
        return -1;
    }
    
    // Copy to static buffer for async transfer
    memcpy(tx_buffer, data, len);
    
    // MT_EP_OUT is 0x08
    #define MT_EP_OUT 0x08
    
    // Create transfer structure
    static tuh_xfer_t xfer_out;
    xfer_out.daddr = dev->dev_addr;
    xfer_out.ep_addr = MT_EP_OUT;
    xfer_out.buflen = len;
    xfer_out.buffer = tx_buffer;
    xfer_out.complete_cb = bulk_out_complete_cb;
    xfer_out.user_data = 0;
    
    // Queue async bulk OUT transfer
    if (!tuh_edpt_xfer(&xfer_out)) {
        printf("MT76: Failed to queue bulk OUT transfer\n");
        return -1;
    }
    
    return 0;
}
