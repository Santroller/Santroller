#include "crkd_neck.hpp"
void CrkdNeck::tick() {
    interface.read_uart(0x65, sizeof(m_crkdNeck), (uint8_t*)&m_crkdNeck);
    green = m_crkdNeck.green;
    red = m_crkdNeck.red;
    yellow = m_crkdNeck.yellow;
    blue = m_crkdNeck.blue;
    orange = m_crkdNeck.orange;
    dpadUp = m_crkdNeck.dpadUpDown == 0x00;
    dpadDown = m_crkdNeck.dpadUpDown == 0xFF;
    dpadRight = m_crkdNeck.dpadLeftRight == 0x00;
    dpadLeft = m_crkdNeck.dpadLeftRight == 0xFF;
};