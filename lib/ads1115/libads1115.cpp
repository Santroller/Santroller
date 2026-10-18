#include "libads1115.hpp"
#include "hardware/sync.h"
#include "main.hpp"
#include "stdio.h"
#include <string.h>
#include "hardware/irq.h"

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    ADS1115 *inst = (ADS1115 *)user_data;
    if (inst)
    {
        inst->process_data(0, false, false, false, false);
    }
    return 0;
}
static ADS1115 *ready_handlers[NUM_BANK0_GPIOS] = {};
// How long to wait before probing again when no ADS1115 answered
#define ADS1115_PROBE_RETRY_MS 500
// add 200us delay between commands otherwise the ADS1115 is overwhelmed
#define ADS1115_RETRY_US 200

void ADS1115::schedule_us(uint32_t delay_us)
{
    alarm_id_t id = add_alarm_in_us(delay_us, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}

void ADS1115::probe_later()
{
    alarm_id_t id = add_alarm_in_ms(ADS1115_PROBE_RETRY_MS, restart_handler, this, true);
    restart_alarm_id = (id > 0) ? id : 0;
}

void ADS1115::disconnected()
{
    status = ADS1115_INIT;
    probesPending = 0;
    failCount = 0;
    memset(inputs, 0, sizeof(inputs));
}

void ADS1115::process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (m_stopped)
    {
        return;
    }
    if (status != ADS1115_INIT && addr && addr != address)
    {
        return;
    }
    lastPoll = to_ms_since_boot(get_absolute_time());
    if (restart_alarm_id)
    {
        cancel_alarm(restart_alarm_id);
        restart_alarm_id = 0;
    }
    if ((timeout || abort_detected || stop_detected) && status == ADS1115_INIT && addr && probesPending)
    {
        probesPending--;
    }
    if (timeout || abort_detected)
    {
        if (status == ADS1115_INIT)
        {
            // Nothing at this address, look again once every address has answered
            if (!probesPending)
            {
                probe_later();
            }
            return;
        }
        // during high load, there might be the occassional drop, so allow a few failures
        failCount++;
        if (failCount > 10)
        {
            disconnected();
            probe_later();
            return;
        }
        // Start the current channel's conversion again
        status = ADS1115_RA_LO_THRESH_INIT;
        schedule_us(ADS1115_RETRY_US);
        return;
    }
    if (stop_detected)
    {
        failCount = 0;
        switch (status)
        {
        case ADS1115_INIT:
            // the first ADS1115 to answer is used, the other probes are ignored from here on
            probesPending = 0;
            config = ADS1115_REG_RESET_VAL;
            config &= ~(0xF000);
            config |= (0x4000 + (current * 0x1000));
            config |= 1 << 15;
            status = ADS1115_RA_LO_THRESH_INIT;
            address = addr;
            bufferTx[0] = ADS1115_RA_LO_THRESH;
            bufferTx[1] = 0x00;
            bufferTx[2] = 0x00;
            interface.dmaWriteRead(address, bufferTx, 3, nullptr, 0);
            break;
        case ADS1115_RA_LO_THRESH_INIT:
            status = ADS1115_RA_CONFIG_INIT;
            bufferTx[0] = ADS1115_RA_HI_THRESH;
            bufferTx[1] = ((1 << 15) >> 8);
            bufferTx[2] = ((1 << 15) & 0xFF);
            interface.dmaWriteRead(address, bufferTx, 3, nullptr, 0);
            break;
        case ADS1115_RA_CONFIG_INIT:
            status = ADS1115_CONFIG_WAIT;
            m_ready = false;
            bufferTx[0] = ADS1115_RA_CONFIG;
            bufferTx[1] = ((config) >> 8);
            bufferTx[2] = ((config) & 0xFF);
            interface.dmaWriteRead(address, bufferTx, 3, nullptr, 0);
            break;
        case ADS1115_CONFIG_WAIT:
            status = ADS1115_POLL_WAIT;
            break;
        case ADS1115_POLL_WAIT:
            // wait for RDY
            return;
        case ADS1115_POLL:
        {
            // single ended inputs near GND can read slightly negative, which would wrap to full scale
            int16_t value = (int16_t)((bufferRx[0] << 8) | bufferRx[1]);
            inputs[current] = value < 0 ? 0 : value << 1;
            current++;
            if (current > 3)
            {
                current = 0;
            }
            config &= ~(0xF000);
            config |= (0x4000 + (current * 0x1000));
            config |= 1 << 15;
            status = ADS1115_RA_LO_THRESH_INIT;
            bufferTx[0] = ADS1115_RA_LO_THRESH;
            bufferTx[1] = 0x00;
            bufferTx[2] = 0x00;
            interface.dmaWriteRead(address, bufferTx, 3, nullptr, 0);
            break;
        }
        }
        return;
    }
    switch (status)
    {
    case ADS1115_POLL:
        bufferTx[0] = ADS1115_RA_CONVERSION;
        interface.dmaWriteRead(address, bufferTx, 1, bufferRx, 2);
        break;
    case ADS1115_RA_LO_THRESH_INIT:
        // retrying the start of a conversion
        bufferTx[0] = ADS1115_RA_LO_THRESH;
        bufferTx[1] = 0x00;
        bufferTx[2] = 0x00;
        interface.dmaWriteRead(address, bufferTx, 3, nullptr, 0);
        break;
    case ADS1115_INIT:
        // Probe every address, the answers (or NAKs) count probesPending back down
        if (probesPending)
        {
            break;
        }
        memset(bufferTxInit, ADS1115_RA_CONFIG, sizeof(bufferTxInit));
        probesPending += interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_GND, bufferTxInit, 1, bufferRxInit, 1);
        probesPending += interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_VDD, bufferTxInit + 1, 1, bufferRxInit + 1, 1);
        probesPending += interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_SDA, bufferTxInit + 2, 1, bufferRxInit + 2, 1);
        probesPending += interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_SCL, bufferTxInit + 3, 1, bufferRxInit + 3, 1);
        if (!probesPending)
        {
            probe_later();
        }
        break;
    default:
        status = ADS1115_INIT;
        probesPending = 0;
        schedule_us(ADS1115_RETRY_US);
        break;
    }
}
// A raw handler, so it doesn't replace the shared GPIO callback other drivers (like PSX) use
static void alert_irq_handler()
{
    for (uint gpio = 0; gpio < NUM_BANK0_GPIOS; gpio++)
    {
        if (ready_handlers[gpio] && (gpio_get_irq_event_mask(gpio) & GPIO_IRQ_EDGE_FALL))
        {
            gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_FALL);
            ready_handlers[gpio]->ready();
        }
    }
}
void ADS1115::ready()
{
    if (status == ADS1115_CONFIG_WAIT || status == ADS1115_POLL_WAIT)
    {
        m_ready = true;
    }
}
void ADS1115::begin()
{
    printf("ads1115 begin\r\n");
    m_stopped = false;

    if (alert == 0xFF)
    {
        return;
    }
    m_ready = false;
    ready_handlers[alert] = this;
    current = 0;
    gpio_init(alert);
    gpio_set_dir(alert, false);
    gpio_pull_up(alert);
    gpio_add_raw_irq_handler(alert, alert_irq_handler);
    gpio_set_irq_enabled(alert, GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    config = ADS1115_REG_RESET_VAL;
    interface.dmaInit(ADS1115_ADDRESS_ADDR_GND, this);
    interface.dmaInit(ADS1115_ADDRESS_ADDR_VDD, this);
    interface.dmaInit(ADS1115_ADDRESS_ADDR_SDA, this);
    interface.dmaInit(ADS1115_ADDRESS_ADDR_SCL, this);
    disconnected();
    process_data(0, false, false, false, false);
}

void ADS1115::end()
{
    printf("ads1115 end\r\n");
    // Stop polls scheduling more before cancelling the pending one, or one finishing in between
    // would leave an alarm that fires after this is freed
    m_stopped = true;
    if (alert != 0xFF)
    {
        m_ready = false;
        if (ready_handlers[alert] == this)
        {
            gpio_set_irq_enabled(alert, GPIO_IRQ_EDGE_FALL, false);
            gpio_remove_raw_irq_handler(alert, alert_irq_handler);
            ready_handlers[alert] = nullptr;
        }
    }
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_GND);
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_VDD);
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_SDA);
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_SCL);
    cancel_alarm(restart_alarm_id);
    restart_alarm_id = 0;
    disconnected();
}
void ADS1115::tick()
{
    if (alert == 0xFF)
    {
        return;
    }
    // Masked so the I2C interrupt can't schedule a poll between process_data cancelling the old alarm
    // and storing the new one, which would leave an alarm that end() can't cancel
    if (m_ready && status == ADS1115_POLL_WAIT)
    {
        uint32_t irq_status = save_and_disable_interrupts();
        m_ready = false;
        status = ADS1115_POLL;
        process_data(0, false, false, false, false);
        restore_interrupts(irq_status);
    }
    interface.tick();
    if (lastPoll && !restart_alarm_id && to_ms_since_boot(get_absolute_time()) - lastPoll > 1000)
    {
        uint32_t irq_status = save_and_disable_interrupts();
        // A transfer or the ready alert never came, probe again, or start the conversion again
        if (status == ADS1115_INIT)
        {
            probesPending = 0;
        }
        else
        {
            status = ADS1115_RA_LO_THRESH_INIT;
        }
        process_data(0, false, false, false, false);
        restore_interrupts(irq_status);
    }
}