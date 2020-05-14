/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.
  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the Arduino-usbserial project. This file contains the
 * main tasks of the project and is responsible for the initial application
 * hardware configuration.
 */

#include "main.h"
#include "../../shared/output/bootloader/bootloader.h"
#include "../../shared/output/usb/API.h"
#include "../../shared/util.h"

RingBuff_Data_t USBtoUSART_Buf[BUFFER_SIZE];
RingBuff_Data_t USARTtoSER_Buf[BUFFER_SIZE];
RingBuff_Data_t USARTtoHID_Buf[BUFFER_SIZE];
/** Circular buffer to hold data from the host before it is sent to the device
 * via the serial port. */
RingBuff_t USBtoUSART_Buffer;

/** Circular buffer to hold data from the serial port before it is sent to the
 * host. */
RingBuff_t USARTtoSER_Buffer;
/** Circular buffer to hold data from the serial port before it is sent to the
 * host. */
RingBuff_t USARTtoHID_Buffer;

/** Contains the current baud rate and other settings of the first virtual
 * serial port. This must be retained as some operating systems will not open
 * the port unless the settings can be set successfully.
 */
static CDC_LineEncoding_t LineEncoding = {0};
#define BAUD 1000000
#define FRAME_START_1 0x7c
#define FRAME_START_2 0x7e
#define FRAME_END 0x7f
#define ESC 0x7b
eeprom_config_t EEMEM config_mem;

eeprom_config_t config;
bool entered_prog = false;
bool is_ardwiino = true;
int lastCommand = 0;
#define JUMP 0xDEAD8001

// by placing this in noinit, we can jump to the bootloader after a watchdog
// reset.
uint32_t jmpToBootloader __attribute__((section(".noinit")));
void handle_out(uint8_t ep, RingBuff_t *buf, bool serial);

/** Main program entry point. This routine contains the overall program flow,
 * including initial setup of all components and the main program loop.
 */
void set_baud(bool b) {
  is_ardwiino = b;
  UCSR1B = 0;
  UCSR1A = 0;
  UCSR1C = 0;
  /* Hardware Initialization */
  UBRR1 = is_ardwiino ? SERIAL_2X_UBBRVAL(BAUD) : SERIAL_2X_UBBRVAL(115200);

  UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
  UCSR1A = (1 << U2X1);
  UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
}
int main(void) {
  if (jmpToBootloader == JUMP) {
    jmpToBootloader = 0;
    // Bootloader is at address 0x1000
    asm volatile("jmp 0x1000");
  }

  SetupHardware();
  eeprom_read_block(&config, &config_mem, sizeof(eeprom_config_t));

  if (config.id == ARDWIINO_DEVICE_TYPE) {
    device_type = config.device_type;
  } else {
    config.id = ARDWIINO_DEVICE_TYPE;
    config.device_type = device_type;
  }

  RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buf);
  RingBuffer_InitBuffer(&USARTtoSER_Buffer, USARTtoSER_Buf);
  RingBuffer_InitBuffer(&USARTtoHID_Buffer, USARTtoHID_Buf);
  sei();
  for (;;) {
    for (;;) {
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

      /* Only try to read in bytes from the CDC interface if the transmit buffer
       * is not full */
      if (!(RingBuffer_IsFull(&USBtoUSART_Buffer))) {
        Endpoint_SelectEndpoint(CDC_RX_EPADDR);
        /* Read bytes from the USB OUT endpoint into the USART transmit buffer
         */
        if (Endpoint_IsOUTReceived()) {
          if (Endpoint_BytesInEndpoint()) {
            uint8_t b = Endpoint_Read_8();
            RingBuffer_Insert(&USBtoUSART_Buffer, b);
            if (!is_ardwiino) {
              entered_prog |= b == COMMAND_STK_500_ENTER_PROG;
            } else if (lastCommand == COMMAND_WRITE_CONFIG_VALUE) {
              lastCommand = b;
            } else if (lastCommand == CONFIG_SUB_TYPE) {
              config.device_type = b;
              lastCommand = 0;
            } else if (b == COMMAND_APPLY_CONFIG) {
              Serial_SendByte(b);
              _delay_ms(2000);
              eeprom_update_block(&config, &config_mem,
                                  sizeof(eeprom_config_t));
              jmpToBootloader = 0;
              reboot();
            } else if (b == COMMAND_JUMP_BOOTLOADER) {
              set_baud(false);
            } else if (b == COMMAND_JUMP_BOOTLOADER_UNO) {
              jmpToBootloader = JUMP;
              reboot();
            } else if (b == COMMAND_WRITE_CONFIG_VALUE) {
              lastCommand = b;
            }
          }

          if (!(Endpoint_BytesInEndpoint())) Endpoint_ClearOUT();
        }
      }
      handle_out(HID_EPADDR_IN, &USARTtoHID_Buffer, false);
      handle_out(CDC_TX_EPADDR, &USARTtoSER_Buffer, true);
      /* Load the next byte from the USART transmit buffer into the USART */
      if (!(RingBuffer_IsEmpty(&USBtoUSART_Buffer))) {
        Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));
      }
    }
  }
}
void handle_out(uint8_t ep, RingBuff_t *buf, bool serial) {

  RingBuff_Count_t BufferCount = RingBuffer_GetCount(buf);
  if (BufferCount == 0) return;
  if (serial) {
    if ((((TIFR0 & (1 << TOV0)) == 0) && (BufferCount < BUFFER_NEARLY_FULL))) {
      return;
    }
    TIFR0 |= (1 << TOV0);
  }
  uint8_t b;
  if (is_ardwiino) {
    b = RingBuffer_Remove(buf);
    if (b != FRAME_START_1 && b != FRAME_START_2) { return; }
  }
  Endpoint_SelectEndpoint(ep);

  if (Endpoint_IsReadWriteAllowed() && Endpoint_IsINReady()) {
    /* Read bytes from the USART receive buffer into the USB IN
    endpoint
     */
    while (BufferCount--) {
      b = RingBuffer_Remove(buf);
      if (is_ardwiino) {
        BufferCount++;
        if (b == FRAME_END) { break; };
        if (b == ESC) b = RingBuffer_Remove(buf) ^ 0x20;
      }
      Endpoint_Write_8(b);
    }
    Endpoint_ClearIN();
  }
}
/** Configures the board hardware and chip peripherals for the demo's
 * functionality. */
