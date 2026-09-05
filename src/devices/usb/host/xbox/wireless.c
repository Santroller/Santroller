#include "wireless.h"
#include "mt76_defs.h"
#include "ieee80211.h"
#include "mt76.h"
#include "xbox_adapter.h"
#include "gip_crypto.h"
#include <stdio.h>
#include <string.h>

#define PAIRING_TIMEOUT_MS 30000
#define PAIRING_RESPONSE_GRACE_MS 200
uint8_t scratch[1024];
static void wireless_handle_client_lost(struct mt76_dev *dev, const uint8_t *data, uint16_t len);
static void wireless_handle_client_command(struct mt76_dev *dev, const uint8_t *data, uint16_t len, uint8_t wcid, const uint8_t *addr);
static void wireless_handle_disassociation(struct mt76_dev *dev, uint8_t wcid);

static void wireless_drop_client_events(struct mt76_dev *dev, uint8_t wcid, const uint8_t *addr)
{
    uint8_t count = 0;
    uint8_t original_count = (dev->event_queue_head + MT76_EVENT_QUEUE_SIZE - dev->event_queue_tail) % MT76_EVENT_QUEUE_SIZE;
    for (uint8_t i = 0; i < original_count; i++)
    {
        uint8_t index = (dev->event_queue_tail + i) % MT76_EVENT_QUEUE_SIZE;
        struct mt76_wireless_event *event = &dev->event_queue[index];
        bool matches = event->wcid == wcid && event->type != MT76_EVENT_ASSOCIATION;
        if (event->type == MT76_EVENT_ASSOCIATION && addr)
        {
            matches = memcmp(event->addr, addr, MT76_MAC_ADDR_LEN) == 0;
        }
        if (!matches)
        {
            uint8_t destination = (dev->event_queue_tail + count) % MT76_EVENT_QUEUE_SIZE;
            if (destination != index)
            {
                dev->event_queue[destination] = *event;
            }
            count++;
        }
    }
    dev->event_queue_head = (dev->event_queue_tail + count) % MT76_EVENT_QUEUE_SIZE;
}

static bool wireless_queue_event(struct mt76_dev *dev, const struct mt76_wireless_event *event)
{
    uint8_t next = (dev->event_queue_head + 1) % MT76_EVENT_QUEUE_SIZE;
    if (next == dev->event_queue_tail)
    {
        printf("Wireless event queue full\n");
        return false;
    }

    dev->event_queue[dev->event_queue_head] = *event;
    dev->event_queue_head = next;
    return true;
}

static bool wireless_dequeue_event(struct mt76_dev *dev, struct mt76_wireless_event *event)
{
    if (dev->event_queue_tail == dev->event_queue_head)
    {
        return false;
    }

    *event = dev->event_queue[dev->event_queue_tail];
    dev->event_queue_tail = (dev->event_queue_tail + 1) % MT76_EVENT_QUEUE_SIZE;
    return true;
}

static void wireless_queue_association(struct mt76_dev *dev, const uint8_t *addr)
{
    struct mt76_wireless_event event = {.type = MT76_EVENT_ASSOCIATION};
    memcpy(event.addr, addr, sizeof(event.addr));
    wireless_queue_event(dev, &event);
}

static void wireless_queue_client_command(struct mt76_dev *dev, uint8_t command, uint8_t wcid, const uint8_t *addr)
{
    struct mt76_wireless_event event = {
        .type = MT76_EVENT_CLIENT_COMMAND,
        .wcid = wcid,
        .command = command,
    };
    memcpy(event.addr, addr, sizeof(event.addr));
    wireless_queue_event(dev, &event);
}

static void wireless_request_pairing(struct mt76_dev *dev, bool enable)
{
    enum mt76_pairing_state requested_state = enable ? MT76_PAIRING_ENABLING : MT76_PAIRING_DISABLING;
    if (dev->pairing_state == requested_state ||
        (enable && dev->pairing_state == MT76_PAIRING_ACTIVE) ||
        (!enable && dev->pairing_state == MT76_PAIRING_DISABLED))
    {
        return;
    }

    dev->pairing_state = requested_state;
    dev->pending_pairing = enable ? 1 : 0;
}

static void wireless_apply_pairing(struct mt76_dev *dev, bool enable)
{
    bool was_pairing_active = dev->pairing_active;
    enum mt76_pairing_state was_pairing_state = dev->pairing_state;
    dev->pairing_active = enable;

    if (mt76_set_pairing(dev, enable) < 0)
    {
        dev->pairing_active = was_pairing_active;
        dev->pairing_state = was_pairing_state;
        printf("Failed to %s pairing mode\n", enable ? "enable" : "disable");
        return;
    }

    bool has_clients = false;
    for (int i = 0; i < MT76_MAX_CLIENTS; i++)
    {
        if (dev->clients[i].used)
        {
            has_clients = true;
            break;
        }
    }

    mt76_set_led_mode(dev,
                      enable ? MT_LED_BLINK : (has_clients ? MT_LED_ON : MT_LED_OFF));
    if (enable)
    {
        dev->pairing_state = MT76_PAIRING_ACTIVE;
        dev->pairing_start_time = to_ms_since_boot(get_absolute_time());
        dev->pairing_response_sent = false;
        dev->pairing_disable_at = 0;
    }
    else
    {
        dev->pairing_state = MT76_PAIRING_DISABLED;
    }
}

