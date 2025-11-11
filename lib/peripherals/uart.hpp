#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <hardware/uart.h>

class UARTInterface {
   public:
    UARTInterface(uint8_t block, uint8_t tx, uint8_t rx, uint32_t baudrate);
    bool read_uart(uint8_t header, uint8_t size, uint8_t *dest);
    bool read_uart(uint8_t size, uint8_t *dest);
    bool send(uint8_t* data, uint8_t size);
    void set_format(uint data_bits, uint stop_bits, uart_parity_t parity);
    bool readable();
    void setup_interrupts(uint8_t* dest,uint8_t start_char, size_t maxlen);
    void disable_interrupts();
    bool transfer_done();
    void reset_transfer();
    long last_read_time();

   private:
    uart_inst_t *uart;
    uart_inst_t* _hardwareBlocks[NUM_UARTS] = {uart0,uart1};
};
