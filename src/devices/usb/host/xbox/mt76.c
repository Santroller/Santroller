#include "mt76.h"
#include "tusb.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include <stdio.h>
#include <string.h>
#include <uzlib.h>

static int mt76_usb_bulk_out_blocking(struct mt76_dev *dev, uint8_t *buffer, uint16_t len) {
    if (!usbh_edpt_claim(dev->dev_addr, MT_EP_OUT)) {
        return -1;
    }

    if (!usbh_edpt_xfer(dev->dev_addr, MT_EP_OUT, buffer, len)) {
        usbh_edpt_release(dev->dev_addr, MT_EP_OUT);
        return -1;
    }

    absolute_time_t deadline = make_timeout_time_ms(1000);
    while (usbh_edpt_busy(dev->dev_addr, MT_EP_OUT)) {
        tuh_task();
        tud_task();
        if (time_reached(deadline)) {
            printf("MT76: Bulk OUT transfer timed out\n");
            tuh_edpt_abort_xfer(dev->dev_addr, MT_EP_OUT);
            return -1;
        }
    }

    return 0;
}

static int mt76_send_firmware_part(struct mt76_dev *dev, uint32_t base_offset,
                                   const uint8_t *data, uint32_t len);

static uint8_t firmware_stream_output[MT_FW_CHUNK_SIZE];
static uint8_t firmware_stream_chunk[MT_FW_CHUNK_SIZE];
static uint8_t firmware_stream_dictionary[32768];

struct mt76_firmware_stream {
    struct mt76_fw_header header;
    uint32_t header_len;
    uint32_t output_len;
    uint32_t section_offset;
    uint32_t section_sent;
    uint32_t chunk_len;
    uint8_t section;
    bool configured;
};

static struct uzlib_uncomp firmware_stream_decompressor;
static struct mt76_firmware_stream firmware_stream;
static uint32_t firmware_stream_expected_len;
static bool firmware_stream_active;

static int mt76_stream_output(struct mt76_dev *dev, struct mt76_firmware_stream *stream,
                              const uint8_t *data, uint32_t len)
{
    while (len > 0)
    {
        if (stream->header_len < sizeof(stream->header))
        {
            uint32_t count = sizeof(stream->header) - stream->header_len;
            if (count > len) count = len;
            memcpy((uint8_t *)&stream->header + stream->header_len, data, count);
            stream->header_len += count;
            stream->output_len += count;
            data += count;
            len -= count;
            if (stream->header_len < sizeof(stream->header)) continue;

            if (stream->header.ilm_len == 0 || stream->header.dlm_len == 0) return -1;
            mt76_write_register(dev, MT_USB_U3DMA_CFG | MT_VEND_TYPE_CFG,
                                MT_USB_DMA_CFG_TX_BULK_EN | MT_USB_DMA_CFG_RX_BULK_EN);
            mt76_write_register(dev, MT_FCE_PSE_CTRL, 0x01);
            mt76_write_register(dev, MT_TX_CPU_FROM_FCE_BASE_PTR, 0x00400230);
            mt76_write_register(dev, MT_TX_CPU_FROM_FCE_MAX_COUNT, 0x01);
            mt76_write_register(dev, MT_TX_CPU_FROM_FCE_CPU_DESC_IDX, 0x01);
            mt76_write_register(dev, MT_FCE_PDMA_GLOBAL_CONF, 0x44);
            mt76_write_register(dev, MT_FCE_SKIP_FS, 0x03);
            stream->configured = true;
        }

        if (stream->section > 1) return -1;
        uint32_t section_len = stream->section == 0 ? stream->header.ilm_len : stream->header.dlm_len;
        if (stream->section_offset > section_len) return -1;

        uint32_t count = len;
        uint32_t remaining = section_len - stream->section_offset;
        if (count > remaining) count = remaining;
        if (count > MT_FW_CHUNK_SIZE - stream->chunk_len) count = MT_FW_CHUNK_SIZE - stream->chunk_len;
        if (count == 0)
        {
            if (stream->section_offset != section_len) return -1;
            stream->section++;
            stream->section_offset = 0;
            stream->section_sent = 0;
            continue;
        }

        memcpy(firmware_stream_chunk + stream->chunk_len, data, count);
        stream->chunk_len += count;
        stream->section_offset += count;
        stream->output_len += count;
        data += count;
        len -= count;

        if (stream->chunk_len == MT_FW_CHUNK_SIZE || stream->section_offset == section_len)
        {
            uint32_t base = stream->section == 0 ? MT_FW_ILM_OFFSET : MT_FW_DLM_OFFSET;
            if (mt76_send_firmware_part(dev, base + stream->section_sent,
                                        firmware_stream_chunk, stream->chunk_len) < 0) return -1;
            stream->section_sent += stream->chunk_len;
            stream->chunk_len = 0;
        }

        if (stream->section_offset == section_len)
        {
            stream->section++;
            stream->section_offset = 0;
            stream->section_sent = 0;
        }
    }
    return 0;
}

