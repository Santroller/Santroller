#include "config/eeprom.h"
#include "device_comms.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "output/serial_handler.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
Controller_t controller;
uint8_t currentReport[sizeof(USB_Report_Data_t)];
uint8_t previousReport[sizeof(USB_Report_Data_t)];
Configuration_t newConfig;
bool readyForPacket = true;
long lastPoll = 0;

int main(void) {
  loadConfig();
  Serial_InitInterrupt(BAUD, true);
  sei();
  initInputs();
  initReports();
  uint8_t packetCount = 0;
  uint8_t state = 0;
  uint8_t *buf;
  while (true) {
    //================================================================================
    // USARTtoUSB
    //================================================================================

    // This requires the USART RX buffer to be 256 bytes.
    uint8_t count = USARTtoUSB_WritePtr - USARTtoUSB_ReadPtr;

    // Check if we have something worth to send
    if (count) {

      // Prepare temporary pointer
      uint16_t tmp; // = 0x100 | USARTtoUSBReadPtr
      asm(
          // Do not initialize high byte, it will be done in first loop
          // below.
          "lds %A[tmp], %[readPtr]\n\t" // (1) Copy read pointer into
                                        // lower byte
          // Outputs
          : [tmp] "=&e"(tmp) // Pointer register, output only
          // Inputs
          : [readPtr] "m"(USARTtoUSB_ReadPtr) // Memory location
      );
      // Write all bytes from USART to the USB endpoint
      do {
        register uint8_t data;
        asm("ldi %B[tmp] , 0x01\n\t"     // (1) Force high byte to 0x01
            "ld %[data] , %a[tmp] +\n\t" // (2) Load next data byte, wraps
                                         // around 255
            // Outputs
            : [data] "=&r"(data), // Output only
              [tmp] "=e"(tmp)     // Input and output
            // Inputs
            : "1"(tmp));

        if (state == 0) {
          if (data == FRAME_START_FEATURE_WRITE) {
            state = 1;
          } else if (data == FRAME_START_FEATURE_READ) {
            state = 2;
          } else if (data == FRAME_DONE) {
            readyForPacket = true;
          }
        } else if (state == 1) {
          packetCount = data;
          state = 3;
        } else if (state == 2) {
          USARTtoUSB_ReadPtr = tmp & 0xFF;
          processHIDReadFeatureReport(data);
          state = 0;
          break;
        } else if (state == 3) {
          if (data == COMMAND_WRITE_CONFIG) {
            buf = (uint8_t *)&config;
            state = 4;
          } else if (data == COMMAND_SET_LEDS) {
            buf = (uint8_t *)&controller.leds;
            state = 5;
          } else {
            handleCommand(data);
            state = 0;
            break;
          }
          packetCount--;
        } else if (state == 4) {
          buf += data;
          state = 5;
        } else if (state == 5) {
          packetCount--;
          *(buf++) = data;
          if (packetCount == 0) {
            state = 0;
            break;
          }
        }
      } while (--count);
      // Save new pointer position
      USARTtoUSB_ReadPtr = tmp & 0xFF;
    } else if (millis() - lastPoll > config.main.pollRate && readyForPacket) {
      lastPoll = millis();
      tickInputs(&controller);
      tickLEDs(&controller);
      uint8_t size;
      // TODO: this needs to go!
      controller.l_x = rand();
      fillReport(currentReport, &size, &controller);
      if (memcmp(currentReport, previousReport, size) != 0) {
        readyForPacket = false;
        uint8_t done = FRAME_START_WRITE;
        writeData(&done, 1);
        writeData(&size, 1);
        writeData(currentReport, size);
        memcpy(previousReport, currentReport, size);
      }
    }
  }
}
// Data being written back to USB after a read
void writeToUSB(const void *const Buffer, uint8_t Length) {
  uint8_t done = FRAME_START_WRITE;
  writeData(&done, 1);
  writeData(&Length, 1);
  writeData((uint8_t *)Buffer, Length);
}

/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
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
      [writePointer] "I"(_SFR_IO_ADDR(
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
      "out %[readPointer], r30\n\t" // (1) Save back new USBtoUSART read
                                    // buffer pointer location
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
      "sts %[UCSR1B_Reg], r30\n\t" // (2) Turn off this interrupt (UDRIE1),
                                   // all bytes sent
      "movw r30, r4\n\t"           // (1) Restore backuped Z pointer again (was
                                   // overwritten again above)
      "reti\n\t"                   // (4) Exit ISR

      // Inputs:
      ::[UDR1_Reg] "m"(UDR1),
      [readPointer] "I"(_SFR_IO_ADDR(
          USBtoUSART_ReadPtr)), // 7 bit pointer to USBtoUSART read buffer
      [writePointer] "m"(
          USBtoUSART_WritePtr), // 7 bit pointer to USBtoUSART write buffer
      [UCSR1B_Reg] "m"(UCSR1B)  // Memory location of UDR1
  );
}