void SetupHardware(void) {
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
  /* Hardware Initialization */
  set_baud(true);
  DDRD |= (1 << 3);
  PORTD |= (1 << 2);
  USB_Init();

  /* Start the flush timer so that overflows occur rapidly to push received
   * bytes to the USB interface */
  TCCR0B = (1 << CS02);

  /* Pull target /RESET line high */
  AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
  AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {

  /* Setup CDC Notification, Rx and Tx Endpoints */
  Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPADDR, EP_TYPE_INTERRUPT,
                             CDC_NOTIFICATION_EPSIZE, 1);

  Endpoint_ConfigureEndpoint(CDC_TX_EPADDR, EP_TYPE_BULK, CDC_TX_EPSIZE,
                             CDC_TX_BANK_SIZE);

  Endpoint_ConfigureEndpoint(CDC_RX_EPADDR, EP_TYPE_BULK, CDC_RX_EPSIZE,
                             CDC_RX_BANK_SIZE);
  Endpoint_ConfigureEndpoint(HID_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
}

/** Event handler for the library USB Unhandled Control Request event. */
void EVENT_USB_Device_ControlRequest(void) {
  controller_control_request();
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

      Endpoint_ClearIN();
    }
  } else if (bRequest == CDC_REQ_SetControlLineState) {
    if (USB_ControlRequest.bmRequestType ==
        (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
      Endpoint_ClearSETUP();
      Endpoint_ClearStatusStage();
      Board_Reset(USB_ControlRequest.wValue & CDC_CONTROL_LINE_OUT_DTR);
      if (entered_prog) {
        entered_prog = false;
        set_baud(true);
      }
    }
  }
}

uint8_t frame = 0;
/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK) {
  uint8_t b = UDR1;

  if (b == FRAME_START_1 || b == FRAME_START_2) { frame = b; }
  if (frame == FRAME_START_2 || !is_ardwiino) {
    RingBuffer_Insert(&USARTtoSER_Buffer, b);
  } else if (frame == FRAME_START_1) {
    RingBuffer_Insert(&USARTtoHID_Buffer, b);
  }
  if (b == FRAME_END) { frame = 0; }
}