int mt76_begin_firmware_compressed(struct mt76_dev *dev, const uint8_t *compressed_data,
                                   uint32_t compressed_len, uint32_t decompressed_len)
{
    if (firmware_stream_active || !compressed_data || compressed_len < 4 ||
        decompressed_len < sizeof(struct mt76_fw_header)) return -1;

    uint32_t dma_addr = mt76_read_register(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG);
    if (dma_addr)
    {
        uint32_t rf_patch = mt76_read_register(dev, MT_RF_PATCH | MT_VEND_TYPE_CFG);
        mt76_write_register(dev, MT_RF_PATCH | MT_VEND_TYPE_CFG, rf_patch & ~BIT(19));

        if (mt76_load_ivb(dev) < 0 ||
            !mt76_poll(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG, 0x80000000, 0x80000000))
        {
            return -1;
        }
    }

    memset(&firmware_stream, 0, sizeof(firmware_stream));
    uzlib_uncompress_init(&firmware_stream_decompressor, firmware_stream_dictionary,
                          sizeof(firmware_stream_dictionary));
    firmware_stream_decompressor.source = compressed_data;
    firmware_stream_decompressor.source_limit = compressed_data + compressed_len - 4;
    if (uzlib_gzip_parse_header(&firmware_stream_decompressor) != TINF_OK) return -1;

    firmware_stream_expected_len = decompressed_len;
    firmware_stream_active = true;
    return 1;
}

int mt76_step_firmware_compressed(struct mt76_dev *dev)
{
    if (!firmware_stream_active) return -1;

    firmware_stream_decompressor.dest_start = firmware_stream_output;
    firmware_stream_decompressor.dest = firmware_stream_output;
    firmware_stream_decompressor.dest_limit = firmware_stream_output + sizeof(firmware_stream_output);
    int result = uzlib_uncompress_chksum(&firmware_stream_decompressor);
    tuh_task();
    tud_task();

    if (mt76_stream_output(dev, &firmware_stream, firmware_stream_output,
                           firmware_stream_decompressor.dest - firmware_stream_output) < 0 ||
        firmware_stream.output_len > firmware_stream_expected_len)
    {
        firmware_stream_active = false;
        return -1;
    }

    if (result == TINF_OK) return 1;
    firmware_stream_active = false;
    if (result != TINF_DONE || !firmware_stream.configured ||
        firmware_stream.output_len != firmware_stream_expected_len ||
        firmware_stream.section != 2 || firmware_stream.chunk_len != 0) return -1;

    mt76_write_register(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG, 0);
    return 0;
}

int mt76_load_firmware_compressed(struct mt76_dev *dev, const uint8_t *compressed_data,
                                  uint32_t compressed_len, uint32_t decompressed_len)
{
    int result = mt76_begin_firmware_compressed(dev, compressed_data, compressed_len, decompressed_len);
    while (result == 1) result = mt76_step_firmware_compressed(dev);
    return result;
}

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
    if (len > sizeof(dev->bulk_buffer) - MT_CMD_HDR_LEN * 2) {
        printf("MT76: Command too large\n");
        return -1;
    }
    
    memcpy(dev->bulk_buffer + MT_CMD_HDR_LEN, data, len);
    
    uint32_t info = MT_MCU_MSG_TYPE_CMD |
                    FIELD_PREP(MT_MCU_MSG_PORT, MT_CPU_TX_PORT) |
                    FIELD_PREP(MT_MCU_MSG_CMD_TYPE, cmd);
    
    uint16_t total_len = len;
    mt76_prep_message(dev->bulk_buffer + MT_CMD_HDR_LEN, &total_len, info);
    
    printf("MT76: Sending command 0x%02X, len=%d (dev_addr=%d, ep=0x%02X)\n", 
           cmd, total_len, dev->dev_addr, MT_EP_OUT);
    
    if (mt76_usb_bulk_out_blocking(dev, dev->bulk_buffer, total_len) < 0) {
        printf("MT76: Bulk transfer failed\n");
        return -1;
    }
    
    return 0;
}

