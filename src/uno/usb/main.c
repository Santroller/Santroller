/*
LUFA Library
Copyright (C) Dean Camera, 2014.

dean [at] fourwalledcubicle [dot] com
www.lufa-lib.org
*/

/*
Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

Permission to use, copy, modify, distribute, and sell this
software and its documentation for any purpose is hereby granted
without fee, provided that the above copyright notice appear in
all copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of the author not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

The author disclaims all warranties with regard to this
software, including all implied warranties of merchantability
and fitness.  In no event shall the author be liable for any
special, indirect or consequential damages or any damages
whatsoever resulting from loss of use, data or profits, whether
in an action of contract, negligence or other tortious action,
arising out of or in connection with the use or performance of
this software.
*/

/*
Copyright(c) 2014-2015 NicoHood
See the readme for credit to other people.

This file is part of Hoodloader2.

Hoodloader2 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Hoodloader2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Hoodloader2.  If not, see <http://www.gnu.org/licenses/>.
*/

/** \file
 *
 *  Main source file for the CDC class bootloader. This file contains the
 * complete bootloader logic.
 */

#define INCLUDE_FROM_BOOTLOADERCDC_C
#include "main.h"

/** Contains the current baud rate and other settings of the first virtual
 * serial port. This must be retained as some operating systems will not open
 * the port unless the settings can be set successfully.
 */
static CDC_LineEncoding_t LineEncoding = {.BaudRateBPS = 0,
                                          .CharFormat =
                                              CDC_LINEENCODING_OneStopBit,
                                          .ParityType = CDC_PARITY_None,
                                          .DataBits = 8};

/* NOTE: Using Linker Magic,
 * - Reserved 256 bytes from start of RAM at 0x100 for UART RX Buffer
 * so we can use 256-byte aligned addresssing.
 * - Also 128 bytes from 0x200 for UART TX buffer, same addressing.
 * normal RAM data starts at 0x280, see offset in makefile*/

#define USART2USB_BUFLEN 256 // 0xFF - 8bit
#define USB2USART_BUFLEN 128 // 0x7F - 7bit

// USB-Serial buffer pointers are saved in GPIORn
// for better access (e.g. cbi) in ISRs
// This has nothing to do with r0 and r1!
// GPIORn – General Purpose I/O Register are located in RAM.
// Make sure to set DEVICE_STATE_AS_GPIOR to 2 in the Lufa config.
// They are initialied in the CDC LineEncoding Event
#define USBtoUSART_ReadPtr GPIOR0 // to use cbi()
#define USARTtoUSB_WritePtr GPIOR1

/* USBtoUSART_WritePtr needs to be visible to ISR. */
/* USARTtoUSB_ReadPtr needs to be visible to CDC LineEncoding Event. */
static volatile uint8_t USBtoUSART_WritePtr = 0;
static volatile uint8_t USARTtoUSB_ReadPtr = 0;

