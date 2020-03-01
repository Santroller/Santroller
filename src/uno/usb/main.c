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

const char *mcu = "";
const char *freq = "";
const char *board = ARDWIINO_BOARD;
const char *version = VERSION;
const char *signature = SIGNATURE;
const char *usb_mcu = MCU;
const char *usb_freq = STR(F_CPU);
/** Circular buffer to hold data from the host before it is sent to the device
 * via the serial port. */
RingBuff_t USBtoUSART_Buffer;

/** Circular buffer to hold data from the serial port before it is sent to the
 * host. */
RingBuff_t USARTtoUSB_Buffer;

/** Pulse generation counters to keep track of the number of milliseconds
 * remaining for each pulse type */
volatile struct {
  uint8_t TxLEDPulse;       /**< Milliseconds remaining for data Tx LED pulse */
  uint8_t RxLEDPulse;       /**< Milliseconds remaining for data Rx LED pulse */
  uint8_t PingPongLEDPulse; /**< Milliseconds remaining for enumeration Tx/Rx
                               ping-pong LED pulse */
} PulseMSRemaining;

/** Contains the current baud rate and other settings of the first virtual
 * serial port. This must be retained as some operating systems will not open
 * the port unless the settings can be set successfully.
 */
static CDC_LineEncoding_t LineEncoding = {.BaudRateBPS = 115200,
                                          .CharFormat =
                                              CDC_LINEENCODING_OneStopBit,
                                          .ParityType = CDC_PARITY_None,
                                          .DataBits = 8};
#define STATE_ARDWIINO 0
#define STATE_AVRDUDE 2
#define FRAME_START_1 0x7c
#define FRAME_START_2 0x7e
#define FRAME_END 0x7f
#define ESC 0x7b
eeprom_config_t EEMEM config_mem;

eeprom_config_t config;
bool entered_prog = false;
int state = STATE_ARDWIINO;
int lastCommand = 0;
int lastAddr = 0;
uint8_t frame = 0;
// 0x289 is the final address we have set aside for our own use
bool *jmpToBootloader = (bool *)0x289;
/** Main program entry point. This routine contains the overall program flow,
 * including initial setup of all components and the main program loop.
 */
