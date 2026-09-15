#include "devices/bt/bt_tlv_storage.hpp"
#include <cstring>
#include <algorithm>
#include "config/config.hpp"
#include "config/device_factory.hpp"

namespace
{
    int tlv_get_tag_wrapper(void *context, uint32_t tag, uint8_t *buffer, uint32_t buffer_size)
    {
        auto *storage = static_cast<BtTlvStorage *>(context);
        return storage ? storage->get_tag(tag, buffer, buffer_size) : 0;
    }

    int tlv_store_tag_wrapper(void *context, uint32_t tag, const uint8_t *data, uint32_t data_size)
    {
        auto *storage = static_cast<BtTlvStorage *>(context);
        return storage ? storage->store_tag(tag, data, data_size) : -1;
    }

    void tlv_delete_tag_wrapper(void *context, uint32_t tag)
    {
        auto *storage = static_cast<BtTlvStorage *>(context);
        if (storage)
        {
            storage->delete_tag(tag);
        }
    }
}

BtTlvStorage::BtTlvStorage()
{
    m_btstack_tlv.get_tag = tlv_get_tag_wrapper;
    m_btstack_tlv.store_tag = tlv_store_tag_wrapper;
    m_btstack_tlv.delete_tag = tlv_delete_tag_wrapper;
}

BtTlvStorage &BtTlvStorage::instance()
{
    static BtTlvStorage storage;
    return storage;
}

const btstack_tlv_t *BtTlvStorage::btstack_tlv()
{
    return &m_btstack_tlv;
}

int BtTlvStorage::get_tag(uint32_t tag, uint8_t *buffer, uint32_t buffer_size)
{
    for (const auto &entry : m_entries)
    {
        if (entry.tag == tag)
        {
            if (!buffer || buffer_size == 0)
            {
                return entry.length;
            }
            uint32_t to_copy = std::min(static_cast<uint32_t>(entry.length), buffer_size);
            memcpy(buffer, entry.value, to_copy);
            return entry.length;
        }
    }
    return 0;
}

int BtTlvStorage::store_tag(uint32_t tag, const uint8_t *data, uint32_t data_size)
{
    if (!data || data_size == 0)
    {
        delete_tag(tag);
        return 0;
    }

    if (data_size > sizeof(BluetoothTlvData::value))
    {
        data_size = sizeof(BluetoothTlvData::value);
    }

    bool updated = false;
    for (auto &entry : m_entries)
    {
        if (entry.tag == tag)
        {
            if (entry.length == data_size && memcmp(entry.value, data, data_size) == 0)
            {
                return 0;
            }
            entry.length = static_cast<uint8_t>(data_size);
            memcpy(entry.value, data, data_size);
            updated = true;
            break;
        }
    }

    if (!updated)
    {
        BluetoothTlvData entry;
        entry.tag = tag;
        entry.length = static_cast<uint8_t>(data_size);
        memcpy(entry.value, data, data_size);
        m_entries.push_back(entry);
    }

    constexpr uint32_t BTL_PREFIX = ('B' << 24) | ('T' << 16) | ('L' << 8);
    if ((tag & 0xFFFFFF00) == BTL_PREFIX && data_size >= 26)
    {
        const uint8_t *mac = data + 4;
        const uint8_t *key = data + 10;
        int32_t pairing_id = DeviceFactory::find_bluetooth_pairing_id_by_mac(mac);
        if (pairing_id >= 0)
        {
            DeviceFactory::set_bluetooth_pairing_link_key(pairing_id, key);
        }
    }

    constexpr uint32_t BTD_PREFIX = ('B' << 24) | ('T' << 16) | ('D' << 8);
    if ((tag & 0xFFFFFF00) == BTD_PREFIX && data_size >= 46)
    {
        const uint8_t *mac = data + 8;
        const uint8_t *key = data + 30;
        int32_t pairing_id = DeviceFactory::find_bluetooth_pairing_id_by_mac(mac);
        if (pairing_id >= 0)
        {
            DeviceFactory::set_bluetooth_pairing_link_key(pairing_id, key);
        }
    }

    update_aux_tlv();
    return 0;
}

void BtTlvStorage::delete_tag(uint32_t tag)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        if (it->tag == tag)
        {
            m_entries.erase(it);
            update_aux_tlv();
            return;
        }
    }
}

void BtTlvStorage::set_tag_from_config(uint32_t tag, const uint8_t *data, uint32_t data_size)
{
    if (data_size > sizeof(BluetoothTlvData::value))
    {
        data_size = sizeof(BluetoothTlvData::value);
    }
    for (auto &entry : m_entries)
    {
        if (entry.tag == tag)
        {
            entry.length = static_cast<uint8_t>(data_size);
            memcpy(entry.value, data, data_size);
            return;
        }
    }
    BluetoothTlvData entry;
    entry.tag = tag;
    entry.length = static_cast<uint8_t>(data_size);
    memcpy(entry.value, data, data_size);
    m_entries.push_back(entry);
}

void BtTlvStorage::delete_tags_for_mac(const uint8_t mac[6])
{
    bool changed = false;
    constexpr uint32_t BTL_PREFIX = ('B' << 24) | ('T' << 16) | ('L' << 8);
    constexpr uint32_t BTD_PREFIX = ('B' << 24) | ('T' << 16) | ('D' << 8);

    for (auto it = m_entries.begin(); it != m_entries.end();)
    {
        bool matches = false;
        if ((it->tag & 0xFFFFFF00) == BTL_PREFIX && it->length >= 10)
        {
            if (memcmp(it->value + 4, mac, 6) == 0)
            {
                matches = true;
            }
        }
        else if ((it->tag & 0xFFFFFF00) == BTD_PREFIX && it->length >= 14)
        {
            if (memcmp(it->value + 8, mac, 6) == 0)
            {
                matches = true;
            }
        }

        if (matches)
        {
            it = m_entries.erase(it);
            changed = true;
        }
        else
        {
            ++it;
        }
    }

    if (changed)
    {
        update_aux_tlv();
    }
}

void BtTlvStorage::clear()
{
    m_entries.clear();
}

void BtTlvStorage::foreach_entry(const std::function<void(uint32_t tag, const uint8_t *data, uint8_t length)> &callback) const
{
    for (const auto &entry : m_entries)
    {
        callback(entry.tag, entry.value, entry.length);
    }
}

