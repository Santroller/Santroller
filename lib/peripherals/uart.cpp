#include "uart.hpp"

#include "hardware/gpio.h"
#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static uart_inst_t* irq_uart;
static uint8_t* irq_dest;
static size_t irq_size;
static size_t current;
void on_uart_rx() {
    while (uart_is_readable(irq_uart) && current < irq_size) {
        irq_dest[current++] = uart_getc(uart0);
    }
}
UARTInterface::UARTInterface(uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock)
{
    uart = _hardwareBlocks[block];
    uart_init(uart, clock);
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_function(rx, GPIO_FUNC_UART);
}

void UARTInterface::set_format(uint data_bits, uint stop_bits, uart_parity_t parity)
{
    uart_set_format(uart, data_bits, stop_bits, parity);
}
void UARTInterface::reset_transfer()
{
    current = 0;
}

bool UARTInterface::transfer_done()
{
    return current == irq_size;
}


void UARTInterface::setup_interrupts(uint8_t* dest, size_t maxlen)
{
    irq_uart = uart;
    irq_dest = dest;
    irq_size = maxlen;
    int UART_IRQ = uart ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    uart_set_irq_enables(uart, true, false);
}

void UARTInterface::disable_interrupts() {
    int UART_IRQ = uart ? UART0_IRQ : UART1_IRQ;
    irq_set_enabled(UART_IRQ, false);
}

bool UARTInterface::send(uint8_t *data, uint8_t size)
{
    // if (!uart_is_writable(uart))
    // {
    //     return false;
    // }
    uart_write_blocking(uart, data, size);
    return true;
}

bool UARTInterface::read_uart(uint8_t header, uint8_t size, uint8_t *dest)
{
    if (!uart_is_readable(uart))
    {
        return false;
    }
    while (uart_getc(uart) != header)
    {
        if (!uart_is_readable(uart))
        {
            return false;
        }
    }
    uart_read_blocking(uart, dest, size);
    return true;
}

bool UARTInterface::readable()
{
    return uart_is_readable_within_us(uart, 1000);
}

bool UARTInterface::read_uart(uint8_t size, uint8_t *dest)
{
    if (uart_is_readable_within_us(uart, 1000000))
    {
        uart_read_blocking(uart, dest, size);
        return true;
    }
    return false;
}