int main(void) {
  // jmpToBootloader is only valid after a watchdog reset.
  if (!bit_is_set(MCUSR, WDRF)) { *jmpToBootloader = false; }
  if (*jmpToBootloader) {
    *jmpToBootloader = false;
    // Bootloader is at address 0x1000
    asm volatile("jmp 0x1000");
  }
  SetupHardware();
  eeprom_read_block(&config, &config_mem, sizeof(eeprom_config_t));
  if (config.id == ARDWIINO_DEVICE_TYPE) {
    polling_rate = config.polling_rate;
    device_type = config.device_type;
  } else {
    config.id = ARDWIINO_DEVICE_TYPE;
    config.polling_rate = polling_rate;
    config.device_type = device_type;
  }
  RingBuffer_InitBuffer(&USBtoUSART_Buffer, (RingBuff_Data_t *)0x100);
  RingBuffer_InitBuffer(&USARTtoUSB_Buffer, (RingBuff_Data_t *)0x180);
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

#define TX_RX_LED_PULSE_MS 5
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
            if (state == STATE_ARDWIINO) {
              if (lastCommand == COMMAND_START_CONFIG) {
                config.device_type = device_type;
                config.polling_rate = polling_rate;
              } else if (lastCommand == COMMAND_READ_INFO) {
                const char *c = NULL;
                switch (b) {
                case INFO_USB_MCU:
                  c = usb_mcu;
                  break;
                case INFO_USB_CPU_FREQ:
                  c = usb_freq;
                  break;
                }
                if (c != NULL) {
                  while (*(c) != 0) {
                    RingBuffer_Insert(&USARTtoUSB_Buffer, *(c++));
                  }
                }
                lastCommand = 0;
              } else if (lastCommand == COMMAND_WRITE_CONFIG_VALUE) {
                lastCommand = b;
              } else if (lastCommand == CONFIG_SUB_TYPE) {
                config.device_type = b;
                lastCommand = 0;
              } else if (lastCommand == CONFIG_POLL_RATE) {
                config.polling_rate = b;
                lastCommand = 0;
              } else if (lastCommand == 0) {
                if (b == COMMAND_APPLY_CONFIG) {
                  Serial_SendByte(b);
                  eeprom_update_block(&config, &config_mem,
                                      sizeof(eeprom_config_t));
                  *jmpToBootloader = false;
                  reboot();
                } else if (b == COMMAND_JUMP_BOOTLOADER) {
                  state = STATE_AVRDUDE;
                  frame = FRAME_START_2;
                } else if (b == COMMAND_JUMP_BOOTLOADER_UNO) {
                  *jmpToBootloader = true;
                  reboot();
                }
                if (b == COMMAND_WRITE_CONFIG_VALUE) { lastCommand = b; }
                if (b == COMMAND_READ_INFO) { lastCommand = b; }
              } else {
                lastCommand = 0;
              }
            } else {
              if (b == COMMAND_STK_500_ENTER_PROG && !entered_prog) {
                entered_prog = true;
              }
            }
          }

          if (!(Endpoint_BytesInEndpoint())) Endpoint_ClearOUT();
        }
      }
      uint8_t b;
      if (frame != 0) {
        /* Check if the UART receive buffer flush timer has expired or the
        buffer
         * is nearly full */
        RingBuff_Count_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
        if (frame == FRAME_START_1 ||
            ((TIFR0 & (1 << TOV0)) || (BufferCount > BUFFER_NEARLY_FULL))) {
          if (frame == FRAME_START_1) {
            Endpoint_SelectEndpoint(HID_EPADDR_IN);
          } else {
            TIFR0 |= (1 << TOV0);
            if (USARTtoUSB_Buffer.Count) {
              LEDs_TurnOnLEDs(LEDMASK_TX);
              PulseMSRemaining.TxLEDPulse = TX_RX_LED_PULSE_MS;
            }
            Endpoint_SelectEndpoint(CDC_TX_EPADDR);
          }

          bool ready = Endpoint_IsReadWriteAllowed() && Endpoint_IsINReady();
          /* Read bytes from the USART receive buffer into the USB IN
          endpoint
           */
          while (BufferCount--) {
            b = RingBuffer_Remove(&USARTtoUSB_Buffer);
            if (state != STATE_AVRDUDE) {
              if (b == FRAME_END) {
                frame = 0;
                break;
              };
              if (b == FRAME_START_2) {
                frame = b;
                break;
              };
              if (b == ESC) b = RingBuffer_Remove(&USARTtoUSB_Buffer) ^ 0x20;
            }
            if (ready) Endpoint_Write_8(b);
          }
          if (ready) Endpoint_ClearIN();
          if (frame == FRAME_START_2) {
            /* Turn off TX LED(s) once the TX pulse period has elapsed */
            if (PulseMSRemaining.TxLEDPulse && !(--PulseMSRemaining.TxLEDPulse))
              LEDs_TurnOffLEDs(LEDMASK_TX);

            /* Turn off RX LED(s) once the RX pulse period has elapsed */
            if (PulseMSRemaining.RxLEDPulse && !(--PulseMSRemaining.RxLEDPulse))
              LEDs_TurnOffLEDs(LEDMASK_RX);
          }
        }
      } else if (RingBuffer_Peek(&USARTtoUSB_Buffer) == FRAME_START_1) {
        RingBuffer_Remove(&USARTtoUSB_Buffer);
        frame = FRAME_START_1;
      } else {
        RingBuffer_Remove(&USARTtoUSB_Buffer);
      }

      /* Load the next byte from the USART transmit buffer into the USART */
      if (!(RingBuffer_IsEmpty(&USBtoUSART_Buffer))) {
        Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));

        LEDs_TurnOnLEDs(LEDMASK_RX);
        PulseMSRemaining.RxLEDPulse = TX_RX_LED_PULSE_MS;
      }
    }

    // Dont forget LEDs on if suddenly unconfigured.
    LEDs_TurnOffTXLED;
    LEDs_TurnOffRXLED;
  }
}

/** Configures the board hardware and chip peripherals for the demo's
 * functionality. */
void SetupHardware(void) {
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  /* Hardware Initialization */
  Serial_Init(115200, true);
  UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
  LEDs_Init();
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
      // The next dtr after programming will reset the device.
      if (entered_prog) {
        entered_prog = false;
        state = STATE_ARDWIINO;
      }
    }
  }
}

/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK) {
  uint8_t ReceivedByte = UDR1;

  if (USB_DeviceState == DEVICE_STATE_Configured)
    RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
}