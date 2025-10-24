#include "crkd_neck.hpp"
#include "main.hpp"
#include "utils.h"
#include "main.hpp"
#include "stdio.h"
#include "math.h"
#include "stdint.h"
#include "hardware/irq.h"

// TODO: we would probably want the tool to send a firmware here, instead of needing to hardcode this into the firmware.
static uint8_t firmware[10000];

void CrkdNeck::tick()
{
    if (m_updating)
    {
        if (!m_connected)
        {
            uint8_t data = PY_SYNCH;
            interface.send(&data, sizeof(data));
            m_connected = interface.readable() && check_reply();
        }
        if (m_connected)
        {
            read_info();
            if (!m_error)
            {
                erase(sizeof(firmware));
            }
            if (!m_error)
            {
                writeflash(PY_CODE_ADDR, firmware, sizeof(firmware));
            }
            if (!m_error)
            {
                readflash(PY_CODE_ADDR, sizeof(firmware), firmware);
            }
            m_updating = false;
            interface.set_format(8, 1, UART_PARITY_NONE);
        }
        return;
    }
    auto connected = interface.read_uart(0xA5, sizeof(m_crkdNeck), (uint8_t *)&m_crkdNeck);
    if (connected)
    {
        green = m_crkdNeck.green;
        red = m_crkdNeck.red;
        yellow = m_crkdNeck.yellow;
        blue = m_crkdNeck.blue;
        orange = m_crkdNeck.orange;
        soloGreen = m_crkdNeck.soloGreen;
        soloRed = m_crkdNeck.soloRed;
        soloYellow = m_crkdNeck.soloYellow;
        soloBlue = m_crkdNeck.soloBlue;
        soloOrange = m_crkdNeck.soloOrange;
        rb = m_crkdNeck.footer[0] == 0x03;
        dpadUp = m_crkdNeck.dpadUpDown == 0x00;
        dpadDown = m_crkdNeck.dpadUpDown == 0xFF;
        dpadRight = m_crkdNeck.dpadLeftRight == 0x00;
        dpadLeft = m_crkdNeck.dpadLeftRight == 0xFF;
        m_connected = true;
        m_lastPoll = millis();
    }
    if (millis() - m_lastSend > 10)
    {
        // kick the neck over to polling at 1ms
        uint8_t data[] = {0xA5, 0xC1, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD5};
        interface.send(data, sizeof(data));
        m_lastSend = millis();
    }
    if (millis() - m_lastPoll > 10)
    {
        m_connected = false;
    }
};

bool CrkdNeck::check_reply()
{
    uint8_t reply;
    if (!interface.read_uart(sizeof(reply), &reply))
    {
        return false;
    }
    return reply == PY_REPLY_ACK;
}
void CrkdNeck::send_command(uint8_t cmd)
{
    uint8_t data[] = {cmd, (uint8_t)(cmd ^ 0xff)};
    interface.send(data, sizeof(data));
    if (!check_reply())
    {
        printf("Did not get reply!\r\n");
        m_error = true;
        return;
    }
}
void CrkdNeck::sendaddress(uint32_t addr)
{
    uint8_t data[] = {(uint8_t)(addr >> 24), (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)(addr), 0x00};
    uint8_t parity = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        parity ^= data[i];
    }
    data[4] = parity;
    interface.send(data, sizeof(data));
    if (!check_reply())
    {
        printf("Failed to send address\r\n");
        m_error = true;
    }
}
void CrkdNeck::read_info_stream(uint8_t cmd, uint8_t *out)
{
    send_command(cmd);
    auto size = uart_getc(uart0);
    printf("size: %02x\r\n", size);
    uart_read_blocking(uart0, out, size + 1);
    if (!check_reply())
    {
        printf("Did not get reply!\r\n");
        m_error = true;
        return;
    }
}

void CrkdNeck::read_info()
{
    uint8_t data[64];
    read_info_stream(PY_CMD_GET, data);
    auto ver = data[0];
    printf("%x.%x\r\n", ver >> 4, ver & 7);
    read_info_stream(PY_CMD_PID, data);
    uint16_t pid = data[0] << 8 | data[1];
    printf("%04x\r\n", pid);
    if (pid == PY_CHIP_PID)
    {
        printf("found chip!\r\n");
        return;
    }
    m_error = true;
}

void CrkdNeck::erase(uint16_t size)
{
    uint8_t sectors = ceil((float)size / PY_SECTORSIZE);
    send_command(PY_CMD_ERASE);
    uint8_t temp[64];
    memset(temp, 0, sizeof(temp));
    temp[0] = 0x20;
    temp[1] = sectors - 1;
    uint8_t current = 2;
    for (int i = 1; i <= sectors; i++)
    {
        temp[current++] = i >> 8;
        temp[current++] = i & 0xff;
    }
    for (int i = 0; i < current; i++)
    {
        temp[current] ^= temp[i];
    }
    current++;
    interface.send(temp, current);
    if (!check_reply())
    {
        printf("Failed to erase chip\r\n");
        m_error = true;
        return;
    }
    printf("erased!");
}
void CrkdNeck::readflash(uint32_t addr, uint16_t size, uint8_t *data)
{
    uint8_t block[PY_BLOCKSIZE];
    interface.setup_interrupts(block, sizeof(block));
    for (int i = 0; i < size; i += PY_BLOCKSIZE)
    {
        interface.reset_transfer();
        send_command(PY_CMD_READ);
        sendaddress(addr);
        send_command(PY_BLOCKSIZE - 1);
        while (!interface.transfer_done())
        {
            tight_loop_contents();
        }
        // interface.read_uart(PY_BLOCKSIZE, block);
        if (memcmp(block, data + i, i + PY_BLOCKSIZE > size ? size - i : PY_BLOCKSIZE) != 0)
        {
            printf("found incorrect data at address: %08x\r\n", addr);
            m_error = true;
            break;
        }
        addr += PY_BLOCKSIZE;
    }
    printf("done!\r\n");
    interface.disable_interrupts();
}

void CrkdNeck::writeflash(uint32_t addr, uint8_t *data, uint32_t size)
{
    uint8_t blocksize = PY_BLOCKSIZE;
    for (uint32_t i = 0; i < size; i += blocksize)
    {
        printf("Writing: %08x\r\n", addr);
        if (i + blocksize > size)
        {
            blocksize = size - i;
        }
        uint8_t parity = PY_BLOCKSIZE - 1;
        for (int i2 = 0; i2 < blocksize; i2++)
        {
            parity ^= data[i + i2];
        }
        send_command(PY_CMD_WRITE);
        sendaddress(addr);
        uint8_t sz = PY_BLOCKSIZE - 1;
        interface.send(&sz, sizeof(sz));
        interface.send(data + i, blocksize);
        // pad out final block
        if (blocksize < PY_BLOCKSIZE)
        {
            sz = 0xff;
            for (int i = blocksize; i < PY_BLOCKSIZE; i++)
            {
                interface.send(&sz, sizeof(sz));
                parity ^= sz;
            }
        }
        interface.send(&parity, sizeof(parity));
        if (!check_reply())
        {
            printf("failed to write to address: %08x\r\n", addr);
            m_error = true;
            break;
        }
        addr += blocksize;
    }
}

void CrkdNeck::update_firmware()
{
    m_error = false;
    m_connected = false;
    m_updating = true;
    interface.set_format(8, 1, UART_PARITY_EVEN);
}