void wireless_task(struct mt76_dev *dev)
{
    struct mt76_wireless_event event;
    if (wireless_dequeue_event(dev, &event))
    {
        switch (event.type)
        {
        case MT76_EVENT_ASSOCIATION:
            wireless_handle_association(dev, event.addr);
            break;
        case MT76_EVENT_DISASSOCIATION:
            wireless_handle_disassociation(dev, event.wcid);
            break;
        case MT76_EVENT_CLIENT_LOST:
            wireless_handle_client_lost(dev, &event.wcid, 1);
            break;
        case MT76_EVENT_CLIENT_COMMAND:
        {
            uint8_t command_data[2] = {XONE_MT_WLAN_RESERVED & 0xFF, event.command};
            wireless_handle_client_command(dev, command_data, sizeof(command_data), event.wcid, event.addr);
            break;
        }
        }
    }

    if (dev->pending_pairing >= 0)
    {
        bool enable = dev->pending_pairing != 0;
        dev->pending_pairing = -1;
        wireless_apply_pairing(dev, enable);
    }

    if (dev->pairing_active)
    {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (dev->pairing_response_sent && dev->pairing_disable_at != 0 &&
            now >= dev->pairing_disable_at)
        {
            wireless_request_pairing(dev, false);
        }
        else if (!dev->pairing_response_sent && now - dev->pairing_start_time >= PAIRING_TIMEOUT_MS)
        {
            printf("Pairing timeout - disabling pairing mode\n");
            wireless_request_pairing(dev, false);
        }
    }
}

static void wireless_process_message(struct mt76_dev *dev, const uint8_t *data, uint16_t len)
{
    uint32_t info = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    uint8_t port = FIELD_GET(MT_RX_FCE_INFO_D_PORT, info);
    uint8_t evt_type = FIELD_GET(MT_RX_FCE_INFO_EVT_TYPE, info);
    if (FIELD_GET(MT_RX_FCE_INFO_CMD_SEQ, info) == 0x01)
    {
        return;
    }

    if (port == MT_CPU_RX_PORT)
    {
        switch (evt_type)
        {
        case XONE_MT_EVT_BUTTON:
            printf("Pairing button pressed on adapter!\n");
            if (dev->pairing_state == MT76_PAIRING_DISABLED)
            {
                wireless_request_pairing(dev, true);
            }
            break;

        case XONE_MT_EVT_PACKET_RX:
            if (len > MT_CMD_HDR_LEN * 2)
            {
                wireless_process_packet(dev, data + MT_CMD_HDR_LEN, len - MT_CMD_HDR_LEN * 2);
            }
            break;

        case XONE_MT_EVT_CLIENT_LOST:
            if (len > MT_CMD_HDR_LEN * 2)
            {
                struct mt76_wireless_event event = {
                    .type = MT76_EVENT_CLIENT_LOST,
                    .wcid = data[MT_CMD_HDR_LEN],
                };
                wireless_queue_event(dev, &event);
            }
            break;

        default:
            break;
        }
    }
    else if (port == MT_WLAN_PORT)
    {
        if (len > MT_CMD_HDR_LEN * 2)
        {
            wireless_process_packet(dev, data + MT_CMD_HDR_LEN, len - MT_CMD_HDR_LEN * 2);
        }
    }
}

void wireless_process_data(struct mt76_dev *dev, tu_edpt_stream_t* stream)
{
    uint8_t data[4];
    
    while (tu_fifo_peek_n(&stream->ff, data, 4) == 4)
    {
        uint32_t info = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        uint16_t msg_len = FIELD_GET(MT_RX_FCE_INFO_LEN, info);
        uint16_t total = MT_CMD_HDR_LEN + ((msg_len + 3) & ~3) + MT_CMD_HDR_LEN;
        if (total > tu_fifo_count(&stream->ff))
        {
            return;
        }
        if (total > sizeof(scratch)) {
            // The message is too large to fit in the scratch buffer.
            printf("Message too large to fit in scratch buffer, skipping. %d\r\n", total);
            tu_fifo_discard_n(&stream->ff, total);
            return;
        }
        tu_fifo_read_n(&stream->ff, scratch, total);
        wireless_process_message(dev, scratch, total);
    }
}