// Bootloader timeout timer in ms
#define EXT_RESET_TIMEOUT_PERIOD 750
int main(void) {
  /* Setup hardware required for the bootloader */
  SetupHardware();

  /* Enable global interrupts so that the USB stack can function */
  GlobalInterruptEnable();

  while (true) {
// Pulse generation counters to keep track of the number of milliseconds
// remaining for each pulse type
#define TX_RX_LED_PULSE_MS 3
    uint8_t TxLEDPulse = 0;
    uint8_t RxLEDPulse = 0;

    // USB-Serial main loop
    while (true) {
      // USB Task
      uint8_t lastState = USB_DeviceState;
      Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
      if (Endpoint_IsSETUPReceived()) USB_Device_ProcessControlRequest();

      // Compare last with new state
      uint8_t newState = USB_DeviceState;
      if (newState != DEVICE_STATE_Configured) {
        // Try to reconnect if communication is still broken
        if (lastState != DEVICE_STATE_Configured) continue;
        // Break and disable USART on connection lost
        else
          break;
      }

      /* Check if endpoint has a command in it sent from the host */
      Endpoint_SelectEndpoint(CDC_RX_EPADDR);
      uint8_t countRX = 0;

      if (Endpoint_IsOUTReceived()) {
        // Check if we received any new bytes and if we still have space in the
        // buffer
        countRX = Endpoint_BytesInEndpoint();

        // Acknowledge zero length packet and dont call any read functions
        if (!countRX) Endpoint_ClearOUT();
      }
      //================================================================================
      // USBtoUSART
      //================================================================================

      // Check how much free space the USBtoUSART buffer has
      uint8_t USBtoUSART_free =
          (USB2USART_BUFLEN - 1) -
          ((USBtoUSART_WritePtr - USBtoUSART_ReadPtr) & (USB2USART_BUFLEN - 1));

      // Read new data from the USB host if we still have space in the buffer
      if (countRX && countRX <= USBtoUSART_free) {
        // Prepare temporary pointer
        uint16_t tmp;                      // = 0x200 | USBtoUSART_WritePtr;
        asm("ldi %B[tmp], 0x02\n\t"        // (1) Force high byte to 0x200
            "lds %A[tmp], %[writePtr]\n\t" // (1) Load USBtoUSART_WritePtr into
                                           // low byte
            // Outputs
            : [ tmp ] "=&e"(tmp) // Pointer register, output only
            // Inputs
            : [ writePtr ] "m"(USBtoUSART_WritePtr) // Memory location
        );

        // Save USB bank into our USBtoUSART ringbuffer
        do {
          register uint8_t data;
          data = Endpoint_Read_8();
          asm("st %a[tmp]+, %[data]\n\t" // (2) Save byte in buffer and
                                         // increment
              "andi %A[tmp], 0x7F\n\t"   // (1) Wrap around pointer, 128 bytes
              // Outputs
              : [ tmp ] "=e"(tmp) // Input and output
              // Inputs
              : "0"(tmp), [ data ] "r"(data));
        } while (--countRX);

        // Acknowledge data
        Endpoint_ClearOUT();

        // Save back new pointer position
        // Just save the lower byte of the pointer
        USBtoUSART_WritePtr = tmp & 0xFF;

        // Enable USART again to flush the buffer
        UCSR1B = (_BV(RXCIE1) | _BV(TXEN1) | _BV(RXEN1) | _BV(UDRIE1));

        // Force Leds to turn on
        USBtoUSART_free = 0;
      }

      // Light RX led if we still have data in the USBtoUSART buffer
      if (USBtoUSART_free != (USB2USART_BUFLEN - 1)) {
        LEDs_TurnOnRXLED;
        RxLEDPulse = TX_RX_LED_PULSE_MS;
      }

      //================================================================================
      // USARTtoUSB
      //================================================================================

      // This requires the USART RX buffer to be 256 bytes.
      uint8_t count = USARTtoUSB_WritePtr - USARTtoUSB_ReadPtr;

      // Check if we have something worth to send
      if (count) {

        // Check if the UART receive buffer flush timer has expired or the
        // buffer is nearly full
        if ((TIFR0 & (1 << TOV0)) || (count >= (CDC_TX_EPSIZE - 1))) {
          // Send data to the USB host
          Endpoint_SelectEndpoint(CDC_TX_EPADDR);

          // CDC device is ready for receiving bytes
          if (Endpoint_IsINReady()) {
            // Send a maximum of up to one bank minus one.
            // If we fill the whole bank we'd have to send an empty Zero Length
            // Packet (ZLP) afterwards to determine the end of the transfer.
            // Since this is more complicated we only send single packets
            // with one byte less than the maximum.
            uint8_t txcount = CDC_TX_EPSIZE - 1;
            if (txcount > count) txcount = count;

            // Prepare temporary pointer
            uint16_t tmp; // = 0x100 | USARTtoUSBReadPtr
            asm(
                // Do not initialize high byte, it will be done in first loop
                // below.
                "lds %A[tmp], %[readPtr]\n\t" // (1) Copy read pointer into
                                              // lower byte
                // Outputs
                : [ tmp ] "=&e"(tmp) // Pointer register, output only
                // Inputs
                : [ readPtr ] "m"(USARTtoUSB_ReadPtr) // Memory location
            );

            // Write all bytes from USART to the USB endpoint
            do {
              register uint8_t data;
              asm("ldi %B[tmp] , 0x01\n\t"     // (1) Force high byte to 0x01
                  "ld %[data] , %a[tmp] +\n\t" // (2) Load next data byte, wraps
                                               // around 255
                  // Outputs
                  : [ data ] "=&r"(data), // Output only
                    [ tmp ] "=e"(tmp)     // Input and output
                  // Inputs
                  : "1"(tmp));
              Endpoint_Write_8(data);
            } while (--txcount);

            // Send data to USB Host now
            Endpoint_ClearIN();

            // Save new pointer position
            USARTtoUSB_ReadPtr = tmp & 0xFF;
          }
        }

        // Light TX led if there is data to be send
        LEDs_TurnOnTXLED;
        TxLEDPulse = TX_RX_LED_PULSE_MS;
      }

      // LED timer overflow.
      // Check Leds (this methode takes less flash than an ISR)
      if (TIFR0 & (1 << TOV0)) {
        // Reset the timer
        // http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits
        TIFR0 = (1 << TOV0);

        // Turn off TX LED once the TX pulse period has elapsed
        if (TxLEDPulse && !(--TxLEDPulse)) LEDs_TurnOffTXLED;

        // Turn off RX LED once the RX pulse period has elapsed
        if (RxLEDPulse && !(--RxLEDPulse)) LEDs_TurnOffRXLED;
      }
    };

    // Reset CDC Serial settings and disable USART properly
    LineEncoding.BaudRateBPS = 0;
    CDC_Device_LineEncodingChanged();

    // Dont forget LEDs on if suddenly unconfigured.
    LEDs_TurnOffTXLED;
    LEDs_TurnOffRXLED;
  }
}

