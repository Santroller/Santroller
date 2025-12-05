#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "devices/debug.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "stdio.h"
DebugDevice::DebugDevice(proto_DebugDevice device, uint16_t id) : Device(id)
{
    bi_decl(bi_2pins_with_func(device.uart.tx, device.uart.rx, GPIO_FUNC_UART));
    uart_inst = uart_get_instance(device.uart.block);
    stdio_uart_init_full(uart_inst, device.uart.baudrate, device.uart.tx, device.uart.rx);
    m_lastConnected = true;
}

void DebugDevice::update(bool full_poll)
{
    if (full_poll) {
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event);
    }
}