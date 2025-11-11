#include "uart.hpp"

#include "hardware/gpio.h"
#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
static uint8_t *irq_dest_0;
static uint8_t *irq_dest_1;
static size_t irq_size_0;
static size_t irq_size_1;
static size_t current_0;
static size_t current_1;
static long read_time_0;
static long read_time_1;
static uint8_t start_char_0 = 0;
static uint8_t start_char_1 = 0;
void on_uart_rx_0()
{
    while (uart_is_readable(uart0) && current_0 < irq_size_0)
    {
        irq_dest_0[current_0++] = uart_getc(uart0);
        // if start_char is set, then wait for a specific start character
        if (current_0 == 1 && start_char_0 && irq_dest_0[0] != start_char_0)
        {
            current_0 = 0;
        }
    }
    // if start_char is set, then we reset so we can wait for the next packet
    if (current_0 == irq_size_0 && start_char_0)
    {
        read_time_0 = millis();
        current_0 = 0;
    }
}
void on_uart_rx_1()
{
    while (uart_is_readable(uart1) && current_1 < irq_size_1)
    {
        irq_dest_1[current_1++] = uart_getc(uart1);
        // if start_char is set, then wait for a specific start character
        if (current_1 == 1 && start_char_1 && irq_dest_1[0] != start_char_1)
        {
            current_1 = 0;
        }
    }
    // if start_char is set, then we reset so we can wait for the next packet
    if (current_1 == irq_size_1 && start_char_1)
    {
        read_time_1 = millis();
        current_1 = 0;
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
    if (uart == uart0)
    {
        current_0 = 0;
    }
    if (uart == uart1)
    {
        current_1 = 0;
    }
}
long UARTInterface::last_read_time()
{
    if (uart == uart0)
    {
        return read_time_0;
    }
    if (uart == uart1)
    {
        return read_time_1;
    }
}

bool UARTInterface::transfer_done()
{
    if (uart == uart0)
    {
        return current_0 == irq_size_0;
    }
    return current_1 == irq_size_1;
}

void UARTInterface::setup_interrupts(uint8_t *dest, uint8_t start_char, size_t maxlen)
{
    if (uart == uart0)
    {
        irq_dest_0 = dest;
        irq_size_0 = maxlen;
        start_char_0 = start_char;

        // And set up and enable the interrupt handlers
        irq_set_exclusive_handler(UART0_IRQ, on_uart_rx_0);
        irq_set_enabled(UART0_IRQ, true);

        uart_set_irq_enables(uart, true, false);
    }
    if (uart == uart1)
    {
        irq_dest_1 = dest;
        irq_size_1 = maxlen;
        start_char_1 = start_char;

        // And set up and enable the interrupt handlers
        irq_set_exclusive_handler(UART1_IRQ, on_uart_rx_1);
        irq_set_enabled(UART1_IRQ, true);

        uart_set_irq_enables(uart, true, false);
    }
}

void UARTInterface::disable_interrupts()
{
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