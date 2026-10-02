#pragma once
#include <stdint.h>
#include <stddef.h>
#include "hardware/i2c.h"
#include "events.pb.h"

void secondary_pico_slave_init(i2c_inst_t *i2c_block, uint sda_pin, uint scl_pin, uint id_gpio);
void secondary_pico_coprocessor_loop();
bool is_secondary_pico_mode();

i2c_inst_t* get_secondary_pico_slave_i2c();
uint get_secondary_pico_slave_sda();
uint get_secondary_pico_slave_scl();
uint get_secondary_pico_slave_id_pin();

bool secondary_pico_enqueue_event(const proto_Event *event);
size_t secondary_pico_handle_get_events(uint8_t *tx_buf, size_t max_len);
void secondary_pico_handle_ota_command(const uint8_t *rx_buf, size_t rx_len);