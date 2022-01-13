#include "usb_host.h"

#include <stdio.h>
#include <string.h>
#include <tusb.h>

#include "crc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "helpers.h"
#include "lib_main.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pio_bitstuff.pio.h"
#include "pio_keepalive.pio.h"
#include "pio_serialiser.pio.h"
#include "usb.h"
#include "usb/std_descriptors.h"
#include "usb_host_serialiser.h"

int dma_chan_read;
int dma_chan_write;
int dma_chan_keepalive;
int dma_chan_bitstuff;

float div;

PIO pio_usb;
PIO pio_bitstuff;
uint sm_usb;
uint sm_keepalive;
uint sm_bitstuff;
uint offset_usb;
uint offset_keepalive;
uint offset_bitstuff;

uint8_t maxPacket = 8;
uint32_t currentFrame = 0;

bool full_speed = false;
volatile bool initialised = false;
volatile bool waiting_for_keepalive = false;

TUSB_Descriptor_Device_t pluggedInDescriptor;

struct repeating_timer timer;

state_t state_keepalive;
state_t state_usb;

bool keepAliveThread(struct repeating_timer* t);
void initKeepalive(void);
void sendOut(uint8_t address, uint8_t endpoint, uint len, uint8_t* d);
void initialise_device(void);

void init_usb_host(void) {
    pio_usb = pio0;
    pio_bitstuff = pio1;
    sm_usb = pio_claim_unused_sm(pio_usb, true);
    sm_keepalive = pio_claim_unused_sm(pio_usb, true);
    sm_bitstuff = pio_claim_unused_sm(pio_bitstuff, true);

    offset_usb = pio_add_program(pio_usb, &pio_serialiser_program);
    offset_keepalive = pio_add_program(pio_usb, &pio_keepalive_program);
    offset_bitstuff = pio_add_program(pio_bitstuff, &pio_bitstuff_program);

    dma_chan_write = dma_claim_unused_channel(true);
    dma_chan_read = dma_claim_unused_channel(true);
    dma_chan_keepalive = dma_claim_unused_channel(true);
    dma_chan_bitstuff = dma_claim_unused_channel(true);

    dma_channel_config dma_config_write = dma_channel_get_default_config(dma_chan_write);
    channel_config_set_transfer_data_size(&dma_config_write, DMA_SIZE_8);
    channel_config_set_write_increment(&dma_config_write, false);
    channel_config_set_read_increment(&dma_config_write, true);
    channel_config_set_dreq(&dma_config_write, pio_get_dreq(pio_usb, sm_usb, true));

    dma_channel_configure(
        dma_chan_write,
        &dma_config_write,
        &pio_usb->txf[sm_usb],
        state_usb.buffer,
        0,
        false);

    dma_channel_config dma_config_read = dma_channel_get_default_config(dma_chan_read);
    channel_config_set_transfer_data_size(&dma_config_read, DMA_SIZE_8);
    channel_config_set_write_increment(&dma_config_read, true);
    channel_config_set_read_increment(&dma_config_read, false);
    channel_config_set_dreq(&dma_config_read, pio_get_dreq(pio_bitstuff, sm_bitstuff, false));
    dma_channel_configure(
        dma_chan_read,
        &dma_config_read,
        NULL,
        ((uint8_t*)&pio_bitstuff->rxf[sm_bitstuff]) + 3,
        1,
        false);

    dma_channel_config dma_config_keepalive = dma_channel_get_default_config(dma_chan_keepalive);
    channel_config_set_transfer_data_size(&dma_config_keepalive, DMA_SIZE_32);
    channel_config_set_write_increment(&dma_config_keepalive, false);
    channel_config_set_read_increment(&dma_config_keepalive, true);
    channel_config_set_dreq(&dma_config_keepalive, pio_get_dreq(pio_usb, sm_keepalive, true));
    dma_channel_configure(
        dma_chan_keepalive,
        &dma_config_keepalive,
        &pio_usb->txf[sm_keepalive],  // Write address (only need to set this once)
        state_keepalive.buffer,
        3,
        false);

    dma_channel_config dma_config_bitstuff = dma_channel_get_default_config(dma_chan_bitstuff);
    channel_config_set_transfer_data_size(&dma_config_bitstuff, DMA_SIZE_8);
    channel_config_set_write_increment(&dma_config_bitstuff, false);
    channel_config_set_read_increment(&dma_config_bitstuff, false);
    channel_config_set_dreq(&dma_config_bitstuff, pio_get_dreq(pio_usb, sm_usb, false));
    dma_channel_configure(
        dma_chan_bitstuff,
        &dma_config_bitstuff,
        ((uint8_t*)&pio_bitstuff->txf[sm_bitstuff]) + 3,  // write addr
        ((uint8_t*)&pio_usb->rxf[sm_usb]) + 3,            // read addr
        3,
        false);
}

