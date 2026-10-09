#pragma once
// Fake config/config.hpp: bt_tlv_storage.cpp only needs update_aux_tlv, which saves the aux block
namespace fake_config
{
inline int update_aux_tlv_calls = 0;
}
inline void update_aux_tlv() { fake_config::update_aux_tlv_calls++; }
