#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "input/inputs/rf.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "bootloader/bootloader.h"
#include "output/serial_commands.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "Descriptors.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>
/** LUFA CDC Class driver interface configuration and state information. This
 * structure is passed to all CDC Class driver functions, so that multiple
 * instances of the same class within a device can be differentiated from one
 * another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
    .Config =
        {
            .ControlInterfaceNumber = INTERFACE_ID_CDC_CCI,
            .DataINEndpoint =
                {
                    .Address = CDC_TX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .DataOUTEndpoint =
                {
                    .Address = CDC_RX_EPADDR,
                    .Size = CDC_TXRX_EPSIZE,
                    .Banks = 1,
                },
            .NotificationEndpoint =
                {
                    .Address = CDC_NOTIFICATION_EPADDR,
                    .Size = CDC_NOTIFICATION_EPSIZE,
                    .Banks = 1,
                },
        },
};

Controller_t controller;
Controller_t previousController;
Configuration_t newConfig;
long lastPoll = 0;
volatile bool send_message = false;
int main(void) {
  loadConfig();
  config.rf.rfInEnabled = false;
  sei();
  // Serial_Init(115200, true);
  USB_Init();
  initInputs();
  initReports();
  initRF(true);
  while (true) {
    if (millis() - lastPoll > config.main.pollRate && rf_interrupt) {
      tickInputs(&controller);
      tickLEDs(&controller);
      if (memcmp(&controller, &previousController, sizeof(Controller_t)) != 0) {
        lastPoll = millis();

        uint8_t data[12];
        if (tickRFTX(&controller, data)) {
          // for (int i = 0; i < sizeof(data); i++) { Serial_SendByte(data[i]);
          // }
        }
        memcpy(&previousController, &controller, sizeof(Controller_t));
      }
    }
    USB_USBTask();
  }
}
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
	/* You can get changes to the virtual CDC lines in this callback; a common
	   use-case is to use the Data Terminal Ready (DTR) flag to enable and
	   disable CDC communications in your application when set to avoid the
	   application blocking while waiting for a host to become ready and read
	   in the pending data from the USB endpoints.
	*/
	bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) != 0;

	(void)HostReady;
}
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
	if (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200) {
		bootloader();
	}
}