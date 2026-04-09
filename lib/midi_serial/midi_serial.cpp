#include "midi_serial.hpp"
#include "main.hpp"
#include "utils.h"
#include "main.hpp"
#include "stdio.h"
#include "math.h"
#include "stdint.h"
#include "hardware/irq.h"

static MidiDevice *uart0_midi_device;
static MidiDevice *uart1_midi_device;

static void on_uart_rx_0()
{
    while (uart_is_readable(uart0))
    {
        uint8_t data = uart_getc(uart0);
        uart0_midi_device->processMidiData(&data, 1);
    }
}

static void on_uart_rx_1()
{
    while (uart_is_readable(uart1))
    {
        uint8_t data = uart_getc(uart1);
        uart1_midi_device->processMidiData(&data, 1);
    }
}

MidiSerial::MidiSerial(MidiDevice *midiDevice, uint8_t block, uint8_t tx, uint8_t rx, uint32_t clock) : interface(block, tx, rx, clock)
{
    if (interface.is_uart0())
    {
        uart0_midi_device = midiDevice;
        // And set up and enable the interrupt handlers
        irq_set_exclusive_handler(UART0_IRQ, on_uart_rx_0);
        irq_set_enabled(UART0_IRQ, true);

        uart_set_irqs_enabled(uart0, true, false);
    }
    if (interface.is_uart1())
    {
        uart1_midi_device = midiDevice;

        // And set up and enable the interrupt handlers
        irq_set_exclusive_handler(UART1_IRQ, on_uart_rx_1);
        irq_set_enabled(UART1_IRQ, true);

        uart_set_irqs_enabled(uart1, true, false);
    }
}
MidiSerial::~MidiSerial()
{
    if (interface.is_uart0())
    {
        uart0_midi_device = nullptr;
        irq_set_enabled(UART0_IRQ, false);
        uart_set_irqs_enabled(uart0, false, false);
        irq_remove_handler(UART0_IRQ, on_uart_rx_0);
    }
    if (interface.is_uart1())
    {
        uart1_midi_device = nullptr;
        irq_set_enabled(UART1_IRQ, false);
        uart_set_irqs_enabled(uart1, false, false);
        irq_remove_handler(UART1_IRQ, on_uart_rx_1);
    }
}
void MidiSerial::tick()
{
};