/** Configures all hardware required for the bootloader. */
static void SetupHardware(void) {
  // Disable clock division
  clock_prescale_set(clock_div_1);

  /* Initialize the USB and other board hardware drivers */
  USB_Init();

  /* Start the flush timer for Leds */
  TCCR0B = (1 << CS02); // clk I/O / 256 (From prescaler)

  // Inits Serial pins, leds, reset and erase pins
  // No need to turn off Leds, this is done in the bootkey check function
  Board_Init();
}

/** Event handler for the USB_ConfigurationChanged event. This configures the
 * device's endpoints ready to relay data to and from the attached USB host.
 */
void EVENT_USB_Device_ConfigurationChanged(void) {
  /* Setup CDC Notification, Rx and Tx Endpoints */
  Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPADDR, EP_TYPE_INTERRUPT,
                             CDC_NOTIFICATION_EPSIZE, 1);

  Endpoint_ConfigureEndpoint(CDC_TX_EPADDR, EP_TYPE_BULK, CDC_TX_EPSIZE,
                             CDC_TX_BANK_SIZE);

  Endpoint_ConfigureEndpoint(CDC_RX_EPADDR, EP_TYPE_BULK, CDC_RX_EPSIZE,
                             CDC_RX_BANK_SIZE);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and
 * process control requests sent to the device from the USB host before passing
 * along unhandled control requests to the library for processing internally.
 */