void host_reset() {
    pio_sm_set_enabled(pio_usb, sm_usb, false);
    pio_sm_set_enabled(pio_usb, sm_keepalive, false);
    gpio_init(USB_DM_PIN);
    gpio_init(USB_DP_PIN);
    gpio_set_dir_out_masked(_BV(USB_DM_PIN) | _BV(USB_DP_PIN));
    gpio_put_masked(_BV(USB_DM_PIN) | _BV(USB_DP_PIN), 0);
    sleep_ms(20);
    pio_bitstuff_program_init(pio_bitstuff, sm_bitstuff, offset_bitstuff);
    pio_serialiser_program_init(pio_usb, sm_usb, offset_usb, USB_FIRST_PIN, div);
    uint32_t keepalive_delay = ((clock_get_hz(clk_sys) / div) / 10000);
    pio_keepalive_program_init(pio_usb, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
}

void initialise_device(void) {
    tusb_control_request_t req = {.bmRequestType = 0x80, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = sizeof(pluggedInDescriptor)};
    send_control_request(0x00, 0x00, req, true, (uint8_t*)&pluggedInDescriptor);
    maxPacket = pluggedInDescriptor.Endpoint0Size;
    printf("Max Packet: %d\n", maxPacket);
    host_reset();
    printf("Setting ID\n");
    tusb_control_request_t req2 = {.bmRequestType = 0x00, .bRequest = 0x05, .wValue = 13, .wIndex = 0x0000, .wLength = 0};
    send_control_request(0x00, 0x00, req2, false, NULL);
    printf("Setting config\n");
    tusb_control_request_t req3 = {.bmRequestType = 0x00, .bRequest = 0x09, .wValue = 01, .wIndex = 0x0000, .wLength = 0};
    send_control_request(13, 0x00, req3, false, NULL);
    tusb_control_request_t req4 = {.bmRequestType = 0x80, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = sizeof(pluggedInDescriptor)};
    send_control_request(13, 0x00, req4, false, (uint8_t*)&pluggedInDescriptor);
    printf("Detected Device: %x %x\n", pluggedInDescriptor.VendorID, pluggedInDescriptor.ProductID);
}

void set_xinput_led(int led) {
    uint8_t data2[] = {0x01, 0x03, led};
    sendOut(13, 0x02, sizeof(data2), data2);
}
bool is_xinput(void) {
    return pluggedInDescriptor.Class == 0xFF && pluggedInDescriptor.Protocol == 0xFF && pluggedInDescriptor.SubClass == 0xFF;
}

void tick_usb_host(void) {
    if (!initialised) {
        gpio_init(USB_DM_PIN);
        gpio_init(USB_DP_PIN);
        if (!gpio_get(USB_DM_PIN) && !gpio_get(USB_DP_PIN)) {
            return;
        }
        sleep_ms(2);
        if (gpio_get(USB_DM_PIN)) {
            div = (clock_get_hz(clk_sys) / ((1500000.0f))) / 10;
            full_speed = false;
        } else if (gpio_get(USB_DP_PIN)) {
            div = (clock_get_hz(clk_sys) / ((12000000.0f))) / 10;
            full_speed = true;
        }
        initialised = true;
        printf("Speed: %d %f %d\n", full_speed, div, clock_get_hz(clk_sys));
        multicore_launch_core1(initKeepalive);
        // Reset
        host_reset();

        initialise_device();
        if (is_xinput()) {
            set_xinput_led(0x0A);
        }
        // while (true) {
        //     tusb_control_request_t req = {.bmRequestType = 0x80, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = sizeof(pluggedInDescriptor)};
        //     send_control_request(13, 0x00, req, false, (uint8_t*)&pluggedInDescriptor);
        //     printf("%x %x\n", pluggedInDescriptor.ProductID, pluggedInDescriptor.VendorID);
        // }
        tud_disconnect();
        tud_connect();
    }
}

bool keepaliveLoop(struct repeating_timer* t) {
    if (full_speed) {
        // Increment current frame and then append its crc5
        currentFrame = currentFrame + 1;
        if (currentFrame == 1 << 11) {
            currentFrame = 0;
        }
        reset_state(&state_keepalive);
        sync(&state_keepalive);
        sendPID(SOF, &state_keepalive);
        reset_crc(&state_keepalive);
        sendData(currentFrame, 11, &state_keepalive, false);
        sendCRC5(&state_keepalive);
        EOP(&state_keepalive);
        int remaining = (32 - (state_keepalive.id % 32)) / 2;
        for (int i = 0; i < remaining; i++) {
            J(&state_keepalive);
        }
        dma_channel_transfer_from_buffer_now(dma_chan_keepalive, state_keepalive.buffer, state_keepalive.id / 32);
    } else {
        // EOP (SE0 SE0 J), except the J is implicit as the line falls back to J anyways
        reset_state(&state_keepalive);
        SE0(&state_keepalive);
        SE0(&state_keepalive);
        dma_channel_transfer_from_buffer_now(dma_chan_keepalive, state_keepalive.buffer, state_keepalive.id / 32);
    }
    waiting_for_keepalive = false;
    return true;
}

void initKeepalive() {
    uint32_t keepalive_delay = ((clock_get_hz(clk_sys) / div) / 10000);
    pio_keepalive_program_init(pio_usb, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
    add_repeating_timer_us(800, keepaliveLoop, NULL, &timer);
}
TUSB_Descriptor_Device_t getPluggedInDescriptor(void) {
    return pluggedInDescriptor;
}

bool transfer(uint8_t address, state_t* state, uint8_t* out) {
    waiting_for_keepalive = true;
    while (waiting_for_keepalive) {
    }
usb_transfer_start:
    uint current_read = 0;
    bool wasData0 = true;
    uint test = false;
    for (int p = 0; p < state->current_packet;) {
        PacketAction_t action = state->packet_actions[p];
        uint8_t read_pkt = state->packetresplens[p];
        uint8_t data_read[read_pkt];
        memset(data_read, 0, read_pkt);
        bool wrong = true;
        uint_fast16_t crc = 0xffff;
        uint_fast16_t crc_calc;
        uint8_t* cur = data_read + 2;
        uint8_t cnt = read_pkt - 4;
        uint8_t expected_pid = wasData0 ? DATA1 : DATA0;
        bool timeout = false;
        unsigned long m = micros();
        pio_sm_clear_fifos(pio_bitstuff, sm_bitstuff);
        pio_sm_restart(pio_bitstuff, sm_bitstuff);
        pio_sm_set_enabled(pio_usb, sm_usb, false);
        pio_sm_restart(pio_usb, sm_usb);
        pio_sm_clear_fifos(pio_usb, sm_usb);
        pio_sm_exec(pio_usb, sm_usb, pio_encode_jmp(offset_usb));
        pio_sm_set_enabled(pio_usb, sm_usb, true);
        dma_channel_set_trans_count(dma_chan_bitstuff, read_pkt + 1, true);
        dma_channel_transfer_from_buffer_now(dma_chan_write, state->packets[p], state->packetlens[p]);
        dma_channel_transfer_to_buffer_now(dma_chan_read, data_read, read_pkt);
        // With this new method of LAST_ACK_END, i suspect we could clean up a lot of this weirdness
        if (action == LAST_ACK_END) {
            return;
        }
        while (dma_channel_is_busy(dma_chan_read)) {
            if (cnt && cur < dma_hw->ch[dma_chan_read].write_addr) {
                crc = (crc_table[(crc ^ *cur++) & 0xff] ^ (crc >> 8)) & 0xffff;
                cnt--;
            }
            if ((micros() - m) > 1000) {
                goto usb_transfer_start;
            }
            if (data_read[1] == NAK || data_read[1] == STALL) {
                break;
            }
        }
        if (action == NEXT_ACK_DATA) {
            crc_calc = (data_read[read_pkt - 2] | data_read[read_pkt - 1] << 8) & 0x7ff;
            crc = (crc ^ 0xffff) & 0x7ff;
            uint8_t pid = data_read[1];
            // If the PIDs don't match up, then we are a packet behind and just need to acknowledge whatever is there
            // Otherwise, we want to make sure the CRC is correct before we ack
            if (p && (crc == crc_calc || pid != expected_pid)) {
                pio_sm_restart(pio_usb, sm_usb);
                pio_sm_exec(pio_usb, sm_usb, pio_encode_set(pio_y, 0));
                dma_channel_transfer_from_buffer_now(dma_chan_write, state->packets[p + 1], state->packetlens[p + 1]);
                dma_channel_wait_for_finish_blocking(dma_chan_write);

                if (pid == expected_pid && crc == crc_calc) {
                    // printf("Done\n");
                    memcpy(out + current_read, data_read + 2, maxPacket);
                    current_read += maxPacket;
                    wasData0 = !wasData0;
                    p += 2;  // Skip ACK packet as it has already been transmitted.
                    sleep_us(20);
                    continue;
                }
            }
        }
        uint8_t sync_data = data_read[0] & 0xff;
        uint8_t pid = data_read[1] & 0xff;
        if (sync_data == 128) {
            if (pid == DATA1 || pid == DATA0) {
                uint16_t crc_calc = (data_read[read_pkt - 1] << 8 | data_read[read_pkt - 2]) & 0x7ff;
                crc = crc_16(data_read + 2, read_pkt - 4) & 0x7ff;
                if (crc != crc_calc) {
                    sleep_us(150);
                    continue;
                }
            }
            if (pid == STALL) {
                return false;
            } else if (action == STANDARD_ACK) {
                if (pid == DATA1 || pid == DATA0) {
                    memcpy(out + current_read, data_read + 2, read_pkt - 4);
                    current_read += maxPacket;
                    sleep_us(20);
                    continue;
                } else if (pid == ACK) {
                    p++;
                }
            } else if (action == ABORTED_ACK && (pid == DATA1 || pid == DATA0)) {
                // For reading maxPacketLength, we actually just read the first 8 bytes and then do a reset, so we don't care about finishing up the read
                // Don't copy the PID or SYNC
                memcpy(out + current_read, data_read + 2, read_pkt - 4);
                current_read += maxPacket;
                return true;
            }
        }
        // TODO: is there a reason why this has to be so bloody high? should work fine at 20 or even lower in theory?
        sleep_us(150);
    }
    return true;
}

void sendOut(uint8_t address, uint8_t endpoint, uint len, uint8_t* data) {
    sync(&state_usb);
    sendPID(OUT, &state_usb);
    reset_crc(&state_usb);
    sendAddress(address, &state_usb);
    sendNibble(endpoint, &state_usb);
    sendCRC5(&state_usb);
    EOP(&state_usb);
    J(&state_usb);
    J(&state_usb);
    sync(&state_usb);
    sendPID(DATA0, &state_usb);
    reset_crc(&state_usb);
    for (int i = 0; i < len; i++) {
        sendByte(*data++, &state_usb);
    }
    for (int i = len; i < 8; i++) {
        sendByte(0, &state_usb);
    }
    sendCRC16(&state_usb);
    EOP(&state_usb);
    commit_packet(&state_usb, STANDARD_ACK, 19);
    transfer(address, &state_usb, NULL);
}

bool send_control_request(uint8_t address, uint8_t endpoint, const tusb_control_request_t request, bool terminateEarly, uint8_t* d) {
    reset_state(&state_usb);
    state_usb.current_packet = 0;
    sync(&state_usb);
    sendPID(SETUP, &state_usb);
    reset_crc(&state_usb);
    sendAddress(address, &state_usb);
    sendNibble(endpoint, &state_usb);
    sendCRC5(&state_usb);
    EOP(&state_usb);
    J(&state_usb);
    sync(&state_usb);
    sendPID(DATA0, &state_usb);
    reset_crc(&state_usb);
    uint8_t* data = (uint8_t*)&request;
    for (int i = 0; i < sizeof(tusb_control_request_t); i++) {
        sendByte(*data++, &state_usb);
    }
    sendCRC16(&state_usb);
    EOP(&state_usb);
    commit_packet(&state_usb, STANDARD_ACK, 19);
    if (request.bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if (request.wLength) {
            uint len = request.wLength / maxPacket;
            if (request.wLength % maxPacket) {
                len++;
            }
            bool data1 = true;
            uint8_t* data = (uint8_t*)d;
            for (int i = 0; i < len; i++) {
                sync(&state_usb);
                sendPID(OUT, &state_usb);
                reset_crc(&state_usb);
                sendAddress(address, &state_usb);
                sendNibble(endpoint, &state_usb);
                sendCRC5(&state_usb);
                EOP(&state_usb);
                J(&state_usb);
                sync(&state_usb);
                sendPID(data1 ? DATA1 : DATA0, &state_usb);
                reset_crc(&state_usb);
                uint l = maxPacket;
                if (i == len - 1) {
                    l = (request.wLength % maxPacket);
                }
                for (int i2 = 0; i2 < l; i2++) {
                    sendByte(*data++, &state_usb);
                }
                sendCRC16(&state_usb);
                EOP(&state_usb);
                commit_packet(&state_usb, STANDARD_ACK, 19);
                data1 = !data1;
            }
        }
        sync(&state_usb);
        sendPID(IN, &state_usb);
        reset_crc(&state_usb);
        sendAddress(address, &state_usb);
        sendNibble(endpoint, &state_usb);
        sendCRC5(&state_usb);
        EOP(&state_usb);
        // Since we don't have the time to actually parse DATA0 before responding with an ACK, we can just delay by writing data, and then respond on time
        for (int i = 0; i < 8 * 5; i++) {
            J(&state_usb);
        }
        sync(&state_usb);
        sendPID(ACK, &state_usb);
        EOP(&state_usb);
        commit_packet(&state_usb, LAST_ACK_END, 0);
    } else {
        if (request.wLength) {
            uint len = request.wLength / maxPacket;
            if (request.wLength % maxPacket) {
                len++;
            }
            for (int i = 0; i < len; i++) {
                sync(&state_usb);
                sendPID(IN, &state_usb);
                reset_crc(&state_usb);
                sendAddress(address, &state_usb);
                sendNibble(endpoint, &state_usb);
                sendCRC5(&state_usb);
                EOP(&state_usb);
                // maxPacket is expressed in bytes not bits
                uint l = maxPacket * 8;
                if (i == len - 1) {
                    l = (request.wLength % maxPacket) * 8;
                }
                l = 8 + 8 + 16 + l;
                if (terminateEarly) {
                    commit_packet(&state_usb, ABORTED_ACK, l);
                    break;
                } else {
                    commit_packet(&state_usb, NEXT_ACK_DATA, l);
                    sync(&state_usb);
                    sendPID(ACK, &state_usb);
                    EOP(&state_usb);
                    commit_packet(&state_usb, STANDARD_ACK, 0);
                }
            }
            memset(d, 0, request.wLength);
        }
        // If we are working out what size maxPacket is, we need to skip ACKs as we don't know how long to wait and are just gonna reset anyways
        if (!terminateEarly) {
            sync(&state_usb);
            sendPID(OUT, &state_usb);
            reset_crc(&state_usb);
            sendAddress(address, &state_usb);
            sendNibble(endpoint, &state_usb);
            sendCRC5(&state_usb);
            EOP(&state_usb);
            sync(&state_usb);
            sendPID(DATA1, &state_usb);
            reset_crc(&state_usb);
            sendCRC16(&state_usb);
            EOP(&state_usb);
            commit_packet(&state_usb, STANDARD_ACK, 19);
        }
    }
    return transfer(address, &state_usb, d);
}