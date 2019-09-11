#include "../../shared/output/output_serial.h"
#include "../../shared/config/eeprom.h"
#include "../../shared/output/bootloader/bootloader.h"
#include "../../shared/output/output_handler.h"
#include "avr/wdt.h"
#include <LUFA/Drivers/Board/Board.h>
#include <LUFA/Drivers/Misc/RingBuffer.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
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
uint8_t *controller_data;
uint8_t controller_index;
#define BUFF_SIZE 16
/** Circular buffer to hold data from the host before it is sent to the device
 * via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes
 * are located. */
static uint8_t USBtoUSART_Buffer_Data[BUFF_SIZE];

/** Circular buffer to hold data from the serial port before it is sent to the
 * host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes
 * are located. */
static uint8_t USARTtoUSB_Buffer_Data[BUFF_SIZE];
bool currentlyUploading = false;
void serial_configuration_changed() {
  CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}
void serial_control_request() {
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}
void serial_init(controller_t *c) {
   controller_data = (uint8_t *)c; 
}
void serial_tick() {
  if (currentlyUploading) {
    if (!(RingBuffer_IsFull(&USBtoUSART_Buffer))) {
      int16_t ReceivedByte =
          CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

      /* Store received byte into the USART transmit buffer */
      if (!(ReceivedByte < 0))
        RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedByte);
    }

    uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
    if (BufferCount) {
      Endpoint_SelectEndpoint(
          VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

      /* Check if a packet is already enqueued to the host - if so, we
      shouldn't
       * try to send more data until it completes as there is a chance nothing
       * is listening and a lengthy timeout could occur */
      if (Endpoint_IsINReady()) {
        /* Never send more than one bank size less one byte to the host at a
         * time, so that we don't block while a Zero Length Packet (ZLP) to
         * terminate the transfer is sent if the host isn't listening */
        uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1));

        /* Read bytes from the USART receive buffer into the USB IN endpoint */
        while (BytesToSend--) {
          /* Try to send the next byte of data to the host, abort if there is
          an
           * error without dequeuing */
          if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
                                  RingBuffer_Peek(&USARTtoUSB_Buffer)) !=
              ENDPOINT_READYWAIT_NoError) {
            break;
          }

          /* Dequeue the already sent byte from the buffer now we have
          confirmed
           * that no transmission error occurred */
          RingBuffer_Remove(&USARTtoUSB_Buffer);
        }
      }
    }

    /* Load the next byte from the USART transmit buffer into the USART if
     * transmit buffer space is available */
    if (Serial_IsSendReady() && !(RingBuffer_IsEmpty(&USBtoUSART_Buffer)))
      Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));

    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
  } else {
    int16_t b = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
    if (b > 0) { CDC_Device_SendByte(&VirtualSerial_CDC_Interface, b); }
    if (b == 'r') {
      CDC_Device_SendData(&VirtualSerial_CDC_Interface, &config,
                          sizeof(config_t));
      CDC_Device_SendData(&VirtualSerial_CDC_Interface, controller_data,
                          sizeof(controller_t));
    }
    if (b == 'w') {
      uint8_t *data = (uint8_t *)&config;
      size_t i = 0;
      while (i < sizeof(config_t)) {
        data[i] = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
        if (data[i] != EOF) { i++; }
      }
      reboot();
    }
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    if (controller_index >= sizeof(controller_t) + 2) {
      output_tick();
      controller_index = 0;
    }
  }
}

ISR(USART1_RX_vect, ISR_BLOCK) {
  char data = UDR1;
  if (currentlyUploading) {
    RingBuffer_Insert(&USARTtoUSB_Buffer, data);
  } else {
    switch (controller_index) {
    case 0:
      if (data == 'm') { controller_index++; }
      break;
    case 1:
      if (data == 'a')
        controller_index++;
      else
        controller_index = 0;
      break;
    default:
      if (controller_index < sizeof(controller_t) + 2) {
        controller_data[controller_index - 2] = data;
      }
      controller_index++;
    }
  }
}
int hasReset = 0;
/** Event handler for the CDC Class driver Host-to-Device Line Encoding
Changed
 * event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface
 * configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(
    USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo) {
  bool rst = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
              CDC_CONTROL_LINE_OUT_DTR);

  if (rst)
    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
  else {
    /* Start the flush timer so that overflows occur rapidly to push received
       * bytes to the USB interface */
      TCCR0B = (1 << CS02);

      // /* Pull target /RESET line high */
      AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
      AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
      RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data,
                            sizeof(USBtoUSART_Buffer_Data));
      RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data,
                            sizeof(USARTtoUSB_Buffer_Data));
      currentlyUploading = true;
  }

}