static void wireless_process_frame(struct mt76_dev *dev, const uint8_t *data, uint16_t len)
{
    if (len < sizeof(struct mt76_rxwi) + sizeof(struct ieee80211_hdr))
    {
        return;
    }

    const struct mt76_rxwi *rxwi = (const struct mt76_rxwi *)data;
    data += sizeof(struct mt76_rxwi);
    len -= sizeof(struct mt76_rxwi);

    uint32_t rxinfo = rxwi->rxinfo;
    uint32_t ctl = rxwi->ctl;
    uint16_t mpdu_len = FIELD_GET(MT_RXWI_CTL_MPDU_LEN, ctl);
    uint16_t padding_len = (rxinfo & MT_RXINFO_L2PAD) ? 2 : 0;
    uint16_t header_len = sizeof(struct ieee80211_hdr);

    if (mpdu_len < header_len || header_len + padding_len > len)
    {
        return;
    }

    const struct ieee80211_hdr *hdr = (const struct ieee80211_hdr *)data;
    uint16_t frame_control = hdr->frame_control;
    uint16_t ftype = frame_control & IEEE80211_FCTL_FTYPE;
    uint16_t stype = frame_control & IEEE80211_FCTL_STYPE;

    uint8_t wcid = FIELD_GET(MT_RXWI_CTL_WCID, ctl);
    if (ftype == IEEE80211_FTYPE_DATA && stype == IEEE80211_STYPE_QOS_DATA)
    {
        header_len += 2;
    }

    if (mpdu_len < header_len || header_len + padding_len > len)
    {
        return;
    }

    // The L2 padding sits between the header and payload and is not counted in MPDU_LEN.
    const uint8_t *payload = data + header_len + padding_len;
    uint16_t payload_len = mpdu_len - header_len;

    if (header_len + padding_len + payload_len > len)
    {
        return;
    }

    len = mpdu_len;

    if (ftype == IEEE80211_FTYPE_MGMT)
    {
        if (stype == IEEE80211_STYPE_ASSOC_REQ)
        {
            wireless_queue_association(dev, hdr->addr2);
        }
        else if (stype == IEEE80211_STYPE_DISASSOC)
        {
            struct mt76_wireless_event event = {
                .type = MT76_EVENT_DISASSOCIATION,
                .wcid = wcid,
            };
            wireless_queue_event(dev, &event);
        }
        else if (stype == XONE_MT_WLAN_RESERVED)
        {
            if (payload_len >= 2)
            {
                wireless_queue_client_command(dev, payload[1], wcid, hdr->addr2);
            }
        }
    }
    else if (ftype == IEEE80211_FTYPE_DATA)
    {
        if (stype == IEEE80211_STYPE_QOS_DATA)
        {
            if (wcid > 0 && wcid <= MT76_MAX_CLIENTS)
            {
                // Skip 802.11 QoS header (26 bytes: 24 base + 2 QoS control)
                if (payload_len >= 2)
                {
                    xbox_adapter_process_gip_data(dev, wcid, payload, payload_len);
                }
            }
        }
    }
}

void wireless_process_packet(struct mt76_dev *dev, const uint8_t *data, uint16_t len)
{
    while (len >= sizeof(struct mt76_rxwi) + sizeof(struct ieee80211_hdr))
    {
        const struct mt76_rxwi *rxwi = (const struct mt76_rxwi *)data;
        uint16_t mpdu_len = FIELD_GET(MT_RXWI_CTL_MPDU_LEN, rxwi->ctl);
        uint16_t padding_len = (rxwi->rxinfo & MT_RXINFO_L2PAD) ? 2 : 0;
        uint16_t frame_len = sizeof(struct mt76_rxwi) + mpdu_len + padding_len;
        uint16_t transfer_len = (frame_len + 3) & ~3;

        if (mpdu_len < sizeof(struct ieee80211_hdr) || frame_len > len)
        {
            return;
        }

        wireless_process_frame(dev, data, frame_len);

        if (transfer_len >= len)
        {
            return;
        }

        data += transfer_len;
        len -= transfer_len;
    }
}

static int find_free_wcid(struct mt76_dev *dev)
{
    for (int i = 0; i < MT76_MAX_CLIENTS; i++)
    {
        if (!dev->clients[i].used)
        {
            return i + 1; // WCID is 1-based
        }
    }
    return -1; // No free slots
}

static void remove_client(struct mt76_dev *dev, uint8_t wcid)
{
    if (wcid > 0 && wcid <= MT76_MAX_CLIENTS)
    {
        uint8_t addr[MT76_MAC_ADDR_LEN];
        memcpy(addr, dev->clients[wcid - 1].addr, sizeof(addr));
        wireless_drop_client_events(dev, wcid, addr);
        dev->clients[wcid - 1].used = false;
        memset(dev->clients[wcid - 1].addr, 0, 6);
        memset(dev->clients[wcid - 1].key, 0, sizeof(dev->clients[wcid - 1].key));
        dev->clients[wcid - 1].encryption_enabled = false;
        xbox_remove_controller(dev, wcid - 1);
        printf("Client WCID %d removed\n", wcid);
    }
}

