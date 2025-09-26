#include "crkd_neck.hpp"
#include "main.hpp"
#include "utils.h"
#include "main.hpp"
void CrkdNeck::tick() {
    auto connected = interface.read_uart(0xA5, sizeof(m_crkdNeck), (uint8_t*)&m_crkdNeck);
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
    if (connected) {
        m_connected = true;
        m_lastPoll = millis();
    }
    if (millis() - m_lastPoll > 10) {
        m_connected = false;
    }
};