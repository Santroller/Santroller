#include "libads1115.hpp"
#include "main.hpp"
#include "stdio.h"

static int64_t restart_handler(__unused alarm_id_t id, void *user_data)
{
    ADS1115 *inst = (ADS1115 *)user_data;
    if (inst)
    {
        inst->processData(0, false, false, false, false);
    }
    return 0;
}
static volatile bool rdy[NUM_BANK0_GPIOS];
void ADS1115::processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected)
{
    if (status != ADS1115_INIT && addr && addr != address)
    {
        return;
    }
    cancel_alarm(restart_alarm_id);
    if (timeout || abort_detected)
    {
        if (status != ADS1115_INIT)
        {
            failCount++;
        }
        // during high load, there might be the occassional drop, so allow a few failures
        if (failCount > 10 || status == ADS1115_INIT)
        {
            status = ADS1115_INIT;
        }
        restart_alarm_id = add_alarm_in_ms(500, restart_handler, this, true);
        return;
    }
    if (stop_detected)
    {
        seen[0] = addr == ADS1115_ADDRESS_ADDR_GND;
        seen[1] = addr == ADS1115_ADDRESS_ADDR_VDD;
        seen[2] = addr == ADS1115_ADDRESS_ADDR_SDA;
        seen[3] = addr == ADS1115_ADDRESS_ADDR_SCL;
        if (!abort_detected)
        {
            failCount = 0;
            switch (status)
            {
            case ADS1115_INIT:
                config = ADS1115_REG_RESET_VAL;
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
                status = ADS1115_POLL_WAIT;
                bufferTx[0] = ADS1115_RA_CONFIG;
                bufferTx[1] = ((config) >> 8);
                bufferTx[2] = ((config) & 0xFF);
                interface.dmaWriteRead(address, bufferTx, 3, nullptr, 0);
                break;
            case ADS1115_POLL_WAIT:
                // wait for RDY
                return;
            case ADS1115_POLL:
                inputs[current] = ((bufferRx[0] << 8) + bufferRx[1]) << 1;
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
        // add 200us delay between commands otherwise the extension is overwhelmed
        // If we dont see any sensors, wait a bit before looking again
        if (!abort_detected && status == ADS1115_INIT && seen[0] && seen[1] && seen[2] && seen[3])
        {
            restart_alarm_id = add_alarm_in_us(200, restart_handler, this, true);
        }

        return;
    }
    switch (status)
    {
    case ADS1115_POLL:
        bufferTx[0] = ADS1115_RA_CONVERSION;
        interface.dmaWriteRead(address, bufferTx, 1, bufferRx, 2);
        break;
    case ADS1115_INIT:
        if (seen[0] && seen[1] && seen[2] && seen[3])
        {
            memset(seen, false, sizeof(seen));
            memset(bufferTxInit, ADS1115_RA_CONFIG, sizeof(bufferTxInit));
            interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_GND, bufferTxInit, 1, bufferRxInit, 1);
            interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_VDD, bufferTxInit + 1, 1, bufferRxInit + 1, 1);
            interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_SDA, bufferTxInit + 2, 1, bufferRxInit + 2, 1);
            interface.dmaWriteRead(ADS1115_ADDRESS_ADDR_SCL, bufferTxInit + 3, 1, bufferRxInit + 3, 1);
        }
        break;
    default:
        printf("unknown status: %d\r\n", status);
        break;
    }
}
void cb(uint gpio, uint32_t event_mask)
{
    rdy[gpio] = true;
}
void ADS1115::begin()
{
    printf("ads1115 begin\r\n");

    if (alert == 0xFF)
    {
        return;
    }
    current = 0;
    gpio_init(alert);
    gpio_set_dir(alert, false);
    gpio_pull_up(alert);
    gpio_set_irq_enabled_with_callback(alert, GPIO_IRQ_EDGE_FALL, true, cb);
    config = ADS1115_REG_RESET_VAL;
    interface.dmaInit(ADS1115_ADDRESS_ADDR_GND, this);
    interface.dmaInit(ADS1115_ADDRESS_ADDR_VDD, this);
    interface.dmaInit(ADS1115_ADDRESS_ADDR_SDA, this);
    interface.dmaInit(ADS1115_ADDRESS_ADDR_SCL, this);
    status = ADS1115_INIT;
    memset(seen, true, sizeof(seen));
    processData(0, false, false, false, false);
}

void ADS1115::end()
{
    cancel_alarm(restart_alarm_id);
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_GND);
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_VDD);
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_SDA);
    interface.dmaDeinit(ADS1115_ADDRESS_ADDR_SCL);
}
void ADS1115::tick()
{
    if (rdy[alert])
    {
        rdy[alert] = false;
        status = ADS1115_POLL;
        processData(0, false, false, false, false);
    }
    interface.tick();
}