void EVENT_USB_Device_ControlRequest(void) {
  /* Ignore any requests that aren't directed to the CDC interface */
  if ((USB_ControlRequest.bmRequestType &
       (CONTROL_REQTYPE_TYPE | CONTROL_REQTYPE_RECIPIENT)) !=
      (REQTYPE_CLASS | REQREC_INTERFACE)) {
    return;
  }

  /* Process CDC specific control requests */
  uint8_t bRequest = USB_ControlRequest.bRequest;
  if (bRequest == CDC_REQ_GetLineEncoding) {
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
      Endpoint_ClearSETUP();

      /* Write the line coding data to the control endpoint */
      // this one is not inline because its already used somewhere in the usb
      // core, so it will dupe code
      Endpoint_Write_Control_Stream_LE(&LineEncoding,
                                       sizeof(CDC_LineEncoding_t));
      Endpoint_ClearOUT();
    }
  } else if (bRequest == CDC_REQ_SetLineEncoding) {
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
      Endpoint_ClearSETUP();

      // Read the line coding data in from the host into the global struct (made
      // inline)
      // Endpoint_Read_Control_Stream_LE(&LineEncoding,
      // sizeof(CDC_LineEncoding_t));

      uint8_t Length = sizeof(CDC_LineEncoding_t);
      uint8_t *DataStream = (uint8_t *)&LineEncoding;

      bool skip = false;
      while (Length) {
        uint8_t USB_DeviceState_LCL = USB_DeviceState;

        if ((USB_DeviceState_LCL == DEVICE_STATE_Unattached) ||
            (USB_DeviceState_LCL == DEVICE_STATE_Suspended) ||
            (Endpoint_IsSETUPReceived())) {
          skip = true;
          break;
        }

        if (Endpoint_IsOUTReceived()) {
          while (Length && Endpoint_BytesInEndpoint()) {
            *DataStream = Endpoint_Read_8();
            DataStream++;
            Length--;
          }

          Endpoint_ClearOUT();
        }
      }

      if (!skip) {
        do {
          uint8_t USB_DeviceState_LCL = USB_DeviceState;

          if ((USB_DeviceState_LCL == DEVICE_STATE_Unattached) ||
              (USB_DeviceState_LCL == DEVICE_STATE_Suspended))
            break;
        } while (!(Endpoint_IsINReady()));
      }

      // end of inline Endpoint_Read_Control_Stream_LE

      Endpoint_ClearIN();

      CDC_Device_LineEncodingChanged();
    }
  } else if (bRequest == CDC_REQ_SetControlLineState) {
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
      Endpoint_ClearSETUP();
      Endpoint_ClearStatusStage();

      // check DTR state and reset the MCU
      // You could add the OUTPUT declaration here but it wont help since the pc
      // always tries to open the serial port once. At least if the usb is
      // connected this always results in a main MCU reset if the bootloader is
      // executed. From my testings there is no way to avoid this. Its needed as
      // far as I tested, no way.
      // TODO do not reset main MCU (not possible?)
      Board_Reset(USB_ControlRequest.wValue & CDC_CONTROL_LINE_OUT_DTR);
    }
  }
}

/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.u
 */
ISR(USART1_RX_vect, ISR_NAKED) {
  // This ISR doesnt change SREG. Whoa.
  asm volatile(
      "lds r3, %[UDR1_Reg]\n\t" // (1) Load new Serial byte (UDR1) into r3
      "movw r4, r30\n\t"        // (1) Backup Z pointer (r30 -> r4, r31 -> r5)
      "in r30, %[writePointer]\n\t" // (1) Load USARTtoUSB write buffer 8 bit
                                    // pointer to lower Z pointer
      "ldi r31, 0x01\n\t"           // (1) Set higher Z pointer to 0x01
      "st Z+, r3\n\t" // (2) Save UDR1 in Z pointer (USARTtoUSB write buffer)
                      // and increment
      "out %[writePointer], r30\n\t" // (1) Save back new USARTtoUSB buffer
                                     // pointer location
      "movw r30, r4\n\t"             // (1) Restore backuped Z pointer
      "reti\n\t"                     // (4) Exit ISR

      // Inputs:
      ::[UDR1_Reg] "m"(UDR1), // Memory location of UDR1
      [ writePointer ] "I"(_SFR_IO_ADDR(
          USARTtoUSB_WritePtr)) // 8 bit pointer to USARTtoUSB write buffer
  );
}

