#include <Arduino.h>
#include <SPI.h>

#include "NRFLite.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "defines.h"
#include "hid.h"
#include "packets.h"
#include "rf.h"
#include "shared_main.h"

NRFLite nrfRadio;
USB_Report_Data_t report;
RfInputPacket_t rf_report;
RfHeartbeatPacket_t rf_heartbeat;
uint8_t response[32];
void setup() {
    init_main();
    rf_report.id = Input;
    rf_heartbeat.id = Heartbeat;
    if (!nrfRadio.init(RADIO_ID, RADIO_CE, RADIO_CSN)) {
        while (1)
            ;  // Wait here forever.
    }
}
void loop() {
    uint8_t size = tick_inputs(&report);
    if (size > 0) {
        memcpy(rf_report.data, &report, size);
        nrfRadio.send(DEST_RADIO_ID, &rf_report, size + 1);
    } else {
        nrfRadio.send(DEST_RADIO_ID, &rf_heartbeat, sizeof(rf_heartbeat));
    }
    size = nrfRadio.hasAckData();
    if (size) {
        nrfRadio.readData(&response);
        switch (response[0]) {
            case AckConsoleType:
                consoleType = response[1];
                break;
            case AckAuthLed:
                handle_auth_led();
                break;
            case AckPlayerLed:
                handle_player_leds(response[1]);
                break;
            case AckRumble:
                handle_rumble(response[1], response[2]);
                break;
            case AckKeyboardLed:
                handle_keyboard_leds(response[1]);
                break;
            case AckCommandRequest:
                RfCommandRequestPacket_t* request = (RfCommandRequestPacket_t*)response;
                bool success;
                size = handle_serial_command(request->request, request->wValue, (response + 1), &success);
                response[0] = CommandResponse;
                nrfRadio.send(DEST_RADIO_ID, response, size + 1);
        }
    }
}