int mt76_send_wlan(struct mt76_dev *dev, const uint8_t *data, uint16_t len) {
    uint32_t info = FIELD_PREP(MT_TXD_INFO_DPORT, MT_WLAN_PORT) |
                    FIELD_PREP(MT_TXD_INFO_QSEL, MT_QSEL_EDCA) |
                    MT_TXD_INFO_WIV |
                    MT_TXD_INFO_80211;
    uint16_t total_len = len;

    if (len > sizeof(dev->bulk_buffer) - MT_CMD_HDR_LEN * 2) {
        printf("MT76: WLAN packet too large\n");
        return -1;
    }

    memcpy(dev->bulk_buffer + MT_CMD_HDR_LEN, data, len);
    mt76_prep_message(dev->bulk_buffer + MT_CMD_HDR_LEN, &total_len, info);

    if (mt76_usb_bulk_out_blocking(dev, dev->bulk_buffer, total_len) < 0) {
        printf("MT76: WLAN transfer failed\n");
        return -1;
    }

    return 0;
}

static int mt76_send_firmware_part(struct mt76_dev *dev, uint32_t base_offset, 
                                    const uint8_t *data, uint32_t len) {
    uint32_t offset = 0;
    
    while (offset < len) {
        uint32_t chunk_size = (len - offset > MT_FW_CHUNK_SIZE) ? 
                              MT_FW_CHUNK_SIZE : (len - offset);
        uint32_t chunk_len_aligned = (chunk_size + 3) & ~3;
        
         printf("MT76: Loading chunk at offset 0x%06lX, size %lu (aligned %lu)\n", 
             (unsigned long)(base_offset + offset), chunk_size, chunk_len_aligned);
        
        // Write DMA registers BEFORE sending data (like xone)
        mt76_write_register(dev, MT_FCE_DMA_ADDR | MT_VEND_TYPE_CFG, 
                           base_offset + offset);
        mt76_write_register(dev, MT_FCE_DMA_LEN | MT_VEND_TYPE_CFG, 
                           chunk_len_aligned << 16);
        
        // Send firmware chunk with MCU message header (cmd=0 for firmware data)
        if (mt76_send_command(dev, data + offset, chunk_size, 0) < 0) {
                 printf("MT76: Firmware chunk send failed at offset 0x%06lX\n",
                     (unsigned long)(base_offset + offset));
            return -1;
        }
        
        // Poll for DMA completion
        uint32_t expected = 0xc0000000 | (chunk_len_aligned << 16);
        if (!mt76_poll(dev, MT_FCE_DMA_LEN | MT_VEND_TYPE_CFG, 0xffffffff, expected)) {
                 printf("MT76: Firmware chunk completion timeout at offset 0x%06lX\n",
                     (unsigned long)(base_offset + offset));
            return -1;
        }
        
        offset += chunk_size;
    }
    
    return 0;
}

int mt76_usb_bulk_out(struct mt76_dev *dev, const uint8_t *data, uint16_t len) {
    printf("MT76: USB bulk OUT, len=%d\n", len);
    
    if (len > sizeof(dev->tx_buffer)) {
        printf("MT76: TX packet too large: %d\n", len);
        return -1;
    }
    
    // Copy to static buffer for async transfer
    memcpy(dev->tx_buffer, data, len);
    
    // MT_EP_OUT is 0x08
    #define MT_EP_OUT 0x08
    
    if (mt76_usb_bulk_out_blocking(dev, dev->tx_buffer, len) < 0) {
        printf("MT76: Bulk OUT transfer failed\n");
        return -1;
    }
    
    return 0;
}
