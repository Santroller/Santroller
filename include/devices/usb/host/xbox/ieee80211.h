#ifndef IEEE80211_H
#define IEEE80211_H

#include <stdint.h>
#include <stdbool.h>

/* Frame control field masks */
#define IEEE80211_FCTL_FTYPE        0x000c
#define IEEE80211_FCTL_STYPE        0x00f0
#define IEEE80211_FCTL_FROMDS       0x0200
#define IEEE80211_FCTL_PROTECTED    0x4000

/* Frame types */
#define IEEE80211_FTYPE_MGMT        0x0000
#define IEEE80211_FTYPE_CTL         0x0004
#define IEEE80211_FTYPE_DATA        0x0008

/* Management frame subtypes */
#define IEEE80211_STYPE_ASSOC_REQ   0x0000
#define IEEE80211_STYPE_ASSOC_RESP  0x0010
#define IEEE80211_STYPE_DISASSOC    0x00a0
#define IEEE80211_STYPE_BEACON      0x0080

/* Data frame subtypes */
#define IEEE80211_STYPE_DATA        0x0000
#define IEEE80211_STYPE_QOS_DATA    0x0080

#define ETH_ALEN 6

struct ieee80211_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];
    uint8_t addr2[ETH_ALEN];
    uint8_t addr3[ETH_ALEN];
    uint16_t seq_ctrl;
} __attribute__((packed));

struct ieee80211_qos_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];
    uint8_t addr2[ETH_ALEN];
    uint8_t addr3[ETH_ALEN];
    uint16_t seq_ctrl;
    uint16_t qos_ctrl;
} __attribute__((packed));

bool ieee80211_parse_frame(const uint8_t *frame_data, uint16_t frame_len);

#endif