static void wireless_handle_client_command(struct mt76_dev *dev, const uint8_t *data, uint16_t len, uint8_t wcid, const uint8_t *addr)
{
    if (len < 2 || data[0] != (XONE_MT_WLAN_RESERVED & 0xFF))
    {
        return;
    }

    uint8_t cmd = data[1];

    switch (cmd)
    {
    case XONE_MT_CLIENT_PAIR_REQ:
        if (dev->pairing_state != MT76_PAIRING_ACTIVE || dev->pairing_response_sent)
        {
            break;
        }

        printf("Pairing request from %02X:%02X:%02X:%02X:%02X:%02X\n",
               addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

        if (mt76_send_pair_response(dev, addr) < 0)
        {
            break;
        }

        dev->pairing_response_sent = true;
        dev->pairing_disable_at = to_ms_since_boot(get_absolute_time()) + PAIRING_RESPONSE_GRACE_MS;

        printf("Pairing response sent, pairing mode will disable\n");
        break;

    case XONE_MT_CLIENT_ENABLE_ENCRYPTION:
        printf("Enable encryption request from WCID %d\n", wcid);

        if (wcid > 0 && wcid <= MT76_MAX_CLIENTS && dev->clients[wcid - 1].used)
        {
            gip_crypto_random_bytes(dev->clients[wcid - 1].key, 16);

            if (mt76_set_client_key(dev, wcid, dev->clients[wcid - 1].key, 16) < 0)
            {
                printf("Failed to enable encryption for WCID %d\n", wcid);
                break;
            }

            uint8_t ack_data[] = {0x00, 0x00};
            if (mt76_send_client_command(dev, wcid, addr,
                                         XONE_MT_CLIENT_ENABLE_ENCRYPTION,
                                         ack_data, sizeof(ack_data)) < 0)
            {
                printf("Failed to acknowledge encryption for WCID %d\n", wcid);
                break;
            }

            dev->clients[wcid - 1].encryption_enabled = true;
            printf("Encryption enabled for WCID %d\n", wcid);
        }
        else
        {
            printf("Invalid WCID %d for encryption\n", wcid);
        }
        break;

    default:
        // printf("Unknown client command: 0x%02X\n", cmd);
        break;
    }
}

static void wireless_handle_disassociation(struct mt76_dev *dev, uint8_t wcid)
{
    printf("Disassociation from WCID %d\n", wcid);

    if (wcid == 0 || wcid > MT76_MAX_CLIENTS)
    {
        return;
    }

    mt76_remove_client(dev, wcid);

    remove_client(dev, wcid);

    for (int i = 0; i < MT76_MAX_CLIENTS; i++)
    {
        if (dev->clients[i].used)
        {
            return;
        }
    }

    mt76_set_led_mode(dev, MT_LED_OFF);
}

static void wireless_handle_client_lost(struct mt76_dev *dev, const uint8_t *data, uint16_t len)
{
    if (len < 1)
    {
        return;
    }

    uint8_t wcid = data[0];
    if (wcid == 0 || wcid > MT76_MAX_CLIENTS)
    {
        return;
    }

    printf("Client lost event: WCID %d\n", wcid);
    mt76_remove_client(dev, wcid);
    remove_client(dev, wcid);
}

void wireless_handle_association(struct mt76_dev *dev, const uint8_t *addr)
{
    printf("Association request from: %02X:%02X:%02X:%02X:%02X:%02X\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    for (int i = 0; i < MT76_MAX_CLIENTS; i++)
    {
        if (memcmp(dev->clients[i].addr, addr, 6) == 0)
        {
            printf("Client already seen!\n");
            return;
        }
    }
    int wcid = find_free_wcid(dev);
    if (wcid < 0)
    {
        printf("No free WCID slots!\n");
        return;
    }

    printf("Allocated WCID %d for controller\n", wcid);

    dev->clients[wcid - 1].used = true;
    memcpy(dev->clients[wcid - 1].addr, addr, 6);

    xbox_create_controller(dev, wcid - 1);

    xbox_controller_t *controller = xbox_get_controller(dev, wcid - 1);
    if (controller)
    {
        memcpy(controller->mac_addr, addr, 6);
    }

    if (mt76_add_client(dev, wcid, addr) < 0)
    {
        remove_client(dev, (uint8_t)wcid);
        printf("Failed to add client to MT76\n");
        return;
    }

    if (!dev->pairing_active)
    {
        mt76_set_led_mode(dev, MT_LED_ON);
    }

    printf("Controller associated successfully with WCID %d\n", wcid);
}
