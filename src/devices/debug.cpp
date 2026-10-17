#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "devices/debug.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "stdio.h"
#include "config/config.hpp"
DebugDevice::DebugDevice(proto_DebugDevice device, uint16_t id) : Device(id), m_device(device)
{
    bi_decl(bi_2pins_with_func(device.uart.tx, device.uart.rx, GPIO_FUNC_UART));
    uart_inst = uart_get_instance(device.uart.block);
    // A freshly added device has no pins or baud rate. Initialising stdio then would reset this UART
    // under whatever else is using it, and divide by the zero baud rate
    const bool tx_valid = device.uart.tx >= 0 && device.uart.tx < NUM_BANK0_GPIOS;
    const bool rx_valid = device.uart.rx >= 0 && device.uart.rx < NUM_BANK0_GPIOS;
    if ((tx_valid || rx_valid) && device.uart.block < NUM_UARTS && device.uart.baudrate > 0)
    {
        stdio_uart_init_full(uart_inst, device.uart.baudrate, tx_valid ? device.uart.tx : -1,
                             rx_valid ? device.uart.rx : -1);
    }
    m_lastConnected = true;
}

void DebugDevice::begin()
{
}
void DebugDevice::end(bool full)
{
}
void DebugDevice::update(bool full_poll, bool send_events)
{
}

bool DebugDevice::using_pin(uint8_t pin)
{
    return pin == m_device.uart.rx || pin == m_device.uart.tx;
}