ISR(USART1_UDRE_vect, ISR_NAKED) {
  // Another SREG-less ISR.
  asm volatile(
      "movw r4, r30\n\t" // (1) Backup Z pointer (r30 -> r4, r31 -> r5)
      "in r30, %[readPointer]\n\t" // (1) Load USBtoUSART read buffer 8 bit
                                   // pointer to lower Z pointer
      "ldi r31, 0x02\n\t"          // (1) Set higher Z pointer to 0x02
      "ld r3, Z+\n\t" // (2) Load next byte from USBtoUSART buffer into r3
      "sts %[UDR1_Reg], r3\n\t"     // (2) Save r3 (next byte) in UDR1
      "out %[readPointer], r30\n\t" // (1) Save back new USBtoUSART read buffer
                                    // pointer location
      "cbi %[readPointer], 7\n\t"   // (2) Wrap around for 128 bytes
      //     smart after-the-fact andi 0x7F without using SREG
      "movw r30, r4\n\t"            // (1) Restore backuped Z pointer
      "in r2, %[readPointer]\n\t"   // (1) Load USBtoUSART read buffer 8 bit
                                    // pointer to r2
      "lds r3, %[writePointer]\n\t" // (1) Load USBtoUSART write buffer to r3
      "cpse r2, r3\n\t"   // (1/2) Check if USBtoUSART read buffer == USBtoUSART
                          // write buffer
      "reti\n\t"          // (4) They are not equal, more bytes coming soon!
      "ldi r30, 0x98\n\t" // (1) Set r30 temporary to new UCSR1B setting
                          // ((1<<RXCIE1) | (1 << RXEN1) | (1 << TXEN1))
      //     ldi needs an upper register, restore Z once more afterwards
      "sts %[UCSR1B_Reg], r30\n\t" // (2) Turn off this interrupt (UDRIE1), all
                                   // bytes sent
      "movw r30, r4\n\t"           // (1) Restore backuped Z pointer again (was
                                   // overwritten again above)
      "reti\n\t"                   // (4) Exit ISR

      // Inputs:
      ::[UDR1_Reg] "m"(UDR1),
      [ readPointer ] "I"(_SFR_IO_ADDR(
          USBtoUSART_ReadPtr)), // 7 bit pointer to USBtoUSART read buffer
      [ writePointer ] "m"(
          USBtoUSART_WritePtr),  // 7 bit pointer to USBtoUSART write buffer
      [ UCSR1B_Reg ] "m"(UCSR1B) // Memory location of UDR1
  );
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
static void CDC_Device_LineEncodingChanged(void) {
  /* Keep the TX line held high (idle) while the USART is reconfigured */
  PORTD |= (1 << 3);

  /* Must turn off USART before reconfiguring it, otherwise incorrect operation
   * may occur */
  UCSR1B = 0;
  UCSR1A = 0;
  UCSR1C = 0;

  /* Flush data that was about to be sent. */
  USBtoUSART_ReadPtr = 0;
  USBtoUSART_WritePtr = 0;
  USARTtoUSB_ReadPtr = 0;
  USARTtoUSB_WritePtr = 0;

  // Only reconfigure USART if we are not in self reprogramming mode
  // and if the CDC Serial is not disabled
  uint32_t BaudRateBPS = LineEncoding.BaudRateBPS;
  
  uint8_t ConfigMask = 0;

  switch (LineEncoding.ParityType) {
  case CDC_PARITY_Odd:
    ConfigMask = ((1 << UPM11) | (1 << UPM10));
    break;
  case CDC_PARITY_Even:
    ConfigMask = (1 << UPM11);
    break;
  }

  if (LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
    ConfigMask |= (1 << USBS1);

  switch (LineEncoding.DataBits) {
  case 6:
    ConfigMask |= (1 << UCSZ10);
    break;
  case 7:
    ConfigMask |= (1 << UCSZ11);
    break;
  case 8:
    ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
    break;
  }

  // Set the new baud rate before configuring the USART
  uint8_t clockSpeed = (1 << U2X1);
  uint16_t brr = SERIAL_2X_UBBRVAL(BaudRateBPS);

  // No need U2X or cant have U2X.
  if ((brr & 1) || (brr > 4095)) {
    brr >>= 1;
    clockSpeed = 0;
  }

  // Or special case 57600 baud for compatibility with the ATmega328
  // bootloader.
  else if (brr == SERIAL_2X_UBBRVAL(57600)) {
    brr = SERIAL_UBBRVAL(57600);
    clockSpeed = 0;
  }

  UBRR1 = brr;

  // Reconfigure the USART
  UCSR1C = ConfigMask;
  UCSR1A = clockSpeed;
  UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

  /* Release the TX line after the USART has been reconfigured */
  PORTD &= ~(1 << 3);
}
