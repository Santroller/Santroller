#include "uart.hpp"

#include "hardware/gpio.h"
#include <pico/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
UARTInterface::UARTInterface(uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock)
{
    uart = _hardwareBlocks[block];
    uart_init(uart, clock);
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_function(rx, GPIO_FUNC_UART);
}

bool UARTInterface::send(uint8_t *data, uint8_t size)
{
    if (!uart_is_writable(uart))
    {
        return false;
    }
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