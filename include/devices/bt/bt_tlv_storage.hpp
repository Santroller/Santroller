#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <functional>
#include "btstack_tlv.h"

struct BluetoothTlvData
{
    uint32_t tag;
    uint8_t length;
    uint8_t value[70];
};

class BtTlvStorage
{
public:
    static BtTlvStorage &instance();

    const btstack_tlv_t *btstack_tlv();

    int get_tag(uint32_t tag, uint8_t *buffer, uint32_t buffer_size);
    int store_tag(uint32_t tag, const uint8_t *data, uint32_t data_size);
    void delete_tag(uint32_t tag);

    void set_tag_from_config(uint32_t tag, const uint8_t *data, uint32_t data_size);
    void delete_tags_for_mac(const uint8_t mac[6]);
    // Deletes the LE bonds (le_device_db entries) whose address matches. Only for while btstack isn't
    // running, as it keeps its own index of them
    void delete_le_bonds_if(const std::function<bool(const uint8_t *mac)> &matches);
    void clear();

    void foreach_entry(const std::function<void(uint32_t tag, const uint8_t *data, uint8_t length)> &callback) const;

private:
    BtTlvStorage();
    std::vector<BluetoothTlvData> m_entries;
    btstack_tlv_t m_btstack_tlv;
};

