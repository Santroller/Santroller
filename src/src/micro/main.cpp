/**
 * You should have a LUFAConfig.h for this to work.
 */

/**
 * Include LUFA.h after LUFAConfig.h
 */
#include <Wire.h>
#include <SPI.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>

#include "descriptors_detect.h"
#include "bootloader.h"

#include <LUFA.h>
#include <LUFA/LUFA/Drivers/Board/LEDs.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <LUFA/LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/LUFA/Platform/Platform.h>

USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = INTERFACE_ID_Device,
				.DataINEndpoint           =
					{
						.Address          = DEVICE_EPADDR_IN,
						.Size             = ENDPOINT_SIZE,
						.Type			  = EP_TYPE_BULK,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = DEVICE_EPADDR_IN,
						.Size             = ENDPOINT_SIZE,
						.Type			  = EP_TYPE_BULK,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION,
						.Size             = ENDPOINT_SIZE,
						.Type			  = EP_TYPE_BULK,
						.Banks            = 1,
					},
			},
		.State = {
			.ControlLineStates = {
				.HostToDevice = 0,
				.DeviceToHost = 0
			},
			.LineEncoding = {
				.BaudRateBPS = 0,
				.CharFormat = 0,
				.ParityType = 0,
				.DataBits = 0
			}
		}
	};

void SetupHardware(void);

volatile bool waiting = true;
void setup()
{
	GlobalInterruptEnable(); // enable global interrupts
	SetupHardware(); // ask LUFA to setup the hardware
}

void loop()
{
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();

}

ISR(WDT_vect) {
    wdt_disable();
    waiting = false;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
	if (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200) {
		bootloader();
	}
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
}