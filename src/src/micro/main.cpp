/**
 * You should have a LUFAConfig.h for this to work.
 */
#include "LUFAConfig.h"

/**
 * Include LUFA.h after LUFAConfig.h
 */
#include <LUFA.h>

#include <Wire.h>
#include <SPI.h>

/**
 * Finally include the LUFA device setup header
 */
#include "DualVirtualSerial.h"

#include "ArduinoSerial.h"

void setup()
{
  
	SetupHardware(); // ask LUFA to setup the hardware

	GlobalInterruptEnable(); // enable global interrupts
}

void loop()
{
	delayMicroseconds(100);
	delay(500); // a little delay so we don't spam the console...

	// write to the first Serial
	SerialU1.print("SerialU1: ");
	SerialU1.println(millis());

	// write to the second Serial
	SerialU2.print("SerialU2: ");
	SerialU2.println(millis());

	// read from the first serial
	while (SerialU1.available())
	{
		char c = SerialU1.read();
		SerialU1.print("From 1: ");
		SerialU1.println(c);
		SerialU2.print("From 1: ");
		SerialU2.println(c);
	}

	// read from the second serial
	while (SerialU2.available())
	{
		char c = SerialU2.read();
		SerialU1.print("From 2: ");
		SerialU1.println(c);
		SerialU2.print("From 2: ");
		SerialU2.println(c);
	}

	// These 3 lines let LUFA to process the USB task
	CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);
	CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
	USB_USBTask();

	// These 2 lets the LUFACDCSerial to accept buffers.
	SerialU1.accept();
	SerialU2.accept();
}
