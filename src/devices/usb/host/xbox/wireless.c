#include "wireless.h"
#include "mt76_defs.h"
#include "ieee80211.h"
#include "mt76.h"
#include "xbox_adapter.h"
#include "gip_crypto.h"
#include <stdio.h>
#include <string.h>

// MT76 event types (from xone)
#define XONE_MT_EVT_BUTTON      0x01
#define XONE_MT_EVT_PACKET_RX   0x02
#define XONE_MT_EVT_CLIENT_LOST 0x03

// Client command types
#define XONE_MT_WLAN_RESERVED       0x00d0
#define XONE_MT_CLIENT_PAIR_REQ     0x01
#define XONE_MT_CLIENT_ENABLE_ENCRYPTION 0x02

#define MAX_CLIENTS 8
static struct {
    bool used;
    uint8_t addr[6];
    uint8_t key[16];  // Encryption key
    bool encryption_enabled;
} clients[MAX_CLIENTS];

#define PAIRING_TIMEOUT_MS 30000
static uint32_t pairing_start_time = 0;
static bool pairing_active = false;

static void wireless_handle_client_lost(const uint8_t *data, uint16_t len);
static void wireless_handle_client_command(const uint8_t *data, uint16_t len, uint8_t wcid, const uint8_t *addr);
static void wireless_handle_disassociation(uint8_t wcid);

void wireless_task(void) {
    if (pairing_active) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - pairing_start_time >= PAIRING_TIMEOUT_MS) {
            printf("Pairing timeout - disabling pairing mode\n");
            mt76_set_pairing(&mt76_device, false);
            pairing_active = false;
        }
    }
}

void wireless_process_data(const uint8_t *data, uint16_t len) {
    if (len < 8) {
        return;  // Too short for RX info header
    }
    
    uint32_t info = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    uint8_t port = (info >> 27) & 0x07;  // MT_RX_FCE_INFO_D_PORT
    uint8_t evt_type = (info >> 16) & 0xFF;  // MT_RX_FCE_INFO_EVT_TYPE
    
    printf("Wireless RX: port=%d, evt_type=0x%02X, len=%d\n", port, evt_type, len);
    
    if (port == 3) {  // MT_CPU_RX_PORT
        switch (evt_type) {
            case XONE_MT_EVT_BUTTON:
                printf("Pairing button pressed on adapter!\n");
                mt76_set_pairing(&mt76_device, true);
                pairing_active = true;
                pairing_start_time = to_ms_since_boot(get_absolute_time());
                printf("Pairing mode enabled - will timeout in 30 seconds\n");
                break;
                
            case XONE_MT_EVT_PACKET_RX:
                printf("Wireless packet received\n");
                wireless_process_packet(data, len);
                break;
                
            case XONE_MT_EVT_CLIENT_LOST:
                if (len > 8) {
                    wireless_handle_client_lost(data + 8, len - 8);
                }
                break;
                
            default:
                printf("Unknown event type: 0x%02X\n", evt_type);
                break;
        }
    } else {
        wireless_process_packet(data, len);
    }
}

void wireless_process_packet(const uint8_t *data, uint16_t len) {
    if (len < sizeof(struct mt76_rxwi) + sizeof(struct ieee80211_hdr)) {
        return;
    }
    
    const struct mt76_rxwi *rxwi = (const struct mt76_rxwi *)data;
    data += sizeof(struct mt76_rxwi);
    len -= sizeof(struct mt76_rxwi);
    
    const struct ieee80211_hdr *hdr = (const struct ieee80211_hdr *)data;
    uint16_t frame_control = hdr->frame_control;
    uint16_t ftype = frame_control & IEEE80211_FCTL_FTYPE;
    uint16_t stype = frame_control & IEEE80211_FCTL_STYPE;
    
    printf("802.11 frame: type=0x%04X, ftype=0x%04X, stype=0x%04X\n", 
           frame_control, ftype, stype);
    
    uint32_t ctl = rxwi->ctl;
    uint8_t wcid = (ctl >> 16) & 0xFF;  // MT_RXWI_CTL_WCID
    
    if (ftype == IEEE80211_FTYPE_MGMT) {
        if (stype == IEEE80211_STYPE_ASSOC_REQ) {
            printf("Association request from: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
                   hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
            wireless_handle_association(hdr->addr2);
        } else if (stype == IEEE80211_STYPE_DISASSOC) {
            wireless_handle_disassociation(wcid);
        } else if (stype == XONE_MT_WLAN_RESERVED) {
            const uint8_t *cmd_data = data + sizeof(struct ieee80211_hdr);
            uint16_t cmd_len = len - sizeof(struct ieee80211_hdr);
            wireless_handle_client_command(cmd_data, cmd_len, wcid, hdr->addr2);
        }
    }
    else if (ftype == IEEE80211_FTYPE_DATA) {
        if (stype == IEEE80211_STYPE_QOS_DATA) {
            if (wcid > 0 && wcid <= MAX_CLIENTS) {
                printf("QoS data from WCID %d, len=%d\n", wcid, len);
                
                // Skip 802.11 QoS header (26 bytes: 24 base + 2 QoS control)
                if (len >= 26) {
                    const uint8_t *gip_data = data + 26;
                    uint16_t gip_len = len - 26;
                    
                    xbox_adapter_process_gip_data(gip_data, gip_len);
                }
            }
        }
    }
}

static int find_free_wcid(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].used) {
            return i + 1;  // WCID is 1-based
        }
    }
    return -1;  // No free slots
}

