#include "rf.h"
#ifdef RF_RX
#define RX_CONSOLE_ID() send_console_id()
#define TICK() tick_rf()
#define INIT() init_rx()
NRFLite nrfRadio;
uint8_t rf_data[32];
void init_rx() {
    if (!nrfRadio.init(RADIO_ID, RADIO_CE, RADIO_CSN)) {
        // Maybe turn on an LED to show that an error happened?
        while (1)
            ;  // Wait here forever.
    }
}
void send_console_id() {
    RfConsoleTypePacket_t packet = {
        AckConsoleType, consoleType};
    nrfRadio.addAckData(&packet, sizeof(packet));
}
uint8_t tick_rf() {
    uint8_t size = nrfRadio.hasData();
    if (size) {
        nrfRadio.readData(rf_data);
        if (rf_data[0] == Input) {
            memcpy(&report, rf_data + 1, size - 1);
        }
    }
    return size;
}
#else
#define RX_CONSOLE_ID()
#define INIT()
#define TICK() tick_all(&report)
#endif