static void remove_client(uint8_t wcid) {
    if (wcid > 0 && wcid <= MAX_CLIENTS) {
        clients[wcid - 1].used = false;
        memset(clients[wcid - 1].addr, 0, 6);
        printf("Client WCID %d removed\n", wcid);
    }
}

static void wireless_handle_client_command(const uint8_t *data, uint16_t len, uint8_t wcid, const uint8_t *addr) {
    if (len < 2 || data[0] != (XONE_MT_WLAN_RESERVED & 0xFF)) {
        return;
    }
    
    uint8_t cmd = data[1];
    
    switch (cmd) {
        case XONE_MT_CLIENT_PAIR_REQ:
            printf("Pairing request from %02X:%02X:%02X:%02X:%02X:%02X\n",
                   addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
            
            mt76_send_pair_response(&mt76_device, addr);
            
            mt76_set_pairing(&mt76_device, false);
            pairing_active = false;
            
            printf("Pairing response sent, pairing mode disabled\n");
            break;
            
        case XONE_MT_CLIENT_ENABLE_ENCRYPTION:
            printf("Enable encryption request from WCID %d\n", wcid);
            
            if (wcid > 0 && wcid <= MAX_CLIENTS && clients[wcid - 1].used) {
                gip_crypto_random_bytes(clients[wcid - 1].key, 16);
                
                mt76_set_client_key(&mt76_device, wcid, clients[wcid - 1].key, 16);
                
                uint8_t ack_data[] = { 0x00, 0x00 };
                mt76_send_client_command(&mt76_device, wcid, addr, 
                                        XONE_MT_CLIENT_ENABLE_ENCRYPTION, 
                                        ack_data, sizeof(ack_data));
                
                clients[wcid - 1].encryption_enabled = true;
                printf("Encryption enabled for WCID %d\n", wcid);
            } else {
                printf("Invalid WCID %d for encryption\n", wcid);
            }
            break;
            
        default:
            printf("Unknown client command: 0x%02X\n", cmd);
            break;
    }
}

static void wireless_handle_disassociation(uint8_t wcid) {
    printf("Disassociation from WCID %d\n", wcid);
    
    mt76_remove_client(&mt76_device, wcid);
    
    remove_client(wcid);
}

static void wireless_handle_client_lost(const uint8_t *data, uint16_t len) {
    if (len < 1) {
        return;
    }
    
    uint8_t wcid = data[0];
    printf("Client lost event: WCID %d\n", wcid);
    remove_client(wcid);
}

void wireless_handle_association(const uint8_t *addr) {
    printf("Association request from: %02X:%02X:%02X:%02X:%02X:%02X\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    
    int wcid = find_free_wcid();
    if (wcid < 0) {
        printf("No free WCID slots!\n");
        return;
    }
    
    printf("Allocated WCID %d for controller\n", wcid);
    
    clients[wcid - 1].used = true;
    memcpy(clients[wcid - 1].addr, addr, 6);
    
    xbox_controller_t *controller = xbox_get_controller(wcid - 1);
    if (controller) {
        memcpy(controller->mac_addr, addr, 6);
    }
    
    if (mt76_add_client(&mt76_device, wcid, addr) < 0) {
        printf("Failed to add client to MT76\n");
        return;
    }
    
    printf("Controller associated successfully with WCID %d\n", wcid);
}
